#pragma once

#include <graphene/chain/protocol/types.hpp>
#include <sidechain/types.hpp>
#include <fc/crypto/sha256.hpp>

namespace graphene { namespace chain { class database; } }

namespace sidechain {

class primary_wallet_vout_manager
{

public:
   primary_wallet_vout_manager( graphene::chain::database& _db ) : db( _db ) {}

   bool is_reach_max_unconfirmaed_vout() const;

   fc::optional< graphene::chain::primary_wallet_vout_object > get_latest_unused_vout() const;

   fc::optional< graphene::chain::primary_wallet_vout_object > get_vout( fc::sha256 hash_id ) const;
   
   void create_new_vout( const sidechain::prev_out& out );

   void delete_vout_with_newer( fc::sha256 hash_id );

   void confirm_vout( fc::sha256 hash_id );

   void use_latest_vout( fc::sha256 hash_id );

private:

   fc::optional< graphene::chain::primary_wallet_vout_id_type > get_vout_id( fc::sha256 hash_id ) const;

   graphene::chain::database& db;

};

}