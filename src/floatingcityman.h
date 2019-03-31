
// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef FLOATINGCITYMAN_H
#define FLOATINGCITYMAN_H

#include "bignum.h"
#include "sync.h"
#include "net.h"
#include "key.h"
#include "chain.h"
#include "util.h"
#include "script.h"
#include "base58.h"
#include "main.h"
#include "floatingcity.h"

#define FLOATINGCITYS_DUMP_SECONDS               (15*60)
#define FLOATINGCITYS_DSEG_SECONDS               (3*60*60)

using namespace std;

class CFloatingcityMan;

extern CFloatingcityMan mnodeman;

void DumpFloatingcities();

/** Access to the MN database (mncache.dat) */
class CFloatingcityDB
{
private:
    boost::filesystem::path pathMN;
    std::string strMagicMessage;
public:
    enum ReadResult {
        Ok,
       FileError,
        HashReadError,
        IncorrectHash,
        IncorrectMagicMessage,
        IncorrectMagicNumber,
        IncorrectFormat
    };

    CFloatingcityDB();
    bool Write(const CFloatingcityMan &mnodemanToSave);
    ReadResult Read(CFloatingcityMan& mnodemanToLoad);
};

class CFloatingcityMan
{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;

    // map to hold all MNs
    std::vector<CFloatingcity> vFloatingcities;
    // who's asked for the floatingcity list and the last time
    std::map<CNetAddr, int64_t> mAskedUsForFloatingcityList;
    // who we asked for the floatingcity list and the last time
    std::map<CNetAddr, int64_t> mWeAskedForFloatingcityList;
    // which floatingcities we've asked for
    std::map<COutPoint, int64_t> mWeAskedForFloatingcityListEntry;

public:
    // keep track of dsq count to prevent floatingcities from gaming fcengine queue
    int64_t nDsqCount;

    IMPLEMENT_SERIALIZE
    (
        // serialized format:
        // * version byte (currently 0)
        // * floatingcities vector
        {
                LOCK(cs);
                unsigned char nVersion = 0;
                READWRITE(nVersion);
                READWRITE(vFloatingcities);
                READWRITE(mAskedUsForFloatingcityList);
                READWRITE(mWeAskedForFloatingcityList);
                READWRITE(mWeAskedForFloatingcityListEntry);
                READWRITE(nDsqCount);
        }
    )

    CFloatingcityMan();
    CFloatingcityMan(CFloatingcityMan& other);

    // Add an entry
    bool Add(CFloatingcity &mn);

    // Check all floatingcities
    void Check();

    /// Ask (source) node for mnb
    void AskForMN(CNode *pnode, CTxIn &vin);

    // Check all floatingcities and remove inactive
    void CheckAndRemove();

    // Clear floatingcity vector
    void Clear();

    int CountEnabled(int protocolVersion = -1);

    int CountFloatingcitiesAboveProtocol(int protocolVersion);

    void DsegUpdate(CNode* pnode);

    // Find an entry
    CFloatingcity* Find(const CTxIn& vin);
    CFloatingcity* Find(const CPubKey& pubKeyFloatingcity);

    //Find an entry thta do not match every entry provided vector
    CFloatingcity* FindOldestNotInVec(const std::vector<CTxIn> &vVins, int nMinimumAge);

    // Find a random entry
    CFloatingcity* FindRandom();

    /// Find a random entry
    CFloatingcity* FindRandomNotInVec(std::vector<CTxIn> &vecToExclude, int protocolVersion = -1);

    // Get the current winner for this block
    CFloatingcity* GetCurrentFloatingCity(int mod=1, int64_t nBlockHeight=0, int minProtocol=0);

    std::vector<CFloatingcity> GetFullFloatingcityVector() { Check(); return vFloatingcities; }

    std::vector<pair<int, CFloatingcity> > GetFloatingcityRanks(int64_t nBlockHeight, int minProtocol=0);
    int GetFloatingcityRank(const CTxIn &vin, int64_t nBlockHeight, int minProtocol=0, bool fOnlyActive=true);
    CFloatingcity* GetFloatingcityByRank(int nRank, int64_t nBlockHeight, int minProtocol=0, bool fOnlyActive=true);

    void ProcessFloatingcityConnections();

    void ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);

    // Return the number of (unique) floatingcities
    int size() { return vFloatingcities.size(); }

    std::string ToString() const;

    //
    // Relay Floatingcity Messages
    //

    void RelayFloatingcityEntry(const CTxIn vin, const CService addr, const std::vector<unsigned char> vchSig, const int64_t nNow, const CPubKey pubkey, const CPubKey pubkey2, const int count, const int current, const int64_t lastUpdated, const int protocolVersion, CScript donationAddress, int donationPercentage);
    void RelayFloatingcityEntryPing(const CTxIn vin, const std::vector<unsigned char> vchSig, const int64_t nNow, const bool stop);

    void Remove(CTxIn vin);

};

#endif
