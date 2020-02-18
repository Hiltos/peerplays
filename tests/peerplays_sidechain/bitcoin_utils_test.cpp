#include <boost/test/unit_test.hpp>
#include <graphene/peerplays_sidechain/bitcoin_utils.hpp>
#include <fc/crypto/hex.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/crypto/base58.hpp>
#include <secp256k1.h>

using namespace graphene::peerplays_sidechain;

BOOST_AUTO_TEST_CASE(tx_serialization)
{
   // use real mainnet transaction
   // txid: 6189e3febb5a21cee8b725aa1ef04ffce7e609448446d3a8d6f483c634ef5315
   // json: {"txid":"6189e3febb5a21cee8b725aa1ef04ffce7e609448446d3a8d6f483c634ef5315","hash":"6189e3febb5a21cee8b725aa1ef04ffce7e609448446d3a8d6f483c634ef5315","version":1,"size":224,"vsize":224,"weight":896,"locktime":0,"vin":[{"txid":"55d079ca797fee81416b71b373abedd8722e33c9f73177be0166b5d5fdac478b","vout":0,"scriptSig":{"asm":"3045022100d82e57d4d11d3b811d07f2fa4ded2fb8a3b7bb1d3e9f293433de5c0d1093c3bd02206704ccd2ff437e2f7716b5e9f2502a9cbb41f1245a18b2b10296980f1ae38253[ALL] 02be9919a5ba373b1af58ad757db19e7c836116bb8138e0c6d99599e4db96568f4","hex":"483045022100d82e57d4d11d3b811d07f2fa4ded2fb8a3b7bb1d3e9f293433de5c0d1093c3bd02206704ccd2ff437e2f7716b5e9f2502a9cbb41f1245a18b2b10296980f1ae38253012102be9919a5ba373b1af58ad757db19e7c836116bb8138e0c6d99599e4db96568f4"},"sequence":4294967295}],"vout":[{"value":1.26491535,"n":0,"scriptPubKey":{"asm":"OP_DUP OP_HASH160 95783804d28e528fbc4b48c7700471e6845804eb OP_EQUALVERIFY OP_CHECKSIG","hex":"76a91495783804d28e528fbc4b48c7700471e6845804eb88ac","reqSigs":1,"type":"pubkeyhash","addresses":["1EdKhXv7zjGowPzgDQ4z1wa2ukVrXRXXkP"]}},{"value":0.0002,"n":1,"scriptPubKey":{"asm":"OP_HASH160 fb0670971091da8248b5c900c6515727a20e8662 OP_EQUAL","hex":"a914fb0670971091da8248b5c900c6515727a20e866287","reqSigs":1,"type":"scripthash","addresses":["3QaKF8zobqcqY8aS6nxCD5ZYdiRfL3RCmU"]}}]}
   // hex: "01000000018b47acfdd5b56601be7731f7c9332e72d8edab73b3716b4181ee7f79ca79d055000000006b483045022100d82e57d4d11d3b811d07f2fa4ded2fb8a3b7bb1d3e9f293433de5c0d1093c3bd02206704ccd2ff437e2f7716b5e9f2502a9cbb41f1245a18b2b10296980f1ae38253012102be9919a5ba373b1af58ad757db19e7c836116bb8138e0c6d99599e4db96568f4ffffffff028f1b8a07000000001976a91495783804d28e528fbc4b48c7700471e6845804eb88ac204e00000000000017a914fb0670971091da8248b5c900c6515727a20e86628700000000"
   fc::string strtx("01000000018b47acfdd5b56601be7731f7c9332e72d8edab73b3716b4181ee7f79ca79d055000000006b483045022100d82e57d4d11d3b811d07f2fa4ded2fb8a3b7bb1d3e9f293433de5c0d1093c3bd02206704ccd2ff437e2f7716b5e9f2502a9cbb41f1245a18b2b10296980f1ae38253012102be9919a5ba373b1af58ad757db19e7c836116bb8138e0c6d99599e4db96568f4ffffffff028f1b8a07000000001976a91495783804d28e528fbc4b48c7700471e6845804eb88ac204e00000000000017a914fb0670971091da8248b5c900c6515727a20e86628700000000");
   bytes bintx;
   bintx.resize(strtx.length() / 2);
   fc::from_hex(strtx, reinterpret_cast<char*>(&bintx[0]), bintx.size());
   btc_tx tx;
   BOOST_CHECK_NO_THROW(tx.fill_from_bytes(bintx));
   BOOST_CHECK(tx.nVersion == 1);
   BOOST_CHECK(tx.nLockTime == 0);
   BOOST_CHECK(tx.vin.size() == 1);
   BOOST_CHECK(tx.vout.size() == 2);
   bytes buff;
   tx.to_bytes(buff);
   BOOST_CHECK(bintx == buff);
}

