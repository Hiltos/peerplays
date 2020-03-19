#pragma once

#include <graphene/peerplays_sidechain/sidechain_net_handler.hpp>

#include <string>

#include <fc/signals.hpp>

namespace graphene { namespace peerplays_sidechain {

class sidechain_net_handler_peerplays : public sidechain_net_handler {
public:
   sidechain_net_handler_peerplays(peerplays_sidechain_plugin &_plugin, const boost::program_options::variables_map &options);
   virtual ~sidechain_net_handler_peerplays();

   std::string recreate_primary_wallet();
   std::string process_deposit(const son_wallet_deposit_object &swdo);
   std::string process_withdrawal(const son_wallet_withdraw_object &swwo);
   std::string process_sidechain_transaction(const sidechain_transaction_object &sto, bool &complete);
   bool send_sidechain_transaction(const sidechain_transaction_object &sto);

private:
   void on_applied_block(const signed_block &b);
};

}} // namespace graphene::peerplays_sidechain
