#include "id_generator.h"
#include  "key.h"
#include "base58.h"
#include "common.h"
#include "utilstrencodings.h"
#include "secp256k1.h"
#include "secp256k1_recovery.h"
#include "lax_der_privatekey_parsing.h"
namespace matrix
{
    namespace core
    {
        id_generator::id_generator()
        {

        }


        id_generator::~id_generator()
        {

        }

        int32_t id_generator::generate_node_info(node_info &info)
        {
            CKey secret;

            //private key
            bool fCompressed = true;
            secret.MakeNewKey(fCompressed);
            assert(secret.size() > 0);

            //public key
            CPubKey pubkey = secret.GetPubKey();

            //create compressed public key
            CKeyID keyID = pubkey.GetID();

            //encode node id
            CPrivKey private_key = secret.GetPrivKey();

            std::vector<unsigned char> id_data;
            std::vector<unsigned char> id_prefix = { 'n', 'o', 'd', 'e', '.', NODE_ID_VERSION, '.' };
            id_data.reserve(id_prefix.size() + keyID.size());
            id_data.insert(id_data.end(), id_prefix.begin(), id_prefix.end());
            id_data.insert(id_data.end(), keyID.begin(), keyID.end());
            info.node_id = EncodeBase58Check(id_data);

            //encode node private key
            std::vector<unsigned char> private_key_data;
            std::vector<unsigned char> private_key_prefix = { PRIVATE_KEY_VERSION, '.' };
            private_key_data.reserve(private_key_prefix.size() + private_key.size());
            private_key_data.insert(private_key_data.end(), private_key_prefix.begin(), private_key_prefix.end());
            private_key_data.insert(private_key_data.end(), private_key.begin(), private_key.end());
            info.node_private_key = EncodeBase58Check(private_key_data);

            return E_SUCCESS;
        }

        std::string id_generator::generate_check_sum()
        {
            //check_sum in protocol header
            return std::string();
        }

        std::string id_generator::generate_session_id()
        {
            CKey secret;

            //random number
            bool fCompressed = true;
            secret.MakeNewKey(fCompressed);
            assert(secret.size() > 0);

            std::vector<unsigned char> session_id_data;
            session_id_data.reserve(secret.size());
            session_id_data.insert(session_id_data.end(), secret.begin(), secret.end());

            return EncodeBase58Check(session_id_data);
        }

        std::string id_generator::generate_nonce()
        {
            CKey secret;

            //random number
            bool fCompressed = true;
            secret.MakeNewKey(fCompressed);
            assert(secret.size() > 0);
            
            std::vector<unsigned char> nonce_data;
            nonce_data.reserve(secret.size());
            nonce_data.insert(nonce_data.end(), secret.begin(), secret.end());

            return EncodeBase58Check(nonce_data);
        }

        std::string id_generator::generate_task_id()
        {
            CKey secret;

            //random number
            bool fCompressed = true;
            secret.MakeNewKey(fCompressed);
            assert(secret.size() > 0);

            std::vector<unsigned char> task_id_data;
            task_id_data.reserve(secret.size());
            task_id_data.insert(task_id_data.end(), secret.begin(), secret.end());

            return EncodeBase58Check(task_id_data);
        }

        bool id_generator::check_node_id(const std::string & node_id)
        {
            std::vector<unsigned char> vchRet;
            return decode_node_id(node_id, vchRet);
        }

       bool id_generator::check_node_private_key(const std::string &node_privarte_key)
       {
           std::vector<unsigned char> vchRet;
           return decode_private_key(node_privarte_key, vchRet);;
       }

       bool id_generator::check_base58_id(const std::string & id)
       {
            std::vector<unsigned char> vchRet;
            
            return DecodeBase58Check(SanitizeString(id), vchRet);
       }

       bool id_generator::derive_node_id_by_private_key(std::string & node_privarte_key, std::string & node_id)
       {
           std::vector<unsigned char> vch;
           if (true != decode_private_key(node_privarte_key, vch))
           {
               return false;
           }

           secp256k1_context *ctx_sign = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
           const int32_t key_len = 32;
           unsigned char privkey_data[key_len];
           if (1 != ec_privkey_import_der(ctx_sign, privkey_data, vch.data(), vch.size()))
           {
               return false;
           }

           CKey secret;
           secret.Set(privkey_data, privkey_data + key_len, true);
           CPubKey pubkey = secret.GetPubKey();

           //create compressed public key
           CKeyID keyID = pubkey.GetID();

           std::vector<unsigned char> id_data;
           std::vector<unsigned char> id_prefix = { 'n', 'o', 'd', 'e', '.', NODE_ID_VERSION, '.' };
           id_data.reserve(id_prefix.size() + keyID.size());
           id_data.insert(id_data.end(), id_prefix.begin(), id_prefix.end());
           id_data.insert(id_data.end(), keyID.begin(), keyID.end());
           node_id = EncodeBase58Check(id_data);
           return true;
       }

       bool id_generator::decode_node_id(const std::string & node_id, std::vector<uint8_t> & vch_node)
       {
           if (DecodeBase58Check(SanitizeString(node_id), vch_node) != true)
           {
               return false;
           }
           std::vector<unsigned char> id_prefix = { 'n', 'o', 'd', 'e', '.', NODE_ID_VERSION, '.' };
           std::vector<unsigned char>::iterator it1;
           std::vector<unsigned char>::iterator it2;
           for (it1 = id_prefix.begin(), it2 = vch_node.begin(); it1 != id_prefix.end() && it2 != vch_node.end();)
           {
               if (*it1 != *it2)
               {
                   return false;
               }
               it2 = vch_node.erase(it2);
               it1++;
           }
           return true;
       }

       bool id_generator::decode_private_key(const std::string & node_privarte_key, std::vector<uint8_t> & vch)
       {
           if (DecodeBase58Check(SanitizeString(node_privarte_key), vch) != true)
           {
               return false;
           }

           std::vector<unsigned char> private_key_prefix = { PRIVATE_KEY_VERSION, '.' };
           std::vector<unsigned char>::iterator it_prfix;
           std::vector<unsigned char>::iterator it_key;
           for (it_prfix = private_key_prefix.begin(), it_key = vch.begin(); it_prfix != private_key_prefix.end() && it_key != vch.end(); )
           {
               if (*it_prfix != *it_key)
               {
                   return false;
               }
               it_key = vch.erase(it_key);
               it_prfix++;
           }
           return true;
       }
    }
}


