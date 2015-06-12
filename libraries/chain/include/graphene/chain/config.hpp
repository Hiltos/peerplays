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

#define GRAPHENE_SYMBOL "CORE"
#define GRAPHENE_ADDRESS_PREFIX "GPH"
#define GRAPHENE_MAX_SYMBOL_NAME_LENGTH 16
#define GRAPHENE_MAX_ASSET_NAME_LENGTH 127
#define GRAPHENE_MAX_SHARE_SUPPLY int64_t(1000000000000ll)
#define GRAPHENE_MAX_PAY_RATE 10000 /* 100% */
#define GRAPHENE_MAX_SIG_CHECK_DEPTH 2
#define GRAPHENE_MIN_WITNESS_COUNT 10
#define GRAPHENE_MIN_DELEGATE_COUNT 10
/**
 * Don't allow the delegates to publish a limit that would
 * make the network unable to operate.
 */
#define GRAPHENE_MIN_TRANSACTION_SIZE_LIMIT 1024
#define GRAPHENE_MAX_BLOCK_INTERVAL  30 /* seconds */

#define GRAPHENE_DEFAULT_BLOCK_INTERVAL  5 /* seconds */
#define GRAPHENE_DEFAULT_MAX_TRANSACTION_SIZE 2048
#define GRAPHENE_DEFAULT_MAX_BLOCK_SIZE  (GRAPHENE_DEFAULT_MAX_TRANSACTION_SIZE*GRAPHENE_DEFAULT_BLOCK_INTERVAL*200000)
#define GRAPHENE_DEFAULT_MAX_TIME_UNTIL_EXPIRATION (60*60*24) // seconds,  aka: 1 day
#define GRAPHENE_DEFAULT_MAINTENANCE_INTERVAL  (60*60*24) // seconds, aka: 1 day
#define GRAPHENE_DEFAULT_MAX_UNDO_HISTORY 1024

#define GRAPHENE_MIN_BLOCK_SIZE_LIMIT (GRAPHENE_MIN_TRANSACTION_SIZE_LIMIT*5) // 5 transactions per block
#define GRAPHENE_MIN_TRANSACTION_EXPIRATION_LIMIT (GRAPHENE_MAX_BLOCK_INTERVAL * 5) // 5 transactions per block
#define GRAPHENE_BLOCKCHAIN_MAX_SHARES                          (1000*1000*int64_t(1000)*1000*int64_t(1000))
#define GRAPHENE_BLOCKCHAIN_PRECISION                           100000
#define GRAPHENE_BLOCKCHAIN_PRECISION_DIGITS                    5
#define GRAPHENE_INITIAL_SUPPLY                                 GRAPHENE_BLOCKCHAIN_MAX_SHARES
#define GRAPHENE_DEFAULT_TRANSFER_FEE                           (1*GRAPHENE_BLOCKCHAIN_PRECISION)
#define GRAPHENE_MAX_INSTANCE_ID                                (uint64_t(-1)>>16)
#define GRAPHENE_100_PERCENT                                    10000
#define GRAPHENE_1_PERCENT                                      (GRAPHENE_100_PERCENT/100)
/** NOTE: making this a power of 2 (say 2^15) would greatly accelerate fee calcs */
#define GRAPHENE_MAX_MARKET_FEE_PERCENT                         GRAPHENE_100_PERCENT
#define GRAPHENE_DEFAULT_FORCE_SETTLEMENT_DELAY                 (60*60*24) ///< 1 day
#define GRAPHENE_DEFAULT_FORCE_SETTLEMENT_OFFSET                0 ///< 1%
#define GRAPHENE_DEFAULT_FORCE_SETTLEMENT_MAX_VOLUME            2000 ///< 20%
#define GRAPHENE_DEFAULT_PRICE_FEED_LIFETIME                    (60*60*24) ///< 1 day
#define GRAPHENE_MAX_FEED_PRODUCERS                             200
#define GRAPHENE_DEFAULT_MAX_AUTHORITY_MEMBERSHIP               10
#define GRAPHENE_DEFAULT_MAX_ASSET_WHITELIST_AUTHORITIES        10
#define GRAPHENE_DEFAULT_MAX_ASSET_FEED_PUBLISHERS              10

#define GRAPHENE_MIN_COLLATERAL_RATIO                   1001  // lower than this could result in divide by 0
#define GRAPHENE_MAX_COLLATERAL_RATIO                   32000 // higher than this is unnecessary and may exceed int16 storage
#define GRAPHENE_DEFAULT_INITIAL_COLLATERAL_RATIO       2000
#define GRAPHENE_DEFAULT_MAINTENANCE_COLLATERAL_RATIO   1750
#define GRAPHENE_DEFAULT_MARGIN_PERIOD_SEC              (30*60*60*24)

