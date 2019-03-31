// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "assert.h"

#include "chainparams.h"
#include "main.h"
#include "util.h"

#include <boost/assign/list_of.hpp>

using namespace boost::assign;

struct SeedSpec6 {
    uint8_t addr[16];
    uint16_t port;
};

#include "chainparamsseeds.h"

// Convert the pnSeeds6 array into usable address objects.
static void convertSeed6(std::vector<CAddress> &vSeedsOut, const SeedSpec6 *data, unsigned int count)
{
    // It'll only connect to one or two seed nodes because once it connects,
    // it'll get a pile of addresses with newer timestamps.
    // Seed nodes are given a random 'last seen time' of between one and two
    // weeks ago.
    const int64_t nOneWeek = 7*24*60*60;
    for (unsigned int i = 0; i < count; i++)
    {
        struct in6_addr ip;
        memcpy(&ip, data[i].addr, sizeof(ip));
        CAddress addr(CService(ip, data[i].port));
        addr.nTime = GetTime() - GetRand(nOneWeek) - nOneWeek;
        vSeedsOut.push_back(addr);
    }
}

//
// Main network
//
class CMainParams : public CChainParams {
public:
    CMainParams() {
        // The message start string is designed to be unlikely to occur in normal data.
        // The characters are rarely used upper ASCII, not valid as UTF-8, and produce
        // a large 4-byte int at any alignment.
        pchMessageStart[0] = 0xc9;
        pchMessageStart[1] = 0xb9;
        pchMessageStart[2] = 0xa3;
        pchMessageStart[3] = 0xf0;
        vAlertPubKey = ParseHex("01a30099a18f5556be6b659c91a94fbfebeb5d517698712acdbef262f7c2f81f85d131a669df3be611393f454852a2d08c6314bba5ca3cbe5616262da3d0a0eeea");
        nDefaultPort = 30099;
        nRPCPort = 30097;
        bnProofOfWorkLimit = CBigNum(~uint256(0) >> 20);
        bnProofOfStakeLimit = CBigNum(~uint256(0) >> 22);

        const char* pszTimestamp = "Russian Authorities Force VPN Providers to Engage in Internet Censorship | JP Buntinx | March 29, 2019";
        std::vector<CTxIn> vin;
        vin.resize(1);
        vin[0].scriptSig = CScript() << 0 << CBigNum(42) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
        std::vector<CTxOut> vout;
        vout.resize(1);
        vout[0].nValue = 1 * COIN;
        vout[0].SetEmpty();
        CTransaction txNew(1, timeGenesisBlock, vin, vout, 0);
        genesis.vtx.push_back(txNew);
        genesis.hashPrevBlock = 0;
        genesis.hashMerkleRoot = genesis.BuildMerkleTree();
        genesis.nVersion = 1;
        genesis.nTime    = timeGenesisBlock; // Sat, December 15, 2018 8:00:00 PM
        genesis.nBits    = bnProofOfWorkLimit.GetCompact();
        genesis.nNonce   = 380239;

        /** Genesis Block MainNet */
        /*
        Hashed MainNet Genesis Block Output
        block.hashMerkleRoot == 245c5fc049083a6dde74f688345e9e508ee676ec9b6b5a06cc7112cd30e659cd
        block.nTime = 1553893749
        block.nNonce = 380239
        block.GetHash = 00000b58395d948c20635dffb0232b1811b65494b36b79fc7790bf844dc35066
        */

        hashGenesisBlock = genesis.GetHash();
        assert(hashGenesisBlock == uint256("0x00000b58395d948c20635dffb0232b1811b65494b36b79fc7790bf844dc35066"));
        assert(genesis.hashMerkleRoot == uint256("0x245c5fc049083a6dde74f688345e9e508ee676ec9b6b5a06cc7112cd30e659cd"));
        //assert(hashGenesisBlock == nGenesisBlock);
        //assert(genesis.hashMerkleRoot == nGenesisMerkle);

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,80);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,48);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,50);
        base58Prefixes[STEALTH_ADDRESS] = std::vector<unsigned char>(1,142);
        base58Prefixes[EXT_PUBLIC_KEY] = list_of(0x04)(0x88)(0xB2)(0x1E).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = list_of(0x04)(0x88)(0xAD)(0xE4).convert_to_container<std::vector<unsigned char> >();

        //vSeeds.push_back(CDNSSeedData("node1",  ""));
        //vSeeds.push_back(CDNSSeedData("node2",  ""));
        convertSeed6(vFixedSeeds, pnSeed6_main, ARRAYLEN(pnSeed6_main));

        nPoolMaxTransactions = 9;
        strMNenginePoolDummyAddress = "ZNmquzW1AVgojui8kp33fYgqYJGyj6qeiP";
        nEndPoWBlock = 0x7fffffff;
        nStartPoSBlock = 0;
    }

    virtual const CBlock& GenesisBlock() const { return genesis; }
    virtual Network NetworkID() const { return CChainParams::MAIN; }

    virtual const vector<CAddress>& FixedSeeds() const {
        return vFixedSeeds;
    }
