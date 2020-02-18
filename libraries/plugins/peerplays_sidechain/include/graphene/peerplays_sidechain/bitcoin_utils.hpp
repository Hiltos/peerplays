#pragma once
#include <graphene/peerplays_sidechain/defs.hpp>
#include <fc/optional.hpp>

namespace graphene { namespace peerplays_sidechain {

enum bitcoin_network {
   mainnet,
   testnet,
   regtest
};

bytes generate_redeem_script(std::vector<std::pair<fc::ecc::public_key, int> > key_data);
std::string p2sh_address_from_redeem_script(const bytes& script, bitcoin_network network = mainnet);
bytes lock_script_for_redeem_script(const bytes& script);
/*
 * unsigned_tx - tx,  all inputs of which are spends of the PW P2SH address
 * returns signed transaction
 */
bytes sign_pw_transfer_transaction(const bytes& unsigned_tx, const bytes& redeem_script, const std::vector<fc::optional<fc::ecc::private_key>>& priv_keys);

struct btc_outpoint
{
   fc::uint256 hash;
   uint32_t n;

   void to_bytes(bytes& stream) const;
   size_t fill_from_bytes(const bytes& data, size_t pos = 0);
};

struct btc_in
{
   btc_outpoint prevout;
   bytes scriptSig;
   uint32_t nSequence;
   bytes scriptWitness;

   void to_bytes(bytes& stream) const;
   size_t fill_from_bytes(const bytes& data, size_t pos = 0);
};

struct btc_out
{
   int64_t nValue;
   bytes scriptPubKey;

   void to_bytes(bytes& stream) const;
   size_t fill_from_bytes(const bytes& data, size_t pos = 0);
};

struct btc_tx
{
   std::vector<btc_in> vin;
   std::vector<btc_out> vout;
   int32_t nVersion;
   uint32_t nLockTime;
   bool hasWitness;

   void to_bytes(bytes& stream) const;
   size_t fill_from_bytes(const bytes& data, size_t pos = 0);
};

}}

