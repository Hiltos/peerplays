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
#include <graphene/chain/limit_order_object.hpp>
#include <graphene/chain/short_order_object.hpp>

#include <fc/uint128.hpp>

namespace graphene { namespace chain {

/**
    for each short order, fill it at settlement price and place funds received into a total
    calculate the USD->CORE price and convert all USD balances to CORE at that price and subtract CORE from total
       - any fees accumulated by the issuer in the bitasset are forfeit / not redeemed
       - cancel all open orders with bitasset in it
       - any bitassets that use this bitasset as collateral are immediately settled at their feed price
       - convert all balances in bitasset to CORE and subtract from total
       - any prediction markets with usd as the backing get converted to CORE as the backing
    any CORE left over due to rounding errors is paid to accumulated fees
*/
void database::globally_settle_asset( const asset_object& mia, const price& settlement_price )
{ try {
   elog( "BLACK SWAN!" );
   debug_dump();

   edump( (mia.symbol)(settlement_price) );

   const asset_bitasset_data_object& bitasset = mia.bitasset_data(*this);
   const asset_object& backing_asset = bitasset.options.short_backing_asset(*this);
   asset collateral_gathered = backing_asset.amount(0);

   const asset_dynamic_data_object& mia_dyn = mia.dynamic_asset_data_id(*this);
   auto original_mia_supply = mia_dyn.current_supply;

   const call_order_index& call_index = get_index_type<call_order_index>();
   const auto& call_price_index = call_index.indices().get<by_price>();

    auto call_itr = call_price_index.lower_bound( price::min( bitasset.options.short_backing_asset, mia.id ) );
    auto call_end = call_price_index.upper_bound( price::max( bitasset.options.short_backing_asset, mia.id ) );
    while( call_itr != call_end )
    {
       auto pays = call_itr->get_debt() * settlement_price;
       wdump( (call_itr->get_debt() ) );
       collateral_gathered += pays;
       const auto&  order = *call_itr;
       ++call_itr;
       FC_ASSERT( fill_order( order, pays, order.get_debt() ) );
    }

   const limit_order_index& limit_index = get_index_type<limit_order_index>();
   const auto& limit_price_index = limit_index.indices().get<by_price>();

    // cancel all orders selling the market issued asset
    auto limit_itr = limit_price_index.lower_bound(price::max(mia.id, bitasset.options.short_backing_asset));
    auto limit_end = limit_price_index.upper_bound(~bitasset.current_feed.call_limit);
    while( limit_itr != limit_end )
    {
       const auto& order = *limit_itr;
       ilog( "CANCEL LIMIT ORDER" );
        idump((order));
       ++limit_itr;
       cancel_order( order );
    }

    limit_itr = limit_price_index.begin();
    while( limit_itr != limit_end )
    {
       if( limit_itr->amount_for_sale().asset_id == mia.id )
       {
          const auto& order = *limit_itr;
          ilog( "CANCEL_AGAIN" );
          edump((order));
          ++limit_itr;
          cancel_order( order );
       }
    }

   limit_itr = limit_price_index.begin();
   while( limit_itr != limit_end )
   {
      if( limit_itr->amount_for_sale().asset_id == mia.id )
      {
         const auto& order = *limit_itr;
         edump((order));
         ++limit_itr;
         cancel_order( order );
      }
   }

    // settle all balances
    asset total_mia_settled = mia.amount(0);

    const auto& index = get_index_type<account_balance_index>().indices().get<by_asset>();
    auto range = index.equal_range(mia.get_id());
    for( auto itr = range.first; itr != range.second; ++itr )
    {
       auto mia_balance = itr->get_balance();
       if( mia_balance.amount > 0 )
       {
          adjust_balance(itr->owner, -mia_balance);
          auto settled_amount = mia_balance * settlement_price;
          idump( (mia_balance)(settled_amount)(settlement_price) );
          adjust_balance(itr->owner, settled_amount);
          total_mia_settled += mia_balance;
          collateral_gathered -= settled_amount;
       }
    }

    // TODO: convert payments held in escrow

    modify( mia_dyn, [&]( asset_dynamic_data_object& obj ){
       total_mia_settled.amount += obj.accumulated_fees;
       obj.accumulated_fees = 0;
    });

    wlog( "====================== AFTER SETTLE BLACK SWAN UNCLAIMED SETTLEMENT FUNDS ==============\n" );
    wdump((collateral_gathered)(total_mia_settled)(original_mia_supply)(mia_dyn.current_supply));
    modify( bitasset.options.short_backing_asset(*this).dynamic_asset_data_id(*this), [&]( asset_dynamic_data_object& obj ){
       obj.accumulated_fees += collateral_gathered.amount;
    });

    FC_ASSERT( total_mia_settled.amount == original_mia_supply, "", ("total_settled",total_mia_settled)("original",original_mia_supply) );
   } FC_CAPTURE_AND_RETHROW( (mia)(settlement_price) ) }

void database::cancel_order(const force_settlement_object& order, bool create_virtual_op)
{
   adjust_balance(order.owner, order.balance);
   remove(order);

   if( create_virtual_op )
   {
      // TODO: create virtual op
   }
}

void database::cancel_order( const limit_order_object& order, bool create_virtual_op  )
{
   auto refunded = order.amount_for_sale();

   modify( order.seller(*this).statistics(*this),[&]( account_statistics_object& obj ){
      if( refunded.asset_id == asset_id_type() )
         obj.total_core_in_orders -= refunded.amount;
   });
   adjust_balance(order.seller, refunded);

   if( create_virtual_op )
   {
      // TODO: create a virtual cancel operation
   }

   remove( order );
}

/**
 *  Matches the two orders,
 *
 *  @return a bit field indicating which orders were filled (and thus removed)
 *
 *  0 - no orders were matched
 *  1 - bid was filled
 *  2 - ask was filled
 *  3 - both were filled
 */
template<typename OrderType>
int database::match( const limit_order_object& usd, const OrderType& core, const price& match_price )
{
   assert( usd.sell_price.quote.asset_id == core.sell_price.base.asset_id );
   assert( usd.sell_price.base.asset_id  == core.sell_price.quote.asset_id );
   assert( usd.for_sale > 0 && core.for_sale > 0 );

   auto usd_for_sale = usd.amount_for_sale();
   auto core_for_sale = core.amount_for_sale();

   asset usd_pays, usd_receives, core_pays, core_receives;

   if( usd_for_sale <= core_for_sale * match_price )
   {
      core_receives = usd_for_sale;
      usd_receives  = usd_for_sale * match_price;
   }
   else
   {
      //This line once read: assert( core_for_sale < usd_for_sale * match_price );
      //This assert is not always true -- see trade_amount_equals_zero in operation_tests.cpp
      //Although usd_for_sale is greater than core_for_sale * match_price, core_for_sale == usd_for_sale * match_price
      //Removing the assert seems to be safe -- apparently no asset is created or destroyed.
      usd_receives = core_for_sale;
      core_receives = core_for_sale * match_price;
   }

   core_pays = usd_receives;
   usd_pays  = core_receives;

   assert( usd_pays == usd.amount_for_sale() ||
           core_pays == core.amount_for_sale() );

   int result = 0;
   result |= fill_order( usd, usd_pays, usd_receives );
   result |= fill_order( core, core_pays, core_receives ) << 1;
   assert( result != 0 );
   return result;
}

int database::match( const limit_order_object& bid, const limit_order_object& ask, const price& match_price )
{
   return match<limit_order_object>( bid, ask, match_price );
}

int database::match( const limit_order_object& bid, const short_order_object& ask, const price& match_price )
{
   return match<short_order_object>( bid, ask, match_price );
}

asset database::match( const call_order_object& call, const force_settlement_object& settle, const price& match_price,
                     asset max_settlement )
{
   assert(call.get_debt().asset_id == settle.balance.asset_id );
   assert(call.debt > 0 && call.collateral > 0 && settle.balance.amount > 0);

   auto settle_for_sale = std::min(settle.balance, max_settlement);
   auto call_debt = call.get_debt();

   asset call_receives = std::min(settle_for_sale, call_debt),
         call_pays = call_receives * match_price,
         settle_pays = call_receives,
         settle_receives = call_pays;

   assert( settle_pays == settle_for_sale || call_receives == call.get_debt() );

   fill_order(call, call_pays, call_receives);
   fill_order(settle, settle_pays, settle_receives);

   return call_receives;
}

bool database::fill_order( const limit_order_object& order, const asset& pays, const asset& receives )
{
   assert( order.amount_for_sale().asset_id == pays.asset_id );
   assert( pays.asset_id != receives.asset_id );

   const account_object& seller = order.seller(*this);
   const asset_object& recv_asset = receives.asset_id(*this);

   auto issuer_fees = pay_market_fees( recv_asset, receives );
   pay_order( seller, receives - issuer_fees, pays );

   push_applied_operation( fill_order_operation{ order.id, order.seller, pays, receives, issuer_fees } );

   if( pays == order.amount_for_sale() )
   {
      remove( order );
      return true;
   }
   else
   {
      modify( order, [&]( limit_order_object& b ) {
                             b.for_sale -= pays.amount;
                          });
      /**
       *  There are times when the AMOUNT_FOR_SALE * SALE_PRICE == 0 which means that we
       *  have hit the limit where the seller is asking for nothing in return.  When this
       *  happens we must refund any balance back to the seller, it is too small to be
       *  sold at the sale price.
       */
      if( order.amount_to_receive().amount == 0 )
      {
         cancel_order(order);
         return true;
      }
      return false;
   }
}

bool database::fill_order( const short_order_object& order, const asset& pays, const asset& receives )
{ try {
   assert( order.amount_for_sale().asset_id == pays.asset_id );
   assert( pays.asset_id != receives.asset_id );

   const call_order_index& call_index = get_index_type<call_order_index>();

   const account_object& seller = order.seller(*this);
   const asset_object& recv_asset = receives.asset_id(*this);
   const asset_object& pays_asset = pays.asset_id(*this);
   assert( pays_asset.is_market_issued() );

   auto issuer_fees = pay_market_fees( recv_asset, receives );

   bool filled               = pays == order.amount_for_sale();
   asset seller_to_collateral;
   if( (*pays_asset.bitasset_data_id)(*this).is_prediction_market )
   {
      assert( pays.amount >= receives.amount );
      seller_to_collateral = pays.amount - receives.amount;
   }
   else
   {
      seller_to_collateral = filled ? order.get_collateral() : pays * order.sell_price;
   }
   auto buyer_to_collateral  = receives - issuer_fees;

   if( receives.asset_id == asset_id_type() )
   {
      const auto& statistics = seller.statistics(*this);
      modify( statistics, [&]( account_statistics_object& b ){
             b.total_core_in_orders += buyer_to_collateral.amount;
      });
   }

   modify( pays_asset.dynamic_asset_data_id(*this), [&]( asset_dynamic_data_object& obj ){
      obj.current_supply += pays.amount;
   });

   const auto& call_account_index = call_index.indices().get<by_account>();
   auto call_itr = call_account_index.find(  boost::make_tuple(order.seller, pays.asset_id) );
   if( call_itr == call_account_index.end() )
   {
      create<call_order_object>( [&]( call_order_object& c ){
         c.borrower    = seller.id;
         c.collateral  = seller_to_collateral.amount + buyer_to_collateral.amount;
         c.debt        = pays.amount;
         c.maintenance_collateral_ratio = order.maintenance_collateral_ratio;
         c.call_price  = price::max(seller_to_collateral.asset_id, pays.asset_id);
         c.update_call_price();
      });
   }
   else
   {
      modify( *call_itr, [&]( call_order_object& c ){
         c.debt       += pays.amount;
         c.collateral += seller_to_collateral.amount + buyer_to_collateral.amount;
         c.maintenance_collateral_ratio = order.maintenance_collateral_ratio;
         c.update_call_price();
      });
   }

   if( filled )
   {
      remove( order );
   }
   else
   {
      modify( order, [&]( short_order_object& b ) {
         b.for_sale -= pays.amount;
         b.available_collateral -= seller_to_collateral.amount;
         assert( b.available_collateral > 0 );
         assert( b.for_sale > 0 );
      });

      /**
       *  There are times when the AMOUNT_FOR_SALE * SALE_PRICE == 0 which means that we
       *  have hit the limit where the seller is asking for nothing in return.  When this
       *  happens we must refund any balance back to the seller, it is too small to be
       *  sold at the sale price.
       */
      if( order.amount_to_receive().amount == 0 )
      {
         adjust_balance(seller.get_id(), order.get_collateral());
         if( order.get_collateral().asset_id == asset_id_type() )
         {
            const auto& statistics = seller.statistics(*this);
            modify( statistics, [&]( account_statistics_object& b ){
                 b.total_core_in_orders -= order.available_collateral;
            });
         }

         remove( order );
         filled = true;
      }
   }

   push_applied_operation( fill_order_operation{ order.id, order.seller, pays, receives, issuer_fees } );

   return filled;
} FC_CAPTURE_AND_RETHROW( (order)(pays)(receives) ) }

bool database::fill_order( const call_order_object& order, const asset& pays, const asset& receives )
{ try {
   idump((pays)(receives)(order));
   assert( order.get_debt().asset_id == receives.asset_id );
   assert( order.get_collateral().asset_id == pays.asset_id );
   assert( order.get_collateral() >= pays );

   optional<asset> collateral_freed;
   modify( order, [&]( call_order_object& o ){
            o.debt       -= receives.amount;
            o.collateral -= pays.amount;
            if( o.debt == 0 )
            {
              collateral_freed = o.get_collateral();
              o.collateral = 0;
            }
       });
   const asset_object& mia = receives.asset_id(*this);
   assert( mia.is_market_issued() );

   const asset_dynamic_data_object& mia_ddo = mia.dynamic_asset_data_id(*this);

   modify( mia_ddo, [&]( asset_dynamic_data_object& ao ){
       idump((receives));
        ao.current_supply -= receives.amount;
      });

   const account_object& borrower = order.borrower(*this);
   if( collateral_freed || pays.asset_id == asset_id_type() )
   {
      const account_statistics_object& borrower_statistics = borrower.statistics(*this);
      if( collateral_freed )
         adjust_balance(borrower.get_id(), *collateral_freed);
      modify( borrower_statistics, [&]( account_statistics_object& b ){
              if( collateral_freed && collateral_freed->amount > 0 )
                b.total_core_in_orders -= collateral_freed->amount;
              if( pays.asset_id == asset_id_type() )
                b.total_core_in_orders -= pays.amount;
              assert( b.total_core_in_orders >= 0 );
           });
   }

   if( collateral_freed )
   {
      remove( order );
   }

   push_applied_operation( fill_order_operation{ order.id, order.borrower, pays, receives, asset(0, pays.asset_id) } );

   return collateral_freed.valid();
} FC_CAPTURE_AND_RETHROW( (order)(pays)(receives) ) }

bool database::fill_order(const force_settlement_object& settle, const asset& pays, const asset& receives)
{ try {
   bool filled = false;

   auto issuer_fees = pay_market_fees(get(receives.asset_id), receives);

   if( pays < settle.balance )
   {
      modify(settle, [&pays](force_settlement_object& s) {
         s.balance -= pays;
      });
      filled = false;
   } else {
      remove(settle);
      filled = true;
   }
   adjust_balance(settle.owner, receives - issuer_fees);

   push_applied_operation( fill_order_operation{ settle.id, settle.owner, pays, receives, issuer_fees } );

   return filled;
} FC_CAPTURE_AND_RETHROW( (settle)(pays)(receives) ) }

/**
 *
 */
bool database::check_call_orders( const asset_object& mia )
{ try {
    if( !mia.is_market_issued() ) return false;
    const asset_bitasset_data_object& bitasset = mia.bitasset_data(*this);
    if( bitasset.current_feed.call_limit.is_null() ) return false;
    if( bitasset.is_prediction_market ) return false;

    const call_order_index& call_index = get_index_type<call_order_index>();
    const auto& call_price_index = call_index.indices().get<by_price>();

    const limit_order_index& limit_index = get_index_type<limit_order_index>();
    const auto& limit_price_index = limit_index.indices().get<by_price>();

    const short_order_index& short_index = get_index_type<short_order_index>();
    const auto& short_price_index = short_index.indices().get<by_price>();

    auto short_itr = short_price_index.lower_bound( price::max( mia.id, bitasset.options.short_backing_asset ) );
    auto short_end = short_price_index.upper_bound( ~bitasset.current_feed.call_limit );

    auto limit_itr = limit_price_index.lower_bound( price::max( mia.id, bitasset.options.short_backing_asset ) );
    auto limit_end = limit_price_index.upper_bound( ~bitasset.current_feed.call_limit );

    auto call_itr = call_price_index.lower_bound( price::min( bitasset.options.short_backing_asset, mia.id ) );
    auto call_end = call_price_index.upper_bound( price::max( bitasset.options.short_backing_asset, mia.id ) );

    bool filled_short_or_limit = false;

    while( call_itr != call_end )
    {
       bool  current_is_limit = true;
       bool  filled_call      = false;
       price match_price;
       asset usd_for_sale;
       if( limit_itr != limit_end )
       {
          assert( limit_itr != limit_price_index.end() );
          if( short_itr != short_end && limit_itr->sell_price < short_itr->sell_price )
          {
             assert( short_itr != short_price_index.end() );
             current_is_limit = false;
             match_price      = short_itr->sell_price;
             usd_for_sale     = short_itr->amount_for_sale();
          }
          else
          {
             current_is_limit = true;
             match_price      = limit_itr->sell_price;
             usd_for_sale     = limit_itr->amount_for_sale();
          }
       }
       else if( short_itr != short_end )
       {
          assert( short_itr != short_price_index.end() );
          current_is_limit = false;
          match_price      = short_itr->sell_price;
          usd_for_sale     = short_itr->amount_for_sale();
       }
       else return filled_short_or_limit;

       match_price.validate();

       if( match_price > ~call_itr->call_price )
       {
          return filled_short_or_limit;
       }

       auto usd_to_buy   = call_itr->get_debt();

       if( usd_to_buy * match_price > call_itr->get_collateral() )
       {
          elog( "black swan, we do not have enough collateral to cover at this price" );
          globally_settle_asset( mia, call_itr->get_debt() / call_itr->get_collateral() );
          return true;
       }

       asset call_pays, call_receives, order_pays, order_receives;
       if( usd_to_buy >= usd_for_sale )
       {  // fill order
          call_receives   = usd_for_sale;
          order_receives  = usd_for_sale * match_price;
          call_pays       = order_receives;
          order_pays      = usd_for_sale;

          filled_short_or_limit = true;
          filled_call           = (usd_to_buy == usd_for_sale);
       }
       else // fill call
       {
          call_receives  = usd_to_buy;
          order_receives = usd_to_buy * match_price;
          call_pays      = order_receives;
          order_pays     = usd_to_buy;

          filled_call    = true;
       }

       auto old_call_itr = call_itr;
       if( filled_call ) ++call_itr;
       fill_order( *old_call_itr, call_pays, call_receives );
       if( current_is_limit )
       {
          auto old_limit_itr = !filled_call ? limit_itr++ : limit_itr;
          fill_order( *old_limit_itr, order_pays, order_receives );
       }
       else
       {
          auto old_short_itr = !filled_call ? short_itr++ : short_itr;
          fill_order( *old_short_itr, order_pays, order_receives );
       }
    } // whlie call_itr != call_end

    return filled_short_or_limit;
} FC_CAPTURE_AND_RETHROW() }

void database::pay_order( const account_object& receiver, const asset& receives, const asset& pays )
{
   const auto& balances = receiver.statistics(*this);
   modify( balances, [&]( account_statistics_object& b ){
         if( pays.asset_id == asset_id_type() )
            b.total_core_in_orders -= pays.amount;
   });
   adjust_balance(receiver.get_id(), receives);
}

/**
 *  For Market Issued assets Managed by Delegates, any fees collected in the MIA need
 *  to be sold and converted into CORE by accepting the best offer on the table.
 */
bool database::convert_fees( const asset_object& mia )
{
   if( mia.issuer != account_id_type() ) return false;
   return false;
}

asset database::calculate_market_fee( const asset_object& trade_asset, const asset& trade_amount )
{
   assert( trade_asset.id == trade_amount.asset_id );

   if( !trade_asset.charges_market_fees() )
      return trade_asset.amount(0);
   if( trade_asset.options.market_fee_percent == 0 )
      return trade_asset.amount(trade_asset.options.min_market_fee);

   fc::uint128 a(trade_amount.amount.value);
   a *= trade_asset.options.market_fee_percent;
   a /= GRAPHENE_100_PERCENT;
   asset percent_fee = trade_asset.amount(a.to_uint64());

   if( percent_fee.amount > trade_asset.options.max_market_fee )
      percent_fee.amount = trade_asset.options.max_market_fee;
   else if( percent_fee.amount < trade_asset.options.min_market_fee )
      percent_fee.amount = trade_asset.options.min_market_fee;

   return percent_fee;
}

asset database::pay_market_fees( const asset_object& recv_asset, const asset& receives )
{
   auto issuer_fees = calculate_market_fee( recv_asset, receives );
   assert(issuer_fees <= receives );

   //Don't dirty undo state if not actually collecting any fees
   if( issuer_fees.amount > 0 )
   {
      const auto& recv_dyn_data = recv_asset.dynamic_asset_data_id(*this);
      modify( recv_dyn_data, [&]( asset_dynamic_data_object& obj ){
                   idump((issuer_fees));
         obj.accumulated_fees += issuer_fees.amount;
      });
   }

   return issuer_fees;
}

} }