BOOST_AUTO_TEST_CASE(pw_transfer)
{
   // key set for the old Primary Wallet
   std::vector<fc::ecc::private_key> priv_old;
   for(unsigned i = 0; i < 15; ++i)
   {
      const char* seed = reinterpret_cast<const char*>(&i);
      fc::sha256 h = fc::sha256::hash(seed, sizeof(i));
      priv_old.push_back(fc::ecc::private_key::generate_from_seed(h));
   }
   // print old keys
   for(auto key: priv_old)
   {
      fc::sha256 secret = key.get_secret();
      bytes data({239});
      data.insert(data.end(), secret.data(), secret.data() + secret.data_size());
      fc::sha256 cs = fc::sha256::hash(fc::sha256::hash((char*)&data[0], data.size()));
      data.insert(data.end(), cs.data(), cs.data() + 4);
   }
   std::vector<fc::ecc::public_key> pub_old;
   for(auto& key: priv_old)
      pub_old.push_back(key.get_public_key());
   // old key weights
   std::vector<std::pair<fc::ecc::public_key, int> > weights_old;
   for(unsigned i = 0; i < 15; ++i)
      weights_old.push_back(std::make_pair(pub_old[i], i + 1));
   // redeem script for old PW
   bytes redeem_old =generate_redeem_script(weights_old);

   // Old PW address
   std::string old_pw = p2wsh_address_from_redeem_script(redeem_old, bitcoin_network::testnet);
   // This address was filled with testnet transaction 508a36d65de66db7c57ee6c5502068ebdcba996ca2df23ef42d901ec8fba1766
   BOOST_REQUIRE(old_pw == "tb1qfhstznulf5cmjzahlkmnuuvs0tkjtwjlme3ugz8jzfjanf8h5rwsp45t7e");

   bytes scriptPubKey = lock_script_for_redeem_script(redeem_old);

   // key set for the new Primary Wallet
   std::vector<fc::ecc::private_key> priv_new;
   for(unsigned i = 16; i < 31; ++i)
   {
      const char* seed = reinterpret_cast<const char*>(&i);
      fc::sha256 h = fc::sha256::hash(seed, sizeof(i));
      priv_new.push_back(fc::ecc::private_key::generate_from_seed(h));
   }
   std::vector<fc::ecc::public_key> pub_new;
   for(auto& key: priv_new)
      pub_new.push_back(key.get_public_key());
   // new key weights
   std::vector<std::pair<fc::ecc::public_key, int> > weights_new;
   for(unsigned i = 0; i < 15; ++i)
      weights_new.push_back(std::make_pair(pub_new[i], 16 - i));
   // redeem script for new PW
   bytes redeem_new =generate_redeem_script(weights_new);
   // New PW address
   std::string new_pw = p2wsh_address_from_redeem_script(redeem_new, bitcoin_network::testnet);
   BOOST_REQUIRE(new_pw == "tb1qzegrz8r8z8ddfkql8595d90czng6eyjmx4ur73ls4pq57jg99qhsh9fd2y");

   // try to move funds from old wallet to new one

   // get unspent outputs for old wallet with list_uspent (address should be
   // added to wallet with import_address before). It should return
   // 1 UTXO: [508a36d65de66db7c57ee6c5502068ebdcba996ca2df23ef42d901ec8fba1766:0]
   // with 20000 satoshis
   // So, we creating a raw transaction with 1 input and one output that gets
   // 20000 - fee satoshis with createrawtransaction call (bitcoin_rpc_client::prepare_tx)
   // Here we just serialize the transaction without scriptSig in inputs then sign it.
   btc_outpoint outpoint;
   outpoint.hash = fc::uint256("508a36d65de66db7c57ee6c5502068ebdcba996ca2df23ef42d901ec8fba1766");
   // reverse hash due to the different from_hex algo
   std::reverse(outpoint.hash.data(), outpoint.hash.data() + outpoint.hash.data_size());
   outpoint.n = 0;
   btc_in input;
   input.prevout = outpoint;
   input.nSequence = 0xffffffff;
   btc_out output;
   output.nValue = 19000;
   output.scriptPubKey = lock_script_for_redeem_script(redeem_new);
   btc_tx tx;
   tx.nVersion = 2;
   tx.nLockTime = 0;
   tx.hasWitness = false;
   tx.vin.push_back(input);
   tx.vout.push_back(output);
   bytes unsigned_tx;
   tx.to_bytes(unsigned_tx);
   std::vector<uint64_t> in_amounts({20000});
   std::vector<fc::optional<fc::ecc::private_key>> keys_to_sign;
   for(auto key: priv_old)
      keys_to_sign.push_back(fc::optional<fc::ecc::private_key>(key));
   bytes signed_tx =sign_pw_transfer_transaction(unsigned_tx, in_amounts, redeem_old, keys_to_sign);
   // this is real testnet tx with id 1734a2f6192c3953c90f9fd7f69eba16eeb0922207f81f3af32d6534a6f8e850
   BOOST_CHECK(fc::to_hex((char*)&signed_tx[0], signed_tx.size()) == "020000000001016617ba8fec01d942ef23dfa26c99badceb682050c5e67ec5b76de65dd6368a500000000000ffffffff01384a0000000000002200201650311c6711dad4d81f3d0b4695f814d1ac925b35783f47f0a8414f4905282f10473044022028cf6df7ed5c2761d7aa2af20717c8b5ace168a7800d6a566f2c1ae28160cae502205e01a3d91f5b9870577e36fbc26ce0cecc3e628cc376c7016364ec3f370703140147304402205c9a88cbe41eb9c6a16ba1d747456222cbe951d04739d21309ef0c0cf00727f202202d06db830ee5823882c7b6f82b708111a8f37741878896cd3558fb91efe8076401473044022009c3184fc0385eb7ed8dc0374791cbdace0eff0dc27dd80ac68f8cb81110f700022042267e8a8788c314347234ea10db6c1ec21a2d423b784cbfbaadf3b2393c44630147304402202363ce306570dc0bbf6d18d41b67c6488a014a91d8e24c03670b4f65523aca12022029d04c114b8e93d982cadee89d80bb25c5c8bc437d6cd2bfce8e0d83a08d14410148304502210087b4742e5cf9c77ca9f99928e7c7087e7d786e09216485628509e4e0b2f29d7e02207daf2eaee9fe8bf117074be137b7ae4b8503a4f6d263424e8e6a16405d5b723c0147304402204f1c3ed8cf595bfaf79d90f4c55c04c17bb6d446e3b9beca7ee6ee7895c6b752022022ac032f219a81b2845d0a1abfb904e40036a3ad332e7dfada6fda21ef7080b501483045022100d020eca4ba1aa77de9caf98f3a29f74f55268276860b9fa35fa16cfc00219dd8022028237de6ad063116cf8182d2dd45a09cb90c2ec8104d793eb3635a1290027cd6014730440220322193b0feba7356651465b86463c7619cd3d96729df6242e9571c74ff1c3c2902206e1de8e77b71c7b6031a934b52321134b6a8d138e2124e90f6345decbd543efb01483045022100d70ade49b3f17812785a41711e107b27c3d4981f8e12253629c07ec46ee511af02203e1ea9059ed9165eeff827002c7399a30c478a9b6f2b958621bfbc6713ab4dd30147304402206f7f10d9993c7019360276bbe790ab587adadeab08088593a9a0c56524aca4df02207c147fe2e51484801a4e059e611e7514729d685a5df892dcf02ba59d455e678101483045022100d5071b8039364bfaa53ef5e22206f773539b082f28bd1fbaaea995fa28aae0f5022056edf7a7bdd8a9a54273a667be5bcd11191fc871798fb44f6e1e35c95d86a81201483045022100a39f8ffbcd9c3f0591fc731a9856c8e024041017cba20c9935f13e4abcf9e9dc0220786823b8cd55664ff9ad6277899aacfd56fa8e48c38881482418b7d50ca27211014730440220361d3b87fcc2b1c12a9e7c684c78192ccb7fe51b90c281b7058384b0b036927a0220434c9b403ee3802b4e5b53feb9bb37d2a9d8746c3688da993549dd9d9954c6800147304402206dc4c3a4407fe9cbffb724928aa0597148c14a20d0d7fbb36ad5d3e2a3abf85e022039ef7baebbf08494495a038b009c6d4ff4b91c38db840673b87f6c27c3b53e7e01483045022100cadac495ea78d0ce9678a4334b8c43f7fafeea5a59413cc2a0144addb63485f9022078ca133e020e3afd0e79936337afefc21d84d3839f5a225a0f3d3eebc15f959901fd5c02007c21030e88484f2bb5dcfc0b326e9eb565c27c8291efb064d060d226916857a2676e62ac635193687c2102151ad794a3aeb3cf9c190120da3d13d36cd8bdf21ca1ccb15debd61c601314b0ac635293687c2103b45a5955ea7847d121225c752edaeb4a5d731a056a951a876caaf6d1f69adb7dac635393687c2102def03a6ffade4ffb0017c8d93859a247badd60e2d76d00e2a3713f6621932ec1ac635493687c21035f17aa7d58b8c3ee0d87240fded52b27f3f12768a0a54ba2595e0a929dd87155ac635593687c2103c8582ac6b0bd20cc1b02c6a86bad2ea10cadb758fedd754ba0d97be85b63b5a7ac635693687c21028148a1f9669fc4471e76f7a371d7cc0563b26e0821d9633fd37649744ff54edaac635793687c2102f0313701b0035f0365a59ce1a3d7ae7045e1f2fb25c4656c08071e5baf51483dac635893687c21024c4c25d08173b3c4d4e1375f8107fd7040c2dc0691ae1bf6fe82b8c88a85185fac635993687c210360fe2daa8661a3d25d0df79875d70b1c3d443ade731caafda7488cb68b4071b0ac635a93687c210250e41a6a4abd7b0b3a49eaec24a6fafa99e5aa7b1e3a5aabe60664276df3d937ac635b93687c2103045a32125930ca103c7d7c79b6f379754796cd4ea7fb0059da926e415e3877d3ac635c93687c210344943249d7ca9b47316fef0c2a413dda3a75416a449a29f310ab7fc9d052ed70ac635d93687c2103c62967320b63df5136ff1ef4c7959ef5917ee5a44f75c83e870bc488143d4d69ac635e93687c21020429f776e15770e4dc52bd6f72e6ed6908d51de1c4a64878433c4e3860a48dc4ac635f93680150a000000000");
}