protected:
    CBlock genesis;
    vector<CAddress> vFixedSeeds;
};
static CMainParams mainParams;


//
// Testnet
//
class CTestNetParams : public CMainParams {
public:
    CTestNetParams() {
        // The message start string is designed to be unlikely to occur in normal data.
        // The characters are rarely used upper ASCII, not valid as UTF-8, and produce
        // a large 4-byte int at any alignment.
        pchMessageStart[0] = 0xc8;
        pchMessageStart[1] = 0xb8;
        pchMessageStart[2] = 0xf3;
        pchMessageStart[3] = 0xe0;
        bnProofOfWorkLimit = CBigNum(~uint256(0) >> 18);
        bnProofOfStakeLimit = CBigNum(~uint256(0) >> 20);
        vAlertPubKey = ParseHex("02b30099a18f5556be6b659c91a94fbfebeb5d517698712acdbef262f7c2f81f85d131a669df3be611393f454852a2d08c6314bba5ca3cbe5616262da3d0a0eeea");
        nDefaultPort = 30088;
        nRPCPort = 30085;
        strDataDir = "testnet";

        // Modify the testnet genesis block so the timestamp is valid for a later start.
        genesis.nTime  = timeTestNetGenesis;
        genesis.nBits  = bnProofOfWorkLimit.GetCompact();
        genesis.nNonce = 25998;

        /** Genesis Block TestNet */
        /*
        Hashed TestNet Genesis Block Output
        block.hashMerkleRoot == 245c5fc049083a6dde74f688345e9e508ee676ec9b6b5a06cc7112cd30e659cd
        block.nTime = 1553893779
        block.nNonce = 25998
        block.GetHash = 000012b91a1d0e6a63732221f5ec8ccc2edc5a14e4cb5638bec06ec3f42658b0
        */

        hashGenesisBlock = genesis.GetHash();
        assert(hashGenesisBlock == uint256("0x000012b91a1d0e6a63732221f5ec8ccc2edc5a14e4cb5638bec06ec3f42658b0"));
        //assert(hashGenesisBlock == hashTestNetGenesisBlock);

        vFixedSeeds.clear();
        vSeeds.clear();

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,81);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,49);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,51);
        base58Prefixes[STEALTH_ADDRESS] = std::vector<unsigned char>(1,143);
        base58Prefixes[EXT_PUBLIC_KEY] = list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

        nEndPoWBlock = 0x7fffffff;
    }
    virtual Network NetworkID() const { return CChainParams::TESTNET; }
};
static CTestNetParams testNetParams;

//
// Regression test
//
class CRegTestParams : public CTestNetParams {
public:
    CRegTestParams() {
        pchMessageStart[0] = 0xc7;
        pchMessageStart[1] = 0xb7;
        pchMessageStart[2] = 0x03;
        pchMessageStart[3] = 0x00;
        bnProofOfWorkLimit = CBigNum(~uint256(0) >> 1);
        genesis.nTime = timeRegNetGenesis;
        genesis.nBits  = bnProofOfWorkLimit.GetCompact();
        genesis.nNonce = 8;
        hashGenesisBlock = genesis.GetHash();
        nDefaultPort = 31223;
        strDataDir = "regtest";

        /** Genesis Block RegNet */
        /*
        Hashed RegNet Genesis Block Output
        block.hashMerkleRoot == 245c5fc049083a6dde74f688345e9e508ee676ec9b6b5a06cc7112cd30e659cd
        block.nTime = 1553893839
        block.nNonce = 8
        block.GetHash = d2c53923d77b4f34280f43c3dd96d3998742da230903bb7ad684f2190fd4ec23
        */

        assert(hashGenesisBlock == uint256("0xd2c53923d77b4f34280f43c3dd96d3998742da230903bb7ad684f2190fd4ec23"));
        //assert(hashGenesisBlock == hashRegNetGenesisBlock);

        vSeeds.clear();  // Regtest mode doesn't have any DNS seeds.
    }

    virtual bool RequireRPCPassword() const { return false; }
    virtual Network NetworkID() const { return CChainParams::REGTEST; }
};
static CRegTestParams regTestParams;

static CChainParams *pCurrentParams = &mainParams;

const CChainParams &Params() {
    return *pCurrentParams;
}

void SelectParams(CChainParams::Network network) {
    switch (network) {
        case CChainParams::MAIN:
            pCurrentParams = &mainParams;
            break;
        case CChainParams::TESTNET:
            pCurrentParams = &testNetParams;
            break;
        case CChainParams::REGTEST:
            pCurrentParams = &regTestParams;
            break;
        default:
            assert(false && "Unimplemented network");
            return;
    }
}

bool SelectParamsFromCommandLine() {
    bool fRegTest = GetBoolArg("-regtest", false);
    bool fTestNet = GetBoolArg("-testnet", false);

    if (fTestNet && fRegTest) {
        return false;
    }

    if (fRegTest) {
        SelectParams(CChainParams::REGTEST);
    } else if (fTestNet) {
        SelectParams(CChainParams::TESTNET);
    } else {
        SelectParams(CChainParams::MAIN);
    }
    return true;
}
