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
#pragma once

#include <fc/reflect/reflect.hpp>

#include <cstdint>

#include <graphene/chain/config.hpp>

namespace graphene { namespace chain {

struct immutable_chain_parameters
{
   uint16_t min_committee_member_count = GRAPHENE_DEFAULT_MIN_COMMITTEE_MEMBER_COUNT;
   uint16_t min_witness_count = GRAPHENE_DEFAULT_MIN_WITNESS_COUNT;
   uint32_t num_special_accounts = 0;
   uint32_t num_special_assets = 0;
};

} } // graphene::chain

FC_REFLECT( graphene::chain::immutable_chain_parameters,
   (min_committee_member_count)
   (min_witness_count)
   (num_special_accounts)
   (num_special_assets)
)
