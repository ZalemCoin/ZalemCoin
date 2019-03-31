// Copyright (c) 2016-2019 The CryptoCoderz Team / Espers
// Copyright (c) 2018-2019 The Rubix project
// Copyright (c) 2018-2019 The InvoiceCoin project
// Copyright (c) 2018-2019 The CryptoCoderz Team / Endox project
// Copyright (c) 2019 The Zalem project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_FORK_H
#define BITCOIN_FORK_H

#include "bignum.h"

/** Reserve Phase start block */ 
static const int64_t nReservePhaseStart = 1;
/** Velocity toggle block */
static const int64_t VELOCITY_TOGGLE = 175; // Implementation of the Velocity system into the chain.
/** Velocity retarget toggle block */
static const int64_t VELOCITY_TDIFF = 0; // Use Velocity's retargetting method.
/** Velocity retarget min-diff allow block */
static const int64_t VRX_MDIFF = 1554091740; // ON (Sunday, March 31, 2019 9:09:00 PM GMT-07:00)
/** Protocol 3.0 toggle */
inline bool IsProtocolV3(int64_t nTime) { return TestNet() || nTime > 1553893749; } // ON (Friday, March 29, 2019 2:09:09 PM GMT-07:00)
#endif // BITCOIN_FORK_H
