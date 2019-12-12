#pragma once
#include <graphene/chain/protocol/base.hpp>

namespace graphene { namespace chain {

    struct son_create_operation : public base_operation
    {
        struct fee_parameters_type { uint64_t fee = 0; };

        asset fee;
        account_id_type owner_account;
        std::string url;
        vesting_balance_id_type deposit;
        public_key_type signing_key;
        vesting_balance_id_type pay_vb;

        account_id_type fee_payer()const { return owner_account; }
        share_type      calculate_fee(const fee_parameters_type& k)const { return 0; }
    };

    struct son_update_operation : public base_operation
    {
         struct fee_parameters_type { uint64_t fee = 0; };

         asset fee;
         son_id_type son_id;
         account_id_type owner_account;
         optional<std::string> new_url;
         optional<vesting_balance_id_type> new_deposit;
         optional<public_key_type> new_signing_key;
         optional<vesting_balance_id_type> new_pay_vb;

         account_id_type fee_payer()const { return owner_account; }
         share_type      calculate_fee(const fee_parameters_type& k)const { return 0; }
    };

    struct son_delete_operation : public base_operation
    {
        struct fee_parameters_type { uint64_t fee = 0; };

        asset fee;
        son_id_type son_id;
        account_id_type payer;
        account_id_type owner_account;

        account_id_type fee_payer()const { return payer; }
        share_type      calculate_fee(const fee_parameters_type& k)const { return 0; }
    };

    struct son_heartbeat_operation : public base_operation
    {
        struct fee_parameters_type { uint64_t fee = 0; };

        asset fee;
        son_id_type son_id;
        account_id_type owner_account;
        time_point_sec ts;

        account_id_type fee_payer()const { return owner_account; }
        share_type      calculate_fee(const fee_parameters_type& k)const { return 0; }
    };

    struct son_report_down_operation : public base_operation
    {
        struct fee_parameters_type { uint64_t fee = 0; };

        asset fee;
        son_id_type son_id;
        account_id_type payer;
        time_point_sec down_ts;

        account_id_type fee_payer()const { return payer; }
        share_type      calculate_fee(const fee_parameters_type& k)const { return 0; }
    };

} } // namespace graphene::chain

FC_REFLECT(graphene::chain::son_create_operation::fee_parameters_type, (fee) )
FC_REFLECT(graphene::chain::son_create_operation, (fee)(owner_account)(url)(deposit)(signing_key)
           (pay_vb) )

FC_REFLECT(graphene::chain::son_update_operation::fee_parameters_type, (fee) )
FC_REFLECT(graphene::chain::son_update_operation, (fee)(son_id)(owner_account)(new_url)(new_deposit)
           (new_signing_key)(new_pay_vb) )

FC_REFLECT(graphene::chain::son_delete_operation::fee_parameters_type, (fee) )
FC_REFLECT(graphene::chain::son_delete_operation, (fee)(son_id)(payer)(owner_account) )

FC_REFLECT(graphene::chain::son_heartbeat_operation::fee_parameters_type, (fee) )
FC_REFLECT(graphene::chain::son_heartbeat_operation, (fee)(son_id)(owner_account)(ts) )

FC_REFLECT(graphene::chain::son_report_down_operation::fee_parameters_type, (fee) )
FC_REFLECT(graphene::chain::son_report_down_operation, (fee)(son_id)(payer)(down_ts) )