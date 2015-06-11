/*
 * Copyright (c) 2015, Cryptonomex, Inc.
 * All rights reserved.
 *
 * This source code is provided for evaluation in private test networks only, until September 8, 2015. After this date, this license expires and
 * the code may not be used, modified or distributed for any purpose. Redistribution and use in source and binary forms, with or without modification,
 * are permitted until September 8, 2015, provided that the following conditions are met:
 *
 * 1. The code and/or derivative works are used only for private test networks consisting of no more than 10 P2P nodes.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <graphene/chain/database.hpp>

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/delegate_object.hpp>
#include <graphene/chain/global_property_object.hpp>
#include <graphene/chain/vesting_balance_object.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/witness_schedule_object.hpp>
#include <graphene/chain/worker_object.hpp>

#include <fc/uint128.hpp>

namespace graphene { namespace chain {

template<class ObjectType>
vector<std::reference_wrapper<const ObjectType>> database::sort_votable_objects(size_t count) const
{
   const auto& all_objects = dynamic_cast<const simple_index<ObjectType>&>(get_index<ObjectType>());
   count = std::min(count, all_objects.size());
   vector<std::reference_wrapper<const ObjectType>> refs;
   refs.reserve(all_objects.size());
   std::transform(all_objects.begin(), all_objects.end(),
                  std::back_inserter(refs),
                  [](const ObjectType& o) { return std::cref(o); });
   std::partial_sort( refs.begin(), refs.begin() + count, refs.end(),
                   [this]( const ObjectType& a, const ObjectType& b )->bool {
      return _vote_tally_buffer[a.vote_id] > _vote_tally_buffer[b.vote_id];
   });

   refs.resize(count, refs.front());
   return refs;
}

void database::pay_workers( share_type& budget )
{
   ilog("Processing payroll! Available budget is ${b}", ("b", budget));
   vector<std::reference_wrapper<const worker_object>> active_workers;
   get_index_type<worker_index>().inspect_all_objects([this, &active_workers](const object& o) {
      const worker_object& w = static_cast<const worker_object&>(o);
      auto now = _pending_block.timestamp;
      if( w.is_active(now) && w.approving_stake(_vote_tally_buffer) > 0 )
         active_workers.emplace_back(w);
   });

   std::sort(active_workers.begin(), active_workers.end(), [this](const worker_object& wa, const worker_object& wb) {
      return wa.approving_stake(_vote_tally_buffer) > wb.approving_stake(_vote_tally_buffer);
   });

   for( int i = 0; i < active_workers.size() && budget > 0; ++i )
   {
      const worker_object& active_worker = active_workers[i];
      share_type requested_pay = active_worker.daily_pay;
      if( _pending_block.timestamp - get_dynamic_global_properties().last_budget_time != fc::days(1) )
      {
         fc::uint128 pay(requested_pay.value);
         pay *= (_pending_block.timestamp - get_dynamic_global_properties().last_budget_time).count();
         pay /= fc::days(1).count();
         requested_pay = pay.to_uint64();
      }

      share_type actual_pay = std::min(budget, requested_pay);
      ilog(" ==> Paying ${a} to worker ${w}", ("w", active_worker.id)("a", actual_pay));
      modify(active_worker, [&](worker_object& w) {
         w.worker.visit(worker_pay_visitor(actual_pay, *this));
      });

      budget -= actual_pay;
   }
}

void database::update_active_witnesses()
{ try {
   assert( _witness_count_histogram_buffer.size() > 0 );
   share_type stake_target = _total_voting_stake / 2;
   share_type stake_tally = _witness_count_histogram_buffer[0];
   int witness_count = 0;
   while( (size_t(witness_count) < _witness_count_histogram_buffer.size())
          && (stake_tally <= stake_target) )
      stake_tally += _witness_count_histogram_buffer[++witness_count];

   auto wits = sort_votable_objects<witness_object>(std::max(witness_count*2+1, GRAPHENE_MIN_WITNESS_COUNT));
   const global_property_object& gpo = get_global_properties();

   modify( gpo, [&]( global_property_object& gp ){
      gp.active_witnesses.clear();
      gp.active_witnesses.reserve( wits.size() );
      std::transform(wits.begin(), wits.end(),
                     std::inserter(gp.active_witnesses, gp.active_witnesses.end()),
                     [](const witness_object& w) {
         return w.id;
      });
      gp.witness_accounts.clear();
      gp.witness_accounts.reserve( wits.size() );
      std::transform(wits.begin(), wits.end(),
                     std::inserter(gp.witness_accounts, gp.witness_accounts.end()),
                     [](const witness_object& w) {
         return w.witness_account;
      });
   });

   const witness_schedule_object& wso = witness_schedule_id_type()(*this);
   modify( wso, [&]( witness_schedule_object& _wso )
   {
      _wso.scheduler.update( gpo.active_witnesses );
   } );

} FC_CAPTURE_AND_RETHROW() }

void database::update_active_delegates()
{ try {
   assert( _committee_count_histogram_buffer.size() > 0 );
   uint64_t stake_target = _total_voting_stake / 2;
   uint64_t stake_tally = _committee_count_histogram_buffer[0];
   int delegate_count = 0;
   while( (size_t(delegate_count) < _committee_count_histogram_buffer.size())
          && (stake_tally <= stake_target) )
      stake_tally += _committee_count_histogram_buffer[++delegate_count];

   auto delegates = sort_votable_objects<delegate_object>(std::max(delegate_count*2+1, GRAPHENE_MIN_DELEGATE_COUNT));

   // Update genesis authorities
   if( !delegates.empty() )
      modify( get(account_id_type()), [&]( account_object& a ) {
         uint64_t total_votes = 0;
         map<account_id_type, uint64_t> weights;
         a.owner.weight_threshold = 0;
         a.owner.auths.clear();

         for( const delegate_object& del : delegates )
         {
            weights.emplace(del.delegate_account, _vote_tally_buffer[del.vote_id]);
            total_votes += _vote_tally_buffer[del.vote_id];
         }

         // total_votes is 64 bits. Subtract the number of leading low bits from 64 to get the number of useful bits,
         // then I want to keep the most significant 16 bits of what's left.
         int8_t bits_to_drop = std::max(int(boost::multiprecision::detail::find_msb(total_votes)) - 15, 0);
         for( const auto& weight : weights )
         {
            // Ensure that everyone has at least one vote. Zero weights aren't allowed.
            uint16_t votes = std::max((weight.second >> bits_to_drop), uint64_t(1) );
            a.owner.auths[weight.first] += votes;
            a.owner.weight_threshold += votes;
         }

         a.owner.weight_threshold /= 2;
         a.owner.weight_threshold += 1;
         a.active = a.owner;
      });
   modify( get_global_properties(), [&]( global_property_object& gp ) {
      gp.active_delegates.clear();
      std::transform(delegates.begin(), delegates.end(),
                     std::back_inserter(gp.active_delegates),
                     [](const delegate_object& d) { return d.id; });
   });
} FC_CAPTURE_AND_RETHROW() }

share_type database::get_max_budget( fc::time_point_sec now )const
{
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   const asset_object& core = asset_id_type(0)(*this);
   const asset_dynamic_data_object& core_dd = core.dynamic_asset_data_id(*this);

   if(    (dpo.last_budget_time == fc::time_point_sec())
       || (now <= dpo.last_budget_time) )
      return share_type(0);

   int64_t dt = (now - dpo.last_budget_time).to_seconds();

   // We'll consider accumulated_fees to be burned at the BEGINNING
   // of the maintenance interval.  However, for speed we only
   // call modify() on the asset_dynamic_data_object once at the
   // end of the maintenance interval.  Thus the accumulated_fees
   // are available for the budget at this point, but not included
   // in core.burned().
   share_type reserve = core.burned(*this) + core_dd.accumulated_fees;

   fc::uint128_t budget_u128 = reserve.value;
   budget_u128 *= uint64_t(dt);
   budget_u128 *= GRAPHENE_CORE_ASSET_CYCLE_RATE;
   //round up to the nearest satoshi -- this is necessary to ensure
   //   there isn't an "untouchable" reserve, and we will eventually
   //   be able to use the entire reserve
   budget_u128 += ((uint64_t(1) << GRAPHENE_CORE_ASSET_CYCLE_RATE_BITS) - 1);
   budget_u128 >>= GRAPHENE_CORE_ASSET_CYCLE_RATE_BITS;
   share_type budget;
   if( budget_u128 < reserve.value )
      budget = share_type(budget_u128.to_uint64());
   else
      budget = reserve;

   return budget;
}

/**
 * Update the budget for witnesses and workers.
 */
