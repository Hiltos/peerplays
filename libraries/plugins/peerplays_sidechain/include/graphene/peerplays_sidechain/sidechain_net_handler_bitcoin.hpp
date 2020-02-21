#pragma once

#include <graphene/peerplays_sidechain/sidechain_net_handler.hpp>

#include <string>
#include <zmq.hpp>

#include <fc/signals.hpp>
#include <fc/network/http/connection.hpp>
#include <graphene/chain/database.hpp>

namespace graphene { namespace peerplays_sidechain {

class btc_txout
{
public:
   std::string txid_;
   unsigned int out_num_;
   double amount_;
};

class bitcoin_rpc_client {
public:
   bitcoin_rpc_client( std::string _ip, uint32_t _rpc, std::string _user, std::string _password) ;
   std::string receive_full_block( const std::string& block_hash );
   int32_t receive_confirmations_tx( const std::string& tx_hash );
   bool receive_mempool_entry_tx( const std::string& tx_hash );
   uint64_t receive_estimated_fee();
   void send_btc_tx( const std::string& tx_hex );
   std::string add_multisig_address( const std::vector<std::string> public_keys );
   bool connection_is_not_defined() const;
   std::string create_raw_transaction(const std::string& txid, const std::string& vout, const std::string& out_address, double transfer_amount);
   std::string sign_raw_transaction_with_wallet(const std::string& tx_hash);
   std::string sign_raw_transaction_with_privkey(const std::string& tx_hash, const std::string& private_key);
   void import_address( const std::string& address_or_script);
   std::vector<btc_txout> list_unspent();
   std::vector<btc_txout> list_unspent_by_address_and_amount(const std::string& address, double transfer_amount);
   std::string prepare_tx(const std::vector<btc_txout>& ins, const fc::flat_map<std::string, double> outs);

private:

   fc::http::reply send_post_request( std::string body );

   std::string ip;
   uint32_t rpc_port;
   std::string user;
   std::string password;

   fc::http::header authorization;
};

// =============================================================================

class zmq_listener {
public:
   zmq_listener( std::string _ip, uint32_t _zmq );
   bool connection_is_not_defined() const { return zmq_port == 0; }

   fc::signal<void( const std::string& )> event_received;
private:
   void handle_zmq();
   std::vector<zmq::message_t> receive_multipart();

   std::string ip;
   uint32_t zmq_port;

   zmq::context_t ctx;
   zmq::socket_t socket;
};

// =============================================================================

class sidechain_net_handler_bitcoin : public sidechain_net_handler {
public:
   sidechain_net_handler_bitcoin(peerplays_sidechain_plugin& _plugin, const boost::program_options::variables_map& options);
   virtual ~sidechain_net_handler_bitcoin();

   void recreate_primary_wallet();

   bool connection_is_not_defined() const;

   std::string create_multisignature_wallet( const std::vector<std::string> public_keys );
   std::string transfer( const std::string& from, const std::string& to, const uint64_t amount );
   std::string sign_transaction( const std::string& transaction );
   std::string send_transaction( const std::string& transaction );
   std::string sign_and_send_transaction_with_wallet ( const std::string& tx_json );
   std::string transfer_all_btc(const std::string& from_address, const std::string& to_address);
   std::string transfer_deposit_to_primary_wallet (const sidechain_event_data& sed);
   std::string transfer_withdrawal_from_primary_wallet(const std::string& user_address, double sidechain_amount);


private:
   std::string ip;
   uint32_t zmq_port;
   uint32_t rpc_port;
   std::string rpc_user;
   std::string rpc_password;
   std::map<std::string, std::string> _private_keys;

   std::unique_ptr<zmq_listener> listener;
   std::unique_ptr<bitcoin_rpc_client> bitcoin_client;

   void handle_event( const std::string& event_data);
   std::vector<info_for_vin> extract_info_from_block( const std::string& _block );
};

} } // graphene::peerplays_sidechain

