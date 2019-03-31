// Copyright (c) 2016-2019 The CryptoCoderz Team / Espers
// Copyright (c) 2018-2019 The CryptoCoderz Team / INSaNe project
// Copyright (c) 2018-2019 The Rubix project
// Copyright (c) 2018-2019 The InvoiceCoin project
// Copyright (c) 2018-2019 The CryptoCoderz Team / Endox project
// Copyright (c) 2019 The Zalem project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_MINING_H
#define BITCOIN_MINING_H

#include "bignum.h"

/** Minimum nCoinAge required to stake PoS */
static const unsigned int nStakeMinAge = 2 / 60; // 30 minutes
/** Time to elapse before new modifier is computed */
static const unsigned int nModifierInterval = 2 * 60;
/** Genesis block subsidy */
static const int64_t nGenesisBlockReward = 1 * COIN;
/** Reserve block subsidy */
static const int64_t nBlockRewardReserve = 990000 * COIN; // 99-Million Reserved for Development and for the awesome people responsible for the movie!
/** Standard block subsidy */
static const int64_t nBlockStandardReward = 99 * COIN;
/** Superblock block subsidy */
static const int64_t nBlockSuperReward = 300 * COIN;
/** Block spacing preferred */
static const int64_t BLOCK_SPACING = 180;
/** Block spacing minimum */
static const int64_t BLOCK_SPACING_MIN = 60;
/** Block spacing maximum */
static const int64_t BLOCK_SPACING_MAX = 270;
/** Desired block times/spacing */
static const int64_t GetTargetSpacing = BLOCK_SPACING;
/** MNengine collateral */
static const int64_t MNengine_COLLATERAL = (1 * COIN);
/** MNengine pool values */
static const int64_t MNengine_POOL_MAX = (999 * COIN);
/** FloatingCity required collateral */
inline int64_t FloatingcityCollateral(int nHeight) { return 100000; } // 100K ZLM required as collateral
/** Coinbase transaction outputs can only be staked after this number of new blocks (network rule) */
static const int nStakeMinConfirmations = 30;
/** Coinbase transaction outputs can only be spent after this number of new blocks (network rule) */
static const int nCoinbaseMaturity = 23; // 23-TXs | 99-Mined ;) 


#endif // BITCOIN_MINING_H