void database::process_budget()
{
   try
   {
      const global_property_object& gpo = get_global_properties();
      const dynamic_global_property_object& dpo = get_dynamic_global_properties();
      const asset_dynamic_data_object& core =
         asset_id_type(0)(*this).dynamic_asset_data_id(*this);
      fc::time_point_sec now = _pending_block.timestamp;

      int64_t time_to_maint = (dpo.next_maintenance_time - now).to_seconds();
      //
      // The code that generates the next maintenance time should
      //    only produce a result in the future.  If this assert
      //    fails, then the next maintenance time algorithm is buggy.
      //
      assert( time_to_maint > 0 );
      //
      // Code for setting chain parameters should validate
      //    block_interval > 0 (as well as the humans proposing /
      //    voting on changes to block interval).
      //
      assert( gpo.parameters.block_interval > 0 );
      uint64_t blocks_to_maint = (uint64_t(time_to_maint) + gpo.parameters.block_interval - 1) / gpo.parameters.block_interval;

      // blocks_to_maint > 0 because time_to_maint > 0,
      // which means numerator is at least equal to block_interval

      share_type available_funds = get_max_budget(now);

      share_type witness_budget = gpo.parameters.witness_pay_per_block.value * blocks_to_maint;
      witness_budget = std::min(witness_budget, available_funds);
      available_funds -= witness_budget;

      fc::uint128_t worker_budget_u128 = gpo.parameters.worker_budget_per_day.value;
      worker_budget_u128 *= uint64_t(time_to_maint);
      worker_budget_u128 /= 60*60*24;

      share_type worker_budget;
      if( worker_budget_u128 >= available_funds.value )
         worker_budget = available_funds;
      else
         worker_budget = worker_budget_u128.to_uint64();
      available_funds -= worker_budget;

      share_type leftover_worker_funds = worker_budget;
      pay_workers(leftover_worker_funds);
      available_funds += leftover_worker_funds;

      modify(core, [&]( asset_dynamic_data_object& _core )
      {
         _core.current_supply = (_core.current_supply + witness_budget +
                                 worker_budget - leftover_worker_funds -
                                 _core.accumulated_fees);
         _core.accumulated_fees = 0;
      });
      modify(dpo, [&]( dynamic_global_property_object& _dpo )
      {
         // Should this be +=?
         _dpo.witness_budget = witness_budget;
         _dpo.last_budget_time = now;
      });

      // available_funds is money we could spend, but don't want to.
      // we simply let it evaporate back into the reserve.
   }
   FC_CAPTURE_AND_RETHROW()
}

