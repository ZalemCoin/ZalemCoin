// Copyright (c) 2016-2019 The CryptoCoderz Team / Espers
// Copyright (c) 2018-2019 The CryptoCoderz Team / INSaNe project
// Copyright (c) 2018-2019 The Rubix project
// Copyright (c) 2018-2019 The InvoiceCoin project
// Copyright (c) 2018-2019 The CryptoCoderz Team / Endox project
// Copyright (c) 2019 The Zalem project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_GENESIS_H
#define BITCOIN_GENESIS_H

#include "bignum.h"

/** Genesis Start Time */
static const unsigned int timeGenesisBlock = 1553893749; // Friday, March 29, 2019 2:09:09 PM GMT-07:00
/** Genesis TestNet Start Time */
static const unsigned int timeTestNetGenesis = 1553893749+30; // Friday, March 29, 2019 2:09:09 PM GMT-07:00
/** Genesis RegNet Start Time */
static const unsigned int timeRegNetGenesis = 1553893749+90; // Friday, March 29, 2019 2:09:09 PM GMT-07:00
/** Genesis Nonce Mainnet*/
static const unsigned int nNonceMain = 0;
/** Genesis Nonce Testnet */
static const unsigned int nNonceTest = 0;
/** Genesis Nonce Regnet */
static const unsigned int nNonceReg = 8;
/** Main Net Genesis Block */
static const uint256 nGenesisBlock("0x");
/** Test Net Genesis Block */
static const uint256 hashTestNetGenesisBlock("0x");
/** Reg Net Genesis Block */
static const uint256 hashRegNetGenesisBlock("0x");
/** Genesis Merkleroot */
static const uint256 nGenesisMerkle("0x");

#endif // BITCOIN_GENESIS_H
