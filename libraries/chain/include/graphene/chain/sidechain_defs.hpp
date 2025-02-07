#pragma once

#include <fc/reflect/reflect.hpp>

namespace graphene { namespace chain {

enum class sidechain_type {
   unknown,
   bitcoin,
   ethereum,
   eos,
   peerplays
};

} }

FC_REFLECT_ENUM(graphene::chain::sidechain_type,
        (unknown)
        (bitcoin)
        (ethereum)
        (eos)
        (peerplays) )