void database::perform_chain_maintenance(const signed_block& next_block, const global_property_object& global_props)
{
   const auto& gpo = get_global_properties();

   struct vote_tally_helper {
      database& d;
      const global_property_object& props;

      vote_tally_helper(database& d, const global_property_object& gpo)
         : d(d), props(gpo)
      {
         d._vote_tally_buffer.resize(props.next_available_vote_id);
         d._witness_count_histogram_buffer.resize(props.parameters.maximum_witness_count / 2 + 1);
         d._committee_count_histogram_buffer.resize(props.parameters.maximum_committee_count / 2 + 1);
         d._total_voting_stake = 0;
      }

      void operator()(const account_object& stake_account) {
         if( props.parameters.count_non_member_votes || stake_account.is_member(d.head_block_time()) )
         {
            // There may be a difference between the account whose stake is voting and the one specifying opinions.
            // Usually they're the same, but if the stake account has specified a voting_account, that account is the one
            // specifying the opinions.
            const account_object& opinion_account =
                  (stake_account.voting_account == account_id_type())? stake_account
                                                                     : d.get(stake_account.voting_account);

            const auto& stats = stake_account.statistics(d);
            uint64_t voting_stake = stats.total_core_in_orders.value
                  + (stake_account.cashback_vb.valid() ? (*stake_account.cashback_vb)(d).balance.amount.value: 0)
                  + d.get_balance(stake_account.get_id(), asset_id_type()).amount.value;

            for( vote_id_type id : opinion_account.votes )
            {
               uint32_t offset = id.instance();
               // if they somehow managed to specify an illegal offset, ignore it.
               if( offset < d._vote_tally_buffer.size() )
                  d._vote_tally_buffer[ offset ] += voting_stake;
            }

            if( opinion_account.num_witness <= props.parameters.maximum_witness_count )
            {
               uint16_t offset = std::min(size_t(opinion_account.num_witness/2),
                                          d._witness_count_histogram_buffer.size() - 1);
               // votes for a number greater than maximum_witness_count
               // are turned into votes for maximum_witness_count.
               //
               // in particular, this takes care of the case where a
               // member was voting for a high number, then the
               // parameter was lowered.
               d._witness_count_histogram_buffer[ offset ] += voting_stake;
            }
            if( opinion_account.num_committee <= props.parameters.maximum_committee_count )
            {
               uint16_t offset = std::min(size_t(opinion_account.num_committee/2),
                                          d._committee_count_histogram_buffer.size() - 1);
               // votes for a number greater than maximum_committee_count
               // are turned into votes for maximum_committee_count.
               //
               // same rationale as for witnesses
               d._committee_count_histogram_buffer[ offset ] += voting_stake;
            }

            d._total_voting_stake += voting_stake;
         }
      }
   } tally_helper(*this, gpo);
   struct process_fees_helper {
      database& d;
      const global_property_object& props;

      process_fees_helper(database& d, const global_property_object& gpo)
         : d(d), props(gpo) {}

      share_type cut_fee(share_type a, uint16_t p)const
      {
         if( a == 0 || p == 0 )
            return 0;
         if( p == GRAPHENE_100_PERCENT )
            return a;

         fc::uint128 r(a.value);
         r *= p;
         r /= GRAPHENE_100_PERCENT;
         return r.to_uint64();
      }

      void pay_out_fees(const account_object& account, share_type core_fee_total, bool require_vesting)
      {
         share_type network_cut = cut_fee(core_fee_total, account.network_fee_percentage);
         assert( network_cut <= core_fee_total );
         share_type burned = cut_fee(network_cut, props.parameters.burn_percent_of_fee);
         share_type accumulated = network_cut - burned;
         assert( accumulated + burned == network_cut );
         share_type lifetime_cut = cut_fee(core_fee_total, account.lifetime_referrer_fee_percentage);
         share_type referral = core_fee_total - network_cut - lifetime_cut;

         d.modify(dynamic_asset_data_id_type()(d), [network_cut](asset_dynamic_data_object& d) {
            d.accumulated_fees += network_cut;
         });

         // Potential optimization: Skip some of this math and object lookups by special casing on the account type.
         // For example, if the account is a lifetime member, we can skip all this and just deposit the referral to
         // it directly.
         share_type referrer_cut = cut_fee(referral, account.referrer_rewards_percentage);
         share_type registrar_cut = referral - referrer_cut;

         d.deposit_cashback(d.get(account.lifetime_referrer), lifetime_cut, require_vesting);
         d.deposit_cashback(d.get(account.referrer), referrer_cut, require_vesting);
         d.deposit_cashback(d.get(account.registrar), registrar_cut, require_vesting);

         assert( referrer_cut + registrar_cut + accumulated + burned + lifetime_cut == core_fee_total );
     }

      void operator()(const account_object& a) {
         const account_statistics_object& stats = a.statistics(d);

         if( stats.pending_fees > 0 )
         {
            share_type vesting_fee_subtotal(stats.pending_fees);
            share_type vested_fee_subtotal(stats.pending_vested_fees);
            share_type vesting_cashback, vested_cashback;

            if( stats.lifetime_fees_paid > props.parameters.bulk_discount_threshold_min &&
                a.is_member(d.head_block_time()) )
            {
               auto bulk_discount_rate = stats.calculate_bulk_discount_percent(props.parameters);
               vesting_cashback = cut_fee(vesting_fee_subtotal, bulk_discount_rate);
               vesting_fee_subtotal -= vesting_cashback;

               vested_cashback = cut_fee(vested_fee_subtotal, bulk_discount_rate);
               vested_fee_subtotal -= vested_cashback;
            }

            pay_out_fees(a, vesting_fee_subtotal, true);
            d.deposit_cashback(a, vesting_cashback, true);
            pay_out_fees(a, vested_fee_subtotal, false);
            d.deposit_cashback(a, vested_cashback, false);

            d.modify(stats, [vested_fee_subtotal, vesting_fee_subtotal](account_statistics_object& s) {
               s.lifetime_fees_paid += vested_fee_subtotal + vesting_fee_subtotal;
               s.pending_fees = 0;
               s.pending_vested_fees = 0;
            });
        }
      }
   } fee_helper(*this, gpo);

   perform_account_maintenance(std::tie(tally_helper, fee_helper));

   struct clear_canary {
      clear_canary(vector<uint64_t>& target): target(target){}
      ~clear_canary() { target.clear(); }
   private:
      vector<uint64_t>& target;
   };
   clear_canary a(_witness_count_histogram_buffer),
                b(_committee_count_histogram_buffer),
                c(_vote_tally_buffer);

   update_active_witnesses();
   update_active_delegates();

   if( gpo.pending_parameters )
      modify(gpo, [](global_property_object& p) {
         p.parameters = std::move(*p.pending_parameters);
         p.pending_parameters.reset();
      });

   auto new_block_interval = global_props.parameters.block_interval;

   // if block interval CHANGED during this block *THEN* we cannot simply
   // add the interval if we want to maintain the invariant that all timestamps are a multiple
   // of the interval.
   _pending_block.timestamp = next_block.timestamp + fc::seconds(new_block_interval);
   uint32_t r = _pending_block.timestamp.sec_since_epoch()%new_block_interval;
   if( !r )
   {
      _pending_block.timestamp -=  r;
      assert( (_pending_block.timestamp.sec_since_epoch() % new_block_interval)  == 0 );
   }

   auto next_maintenance_time = get<dynamic_global_property_object>(dynamic_global_property_id_type()).next_maintenance_time;
   auto maintenance_interval = gpo.parameters.maintenance_interval;

   if( next_maintenance_time <= next_block.timestamp )
   {
      if( next_block.block_num() == 1 )
         next_maintenance_time = time_point_sec() +
               (((next_block.timestamp.sec_since_epoch() / maintenance_interval) + 1) * maintenance_interval);
      else
         next_maintenance_time += maintenance_interval;
      assert( next_maintenance_time > next_block.timestamp );
   }

   modify(get_dynamic_global_properties(), [next_maintenance_time](dynamic_global_property_object& d) {
      d.next_maintenance_time = next_maintenance_time;
   });

   // Reset all BitAsset force settlement volumes to zero
   for( const asset_bitasset_data_object* d : get_index_type<asset_bitasset_data_index>() )
      modify(*d, [](asset_bitasset_data_object& d) { d.force_settled_volume = 0; });

   // process_budget needs to run at the bottom because
   //   it needs to know the next_maintenance_time
   process_budget();
}

} }