#define GRAPHENE_DEFAULT_NUM_WITNESSES                        (101)
#define GRAPHENE_DEFAULT_NUM_COMMITTEE                        (11)
#define GRAPHENE_DEFAULT_MAX_WITNESSES                        (1001) // SHOULD BE ODD
#define GRAPHENE_DEFAULT_MAX_COMMITTEE                        (1001) // SHOULD BE ODD
#define GRAPHENE_DEFAULT_MAX_PROPOSAL_LIFETIME_SEC            (60*60*24*7*4) // Four weeks
#define GRAPHENE_DEFAULT_GENESIS_PROPOSAL_REVIEW_PERIOD_SEC   (60*60*24*7*2) // Two weeks
#define GRAPHENE_DEFAULT_NETWORK_PERCENT_OF_FEE               (20*GRAPHENE_1_PERCENT)
#define GRAPHENE_DEFAULT_LIFETIME_REFERRER_PERCENT_OF_FEE     (30*GRAPHENE_1_PERCENT)
#define GRAPHENE_DEFAULT_MAX_BULK_DISCOUNT_PERCENT            (50*GRAPHENE_1_PERCENT)
#define GRAPHENE_DEFAULT_BULK_DISCOUNT_THRESHOLD_MIN          ( GRAPHENE_BLOCKCHAIN_PRECISION*int64_t(1000) )
#define GRAPHENE_DEFAULT_BULK_DISCOUNT_THRESHOLD_MAX          ( GRAPHENE_DEFAULT_BULK_DISCOUNT_THRESHOLD_MIN*int64_t(100) )
#define GRAPHENE_DEFAULT_CASHBACK_VESTING_PERIOD_SEC          (60*60*24*365) ///< 1 year
#define GRAPHENE_DEFAULT_CASHBACK_VESTING_THRESHOLD           (GRAPHENE_BLOCKCHAIN_PRECISION*int64_t(100))
#define GRAPHENE_DEFAULT_BURN_PERCENT_OF_FEE                  (20*GRAPHENE_1_PERCENT)
#define GRAPHENE_WITNESS_PAY_PERCENT_PRECISION                (1000000000)
#define GRAPHENE_GENESIS_TIMESTAMP                            (1431700000)  /// Should be divisible by GRAPHENE_DEFAULT_BLOCK_INTERVAL

// counter initialization values used to derive near and far future seeds for shuffling witnesses
// we use the fractional bits of sqrt(2) in hex
#define GRAPHENE_NEAR_SCHEDULE_CTR_IV                    ( (uint64_t( 0x6a09 ) << 0x30)    \
                                                         | (uint64_t( 0xe667 ) << 0x20)    \
                                                         | (uint64_t( 0xf3bc ) << 0x10)    \
                                                         | (uint64_t( 0xc908 )        ) )

// and the fractional bits of sqrt(3) in hex
#define GRAPHENE_FAR_SCHEDULE_CTR_IV                     ( (uint64_t( 0xbb67 ) << 0x30)    \
                                                         | (uint64_t( 0xae85 ) << 0x20)    \
                                                         | (uint64_t( 0x84ca ) << 0x10)    \
                                                         | (uint64_t( 0xa73b )        ) )

// counter used to determine bits of entropy
// must be less than or equal to secret_hash_type::data_length()
#define GRAPHENE_RNG_SEED_LENGTH (224 / 8)

/**
 * every second, the fraction of burned core asset which cycles is
 * GRAPHENE_CORE_ASSET_CYCLE_RATE / (1 << GRAPHENE_CORE_ASSET_CYCLE_RATE_BITS)
 */
#define GRAPHENE_CORE_ASSET_CYCLE_RATE                        17
#define GRAPHENE_CORE_ASSET_CYCLE_RATE_BITS                   32

#define GRAPHENE_DEFAULT_WITNESS_PAY_PER_BLOCK            (GRAPHENE_BLOCKCHAIN_PRECISION * int64_t( 10) )
#define GRAPHENE_DEFAULT_WORKER_BUDGET_PER_DAY            (GRAPHENE_BLOCKCHAIN_PRECISION * int64_t(500) * 1000 )

#define GRAPHENE_MAX_INTEREST_APR                            uint16_t( 10000 )
#define GRAPHENE_LEGACY_NAME_IMPORT_PERIOD                   3000000 /** 3 million blocks */

