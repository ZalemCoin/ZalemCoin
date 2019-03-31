#include "floatingcityman.h"
#include "floatingcity.h"
#include "activefloatingcity.h"
#include "fcengine.h"
#include "chain.h"
#include "util.h"
#include "addrman.h"
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>


/** Floatingcity manager */
CFloatingcityMan mnodeman;
CCriticalSection cs_process_message;

struct CompareValueOnly
{
    bool operator()(const pair<int64_t, CTxIn>& t1,
                    const pair<int64_t, CTxIn>& t2) const
    {
        return t1.first < t2.first;
    }
};
struct CompareValueOnlyMN
{
    bool operator()(const pair<int64_t, CFloatingcity>& t1,
                    const pair<int64_t, CFloatingcity>& t2) const
    {
        return t1.first < t2.first;
    }
};


//
// CFloatingcityDB
//

CFloatingcityDB::CFloatingcityDB()
{
    pathMN = GetDataDir() / "mncache.dat";
    strMagicMessage = "FloatingcityCache";
}

bool CFloatingcityDB::Write(const CFloatingcityMan& mnodemanToSave)
{
    int64_t nStart = GetTimeMillis();

    // serialize addresses, checksum data up to that point, then append csum
    CDataStream ssFloatingcities(SER_DISK, CLIENT_VERSION);
    ssFloatingcities << strMagicMessage; // floatingcity cache file specific magic message
    ssFloatingcities << FLATDATA(Params().MessageStart()); // network specific magic number
    ssFloatingcities << mnodemanToSave;
    uint256 hash = Hash(ssFloatingcities.begin(), ssFloatingcities.end());
    ssFloatingcities << hash;

    // open output file, and associate with CAutoFile
    FILE *file = fopen(pathMN.string().c_str(), "wb");
    CAutoFile fileout = CAutoFile(file, SER_DISK, CLIENT_VERSION);
    if (!fileout)
        return error("%s : Failed to open file %s", __func__, pathMN.string());

    // Write and commit header, data
    try {
        fileout << ssFloatingcities;
    }
    catch (std::exception &e) {
        return error("%s : Serialize or I/O error - %s", __func__, e.what());
    }
    FileCommit(fileout);
    fileout.fclose();

    LogPrintf("Written info to mncache.dat  %dms\n", GetTimeMillis() - nStart);
    LogPrintf("  %s\n", mnodemanToSave.ToString());

    return true;
}

CFloatingcityDB::ReadResult CFloatingcityDB::Read(CFloatingcityMan& mnodemanToLoad)
{
    int64_t nStart = GetTimeMillis();
    // open input file, and associate with CAutoFile
    FILE *file = fopen(pathMN.string().c_str(), "rb");
    CAutoFile filein = CAutoFile(file, SER_DISK, CLIENT_VERSION);
    if (!filein)
    {
        error("%s : Failed to open file %s", __func__, pathMN.string());
        return FileError;
    }

    // use file size to size memory buffer
    int fileSize = boost::filesystem::file_size(pathMN);
    int dataSize = fileSize - sizeof(uint256);
    // Don't try to resize to a negative number if file is small
    if (dataSize < 0)
        dataSize = 0;
    vector<unsigned char> vchData;
    vchData.resize(dataSize);
    uint256 hashIn;

    // read data and checksum from file
    try {
        filein.read((char *)&vchData[0], dataSize);
        filein >> hashIn;
    }
    catch (std::exception &e) {
        error("%s : Deserialize or I/O error - %s", __func__, e.what());
        return HashReadError;
    }
    filein.fclose();

    CDataStream ssFloatingcities(vchData, SER_DISK, CLIENT_VERSION);

    // verify stored checksum matches input data
    uint256 hashTmp = Hash(ssFloatingcities.begin(), ssFloatingcities.end());
    if (hashIn != hashTmp)
    {
        error("%s : Checksum mismatch, data corrupted", __func__);
        return IncorrectHash;
    }

    unsigned char pchMsgTmp[4];
    std::string strMagicMessageTmp;
    try {
        // de-serialize file header (floatingcity cache file specific magic message) and ..

        ssFloatingcities >> strMagicMessageTmp;

        // ... verify the message matches predefined one
        if (strMagicMessage != strMagicMessageTmp)
        {
            error("%s : Invalid floatingcity cache magic message", __func__);
            return IncorrectMagicMessage;
        }

        // de-serialize file header (network specific magic number) and ..
        ssFloatingcities >> FLATDATA(pchMsgTmp);

        // ... verify the network matches ours
        if (memcmp(pchMsgTmp, Params().MessageStart(), sizeof(pchMsgTmp)))
        {
            error("%s : Invalid network magic number", __func__);
            return IncorrectMagicNumber;
        }

        // de-serialize address data into one CMnList object
        ssFloatingcities >> mnodemanToLoad;
    }
    catch (std::exception &e) {
        mnodemanToLoad.Clear();
        error("%s : Deserialize or I/O error - %s", __func__, e.what());
        return IncorrectFormat;
    }

    mnodemanToLoad.CheckAndRemove(); // clean out expired
    LogPrintf("Loaded info from mncache.dat  %dms\n", GetTimeMillis() - nStart);
    LogPrintf("  %s\n", mnodemanToLoad.ToString());

    return Ok;
}

void DumpFloatingcities()
{
    int64_t nStart = GetTimeMillis();

    CFloatingcityDB mndb;
    CFloatingcityMan tempMnodeman;

    LogPrintf("Verifying mncache.dat format...\n");
    CFloatingcityDB::ReadResult readResult = mndb.Read(tempMnodeman);
    // there was an error and it was not an error on file openning => do not proceed
    if (readResult == CFloatingcityDB::FileError)
        LogPrintf("Missing floatingcity list file - mncache.dat, will try to recreate\n");
    else if (readResult != CFloatingcityDB::Ok)
    {
        LogPrintf("Error reading mncache.dat: ");
        if(readResult == CFloatingcityDB::IncorrectFormat)
            LogPrintf("magic is ok but data has invalid format, will try to recreate\n");
        else
        {
            LogPrintf("file format is unknown or invalid, please fix it manually\n");
            return;
        }
    }
    LogPrintf("Writting info to mncache.dat...\n");
    mndb.Write(mnodeman);

    LogPrintf("Floatingcity dump finished  %dms\n", GetTimeMillis() - nStart);
}

CFloatingcityMan::CFloatingcityMan() {
    nDsqCount = 0;
}

bool CFloatingcityMan::Add(CFloatingcity &mn)
{
    LOCK(cs);

    if (!mn.IsEnabled())
        return false;

    CFloatingcity *pmn = Find(mn.vin);

    if (pmn == NULL)
    {
        LogPrint("floatingcity", "CFloatingcityMan: Adding new floatingcity %s - %i now\n", mn.addr.ToString().c_str(), size() + 1);
        vFloatingcities.push_back(mn);
        return true;
    }

    return false;
}

void CFloatingcityMan::AskForMN(CNode* pnode, CTxIn &vin)
{
    std::map<COutPoint, int64_t>::iterator i = mWeAskedForFloatingcityListEntry.find(vin.prevout);
    if (i != mWeAskedForFloatingcityListEntry.end())
    {
        int64_t t = (*i).second;
        if (GetTime() < t) return; // we've asked recently
    }

    // ask for the mnb info once from the node that sent mnp

    LogPrintf("CFloatingcityMan::AskForMN - Asking node for missing entry, vin: %s\n", vin.ToString());
    pnode->PushMessage("dseg", vin);
    int64_t askAgain = GetTime() + FLOATINGCITY_MIN_DSEEP_SECONDS;
    mWeAskedForFloatingcityListEntry[vin.prevout] = askAgain;
}

void CFloatingcityMan::Check()
{
    LOCK(cs);

    BOOST_FOREACH(CFloatingcity& mn, vFloatingcities)
        mn.Check();
}

void CFloatingcityMan::CheckAndRemove()
{
    LOCK(cs);

    Check();

    //remove inactive
    vector<CFloatingcity>::iterator it = vFloatingcities.begin();
    while(it != vFloatingcities.end()){
        if((*it).activeState == CFloatingcity::FLOATINGCITY_REMOVE || (*it).activeState == CFloatingcity::FLOATINGCITY_VIN_SPENT || (*it).protocolVersion < nFloatingcityMinProtocol){
            LogPrint("floatingcity", "CFloatingcityMan: Removing inactive floatingcity %s - %i now\n", (*it).addr.ToString().c_str(), size() - 1);
            it = vFloatingcities.erase(it);
        } else {
            ++it;
        }
    }

    // check who's asked for the floatingcity list
    map<CNetAddr, int64_t>::iterator it1 = mAskedUsForFloatingcityList.begin();
    while(it1 != mAskedUsForFloatingcityList.end()){
        if((*it1).second < GetTime()) {
            mAskedUsForFloatingcityList.erase(it1++);
        } else {
            ++it1;
        }
    }

    // check who we asked for the floatingcity list
    it1 = mWeAskedForFloatingcityList.begin();
    while(it1 != mWeAskedForFloatingcityList.end()){
        if((*it1).second < GetTime()){
            mWeAskedForFloatingcityList.erase(it1++);
        } else {
            ++it1;
        }
    }

    // check which floatingcities we've asked for
    map<COutPoint, int64_t>::iterator it2 = mWeAskedForFloatingcityListEntry.begin();
    while(it2 != mWeAskedForFloatingcityListEntry.end()){
        if((*it2).second < GetTime()){
            mWeAskedForFloatingcityListEntry.erase(it2++);
        } else {
            ++it2;
        }
    }

}

void CFloatingcityMan::Clear()
{
    LOCK(cs);
    vFloatingcities.clear();
    mAskedUsForFloatingcityList.clear();
    mWeAskedForFloatingcityList.clear();
    mWeAskedForFloatingcityListEntry.clear();
    nDsqCount = 0;
}

int CFloatingcityMan::CountEnabled(int protocolVersion)
{
    int i = 0;
    protocolVersion = protocolVersion == -1 ? floatingcityPayments.GetMinFloatingcityPaymentsProto() : protocolVersion;

    BOOST_FOREACH(CFloatingcity& mn, vFloatingcities) {
        mn.Check();
        if(mn.protocolVersion < protocolVersion || !mn.IsEnabled()) continue;
        i++;
    }

    return i;
}

int CFloatingcityMan::CountFloatingcitiesAboveProtocol(int protocolVersion)
{
    int i = 0;

    BOOST_FOREACH(CFloatingcity& mn, vFloatingcities) {
        mn.Check();
        if(mn.protocolVersion < protocolVersion || !mn.IsEnabled()) continue;
        i++;
    }

    return i;
}

void CFloatingcityMan::DsegUpdate(CNode* pnode)
{
    LOCK(cs);

    std::map<CNetAddr, int64_t>::iterator it = mWeAskedForFloatingcityList.find(pnode->addr);
    if (it != mWeAskedForFloatingcityList.end())
    {
        if (GetTime() < (*it).second) {
            LogPrintf("dseg - we already asked %s for the list; skipping...\n", pnode->addr.ToString());
            return;
        }
    }
    pnode->PushMessage("dseg", CTxIn());
    int64_t askAgain = GetTime() + FLOATINGCITYS_DSEG_SECONDS;
    mWeAskedForFloatingcityList[pnode->addr] = askAgain;
}

CFloatingcity *CFloatingcityMan::Find(const CTxIn &vin)
{
    LOCK(cs);

    BOOST_FOREACH(CFloatingcity& mn, vFloatingcities)
    {
        if(mn.vin.prevout == vin.prevout)
            return &mn;
    }
    return NULL;
}

CFloatingcity* CFloatingcityMan::FindOldestNotInVec(const std::vector<CTxIn> &vVins, int nMinimumAge)
{
    LOCK(cs);

    CFloatingcity *pOldestFloatingcity = NULL;

    BOOST_FOREACH(CFloatingcity &mn, vFloatingcities)
    {   
        mn.Check();
        if(!mn.IsEnabled()) continue;

        if(mn.GetFloatingcityInputAge() < nMinimumAge) continue;

        bool found = false;
        BOOST_FOREACH(const CTxIn& vin, vVins)
            if(mn.vin.prevout == vin.prevout)
            {   
                found = true;
                break;
            }
        if(found) continue;

        if(pOldestFloatingcity == NULL || pOldestFloatingcity->SecondsSincePayment() < mn.SecondsSincePayment())
        {
            pOldestFloatingcity = &mn;
        }
    }

    return pOldestFloatingcity;
}

CFloatingcity *CFloatingcityMan::FindRandom()
{
    LOCK(cs);

    if(size() == 0) return NULL;

    return &vFloatingcities[GetRandInt(vFloatingcities.size())];
}

CFloatingcity *CFloatingcityMan::Find(const CPubKey &pubKeyFloatingcity)
{
    LOCK(cs);

    BOOST_FOREACH(CFloatingcity& mn, vFloatingcities)
    {
        if(mn.pubkey2 == pubKeyFloatingcity)
            return &mn;
    }
    return NULL;
}

CFloatingcity *CFloatingcityMan::FindRandomNotInVec(std::vector<CTxIn> &vecToExclude, int protocolVersion)
{
    LOCK(cs);

    protocolVersion = protocolVersion == -1 ? floatingcityPayments.GetMinFloatingcityPaymentsProto() : protocolVersion;

    int nCountEnabled = CountEnabled(protocolVersion);
    LogPrintf("CFloatingcityMan::FindRandomNotInVec - nCountEnabled - vecToExclude.size() %d\n", nCountEnabled - vecToExclude.size());
    if(nCountEnabled - vecToExclude.size() < 1) return NULL;

    int rand = GetRandInt(nCountEnabled - vecToExclude.size());
    LogPrintf("CFloatingcityMan::FindRandomNotInVec - rand %d\n", rand);
    bool found;

    BOOST_FOREACH(CFloatingcity &mn, vFloatingcities) {
        if(mn.protocolVersion < protocolVersion || !mn.IsEnabled()) continue;
        found = false;
        BOOST_FOREACH(CTxIn &usedVin, vecToExclude) {
            if(mn.vin.prevout == usedVin.prevout) {
                found = true;
                break;
            }
        }
        if(found) continue;
        if(--rand < 1) {
            return &mn;
        }
    }

    return NULL;
}

CFloatingcity* CFloatingcityMan::GetCurrentFloatingCity(int mod, int64_t nBlockHeight, int minProtocol)
{
    unsigned int score = 0;
    CFloatingcity* winner = NULL;

    // scan for winner
    BOOST_FOREACH(CFloatingcity& mn, vFloatingcities) {
        mn.Check();
        if(mn.protocolVersion < minProtocol || !mn.IsEnabled()) continue;

        // calculate the score for each floatingcity
        uint256 n = mn.CalculateScore(mod, nBlockHeight);
        unsigned int n2 = 0;
        memcpy(&n2, &n, sizeof(n2));

        // determine the winner
        if(n2 > score){
            score = n2;
            winner = &mn;
        }
    }

    return winner;
}

int CFloatingcityMan::GetFloatingcityRank(const CTxIn& vin, int64_t nBlockHeight, int minProtocol, bool fOnlyActive)
{
    std::vector<pair<unsigned int, CTxIn> > vecFloatingcityScores;

    //make sure we know about this block
    uint256 hash = 0;
    if(!GetBlockHash(hash, nBlockHeight)) return -1;

    // scan for winner
    BOOST_FOREACH(CFloatingcity& mn, vFloatingcities) {

        if(mn.protocolVersion < minProtocol) continue;
        if(fOnlyActive) {
            mn.Check();
            if(!mn.IsEnabled()) continue;
        }

        uint256 n = mn.CalculateScore(1, nBlockHeight);
        unsigned int n2 = 0;
        memcpy(&n2, &n, sizeof(n2));

        vecFloatingcityScores.push_back(make_pair(n2, mn.vin));
    }

    sort(vecFloatingcityScores.rbegin(), vecFloatingcityScores.rend(), CompareValueOnly());

    int rank = 0;
    BOOST_FOREACH (PAIRTYPE(unsigned int, CTxIn)& s, vecFloatingcityScores){
        rank++;
        if(s.second == vin) {
            return rank;
        }
    }

    return -1;
}

std::vector<pair<int, CFloatingcity> > CFloatingcityMan::GetFloatingcityRanks(int64_t nBlockHeight, int minProtocol)
{
    std::vector<pair<unsigned int, CFloatingcity> > vecFloatingcityScores;
    std::vector<pair<int, CFloatingcity> > vecFloatingcityRanks;

    //make sure we know about this block
    uint256 hash = 0;
    if(!GetBlockHash(hash, nBlockHeight)) return vecFloatingcityRanks;

    // scan for winner
    BOOST_FOREACH(CFloatingcity& mn, vFloatingcities) {

        mn.Check();

        if(mn.protocolVersion < minProtocol) continue;
        if(!mn.IsEnabled()) {
            continue;
        }

        uint256 n = mn.CalculateScore(1, nBlockHeight);
        unsigned int n2 = 0;
        memcpy(&n2, &n, sizeof(n2));

        vecFloatingcityScores.push_back(make_pair(n2, mn));
    }

    sort(vecFloatingcityScores.rbegin(), vecFloatingcityScores.rend(), CompareValueOnlyMN());

    int rank = 0;
    BOOST_FOREACH (PAIRTYPE(unsigned int, CFloatingcity)& s, vecFloatingcityScores){
        rank++;
        vecFloatingcityRanks.push_back(make_pair(rank, s.second));
    }

    return vecFloatingcityRanks;
}

CFloatingcity* CFloatingcityMan::GetFloatingcityByRank(int nRank, int64_t nBlockHeight, int minProtocol, bool fOnlyActive)
{
    std::vector<pair<unsigned int, CTxIn> > vecFloatingcityScores;

    // scan for winner
    BOOST_FOREACH(CFloatingcity& mn, vFloatingcities) {

        if(mn.protocolVersion < minProtocol) continue;
        if(fOnlyActive) {
            mn.Check();
            if(!mn.IsEnabled()) continue;
        }

        uint256 n = mn.CalculateScore(1, nBlockHeight);
        unsigned int n2 = 0;
        memcpy(&n2, &n, sizeof(n2));

        vecFloatingcityScores.push_back(make_pair(n2, mn.vin));
    }

    sort(vecFloatingcityScores.rbegin(), vecFloatingcityScores.rend(), CompareValueOnly());

    int rank = 0;
    BOOST_FOREACH (PAIRTYPE(unsigned int, CTxIn)& s, vecFloatingcityScores){
        rank++;
        if(rank == nRank) {
            return Find(s.second);
        }
    }

    return NULL;
}

void CFloatingcityMan::ProcessFloatingcityConnections()
{
    LOCK(cs_vNodes);

    if(!mnEnginePool.pSubmittedToFloatingcity) return;
    
    BOOST_FOREACH(CNode* pnode, vNodes)
    {
        if(mnEnginePool.pSubmittedToFloatingcity->addr == pnode->addr) continue;

        if(pnode->fMNengineMaster){
            LogPrintf("Closing floatingcity connection %s \n", pnode->addr.ToString().c_str());
            pnode->CloseSocketDisconnect();
        }
    }
}

void CFloatingcityMan::ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{

    //Normally would disable functionality, NEED this enabled for staking.
    //if(fLiteMode) return;

    if(!mnEnginePool.IsBlockchainSynced()) return;

    LOCK(cs_process_message);

    if (strCommand == "dsee") { //MNengine Election Entry

        CTxIn vin;
        CService addr;
        CPubKey pubkey;
        CPubKey pubkey2;
        vector<unsigned char> vchSig;
        int64_t sigTime;
        int count;
        int current;
        int64_t lastUpdated;
        int protocolVersion;
        CScript donationAddress;
        int donationPercentage;
        std::string strMessage;

        // 70047 and greater
        vRecv >> vin >> addr >> vchSig >> sigTime >> pubkey >> pubkey2 >> count >> current >> lastUpdated >> protocolVersion >> donationAddress >> donationPercentage;

        // make sure signature isn't in the future (past is OK)
        if (sigTime > GetAdjustedTime() + 60 * 60) {
            LogPrintf("dsee - Signature rejected, too far into the future %s\n", vin.ToString().c_str());
            return;
        }

        bool isLocal = addr.IsRFC1918() || addr.IsLocal();
        //if(RegTest()) isLocal = false;

        std::string vchPubKey(pubkey.begin(), pubkey.end());
        std::string vchPubKey2(pubkey2.begin(), pubkey2.end());

        strMessage = addr.ToString() + boost::lexical_cast<std::string>(sigTime) + vchPubKey + vchPubKey2 + boost::lexical_cast<std::string>(protocolVersion)  + donationAddress.ToString() + boost::lexical_cast<std::string>(donationPercentage);

        if(donationPercentage < 0 || donationPercentage > 100){
            LogPrintf("dsee - donation percentage out of range %d\n", donationPercentage);
            return;     
        }
        if(protocolVersion < MIN_POOL_PEER_PROTO_VERSION) {
            LogPrintf("dsee - ignoring outdated floatingcity %s protocol version %d\n", vin.ToString().c_str(), protocolVersion);
            return;
        }

        CScript pubkeyScript;
        pubkeyScript.SetDestination(pubkey.GetID());

        if(pubkeyScript.size() != 25) {
            LogPrintf("dsee - pubkey the wrong size\n");
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        CScript pubkeyScript2;
        pubkeyScript2.SetDestination(pubkey2.GetID());

        if(pubkeyScript2.size() != 25) {
            LogPrintf("dsee - pubkey2 the wrong size\n");
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        if(!vin.scriptSig.empty()) {
            LogPrintf("dsee - Ignore Not Empty ScriptSig %s\n",vin.ToString().c_str());
            return;
        }

        std::string errorMessage = "";
        if(!mnEngineSigner.VerifyMessage(pubkey, vchSig, strMessage, errorMessage)){
            LogPrintf("dsee - Got bad floatingcity address signature\n");
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        //search existing floatingcity list, this is where we update existing floatingcities with new dsee broadcasts
        CFloatingcity* pmn = this->Find(vin);
        // if we are a floatingcity but with undefined vin and this dsee is ours (matches our Floatingcity privkey) then just skip this part
        if(pmn != NULL && !(fFloatingCity && activeFloatingcity.vin == CTxIn() && pubkey2 == activeFloatingcity.pubKeyFloatingcity))
        {
            // count == -1 when it's a new entry
            //   e.g. We don't want the entry relayed/time updated when we're syncing the list
            // mn.pubkey = pubkey, IsVinAssociatedWithPubkey is validated once below,
            //   after that they just need to match
            if(count == -1 && pmn->pubkey == pubkey && !pmn->UpdatedWithin(FLOATINGCITY_MIN_DSEE_SECONDS)){
                pmn->UpdateLastSeen();

                if(pmn->sigTime < sigTime){ //take the newest entry
                    if (!CheckNode((CAddress)addr)){
                        pmn->isPortOpen = false;
                    } else {
                        pmn->isPortOpen = true;
                        addrman.Add(CAddress(addr), pfrom->addr, 2*60*60); // use this as a peer
                    }
                    LogPrintf("dsee - Got updated entry for %s\n", addr.ToString().c_str());
                    pmn->pubkey2 = pubkey2;
                    pmn->sigTime = sigTime;
                    pmn->sig = vchSig;
                    pmn->protocolVersion = protocolVersion;
                    pmn->addr = addr;
                    pmn->donationAddress = donationAddress;
                    pmn->donationPercentage = donationPercentage;
                    pmn->Check();
                    if(pmn->IsEnabled())
                        mnodeman.RelayFloatingcityEntry(vin, addr, vchSig, sigTime, pubkey, pubkey2, count, current, lastUpdated, protocolVersion, donationAddress, donationPercentage);
                }
            }

            return;
        }

        // make sure the vout that was signed is related to the transaction that spawned the floatingcity
        //  - this is expensive, so it's only done once per floatingcity
        if(!mnEngineSigner.IsVinAssociatedWithPubkey(vin, pubkey)) {
            LogPrintf("dsee - Got mismatched pubkey and vin\n");
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        LogPrint("floatingcity", "dsee - Got NEW floatingcity entry %s\n", addr.ToString().c_str());

        // make sure it's still unspent
        //  - this is checked later by .check() in many places and by ThreadCheckMNenginePool()

        CValidationState state;
        CTransaction tx = CTransaction();
        CTxOut vout = CTxOut(MNengine_POOL_MAX, mnEnginePool.collateralPubKey);
        tx.vin.push_back(vin);
        tx.vout.push_back(vout);
        bool fAcceptable = false;
        {
            TRY_LOCK(cs_main, lockMain);
            if(!lockMain) return;
            fAcceptable = AcceptableInputs(mempool, tx, false, NULL);
        }
        if(fAcceptable){
            LogPrint("floatingcity", "dsee - Accepted floatingcity entry %i %i\n", count, current);

            if(GetInputAge(vin) < FLOATINGCITY_MIN_CONFIRMATIONS){
                LogPrintf("dsee - Input must have least %d confirmations\n", FLOATINGCITY_MIN_CONFIRMATIONS);
                Misbehaving(pfrom->GetId(), 20);
                return;
            }

            // verify that sig time is legit in past
            // should be at least not earlier than block when 10,000 Zalem-Coin tx got FLOATINGCITY_MIN_CONFIRMATIONS
            uint256 hashBlock = 0;
            GetTransaction(vin.prevout.hash, tx, hashBlock);
            map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hashBlock);
           if (mi != mapBlockIndex.end() && (*mi).second)
            {
                CBlockIndex* pMNIndex = (*mi).second; // block for 10,000 Zalem-Coin tx -> 1 confirmation
                CBlockIndex* pConfIndex = FindBlockByHeight((pMNIndex->nHeight + FLOATINGCITY_MIN_CONFIRMATIONS - 1)); // block where tx got FLOATINGCITY_MIN_CONFIRMATIONS
                if(pConfIndex->GetBlockTime() > sigTime)
                {
                    LogPrintf("dsee - Bad sigTime %d for floatingcity %20s %105s (%i conf block is at %d)\n",
                              sigTime, addr.ToString(), vin.ToString(), FLOATINGCITY_MIN_CONFIRMATIONS, pConfIndex->GetBlockTime());
                    return;
                }
            }


            // use this as a peer
            addrman.Add(CAddress(addr), pfrom->addr, 2*60*60);

            //doesn't support multisig addresses
            if(donationAddress.IsPayToScriptHash()){
                donationAddress = CScript();
                donationPercentage = 0;
            }

            // add our floatingcity
            CFloatingcity mn(addr, vin, pubkey, vchSig, sigTime, pubkey2, protocolVersion, donationAddress, donationPercentage);
            mn.UpdateLastSeen(lastUpdated);
            this->Add(mn);

            // if it matches our floatingcityprivkey, then we've been remotely activated
            if(pubkey2 == activeFloatingcity.pubKeyFloatingcity && protocolVersion == PROTOCOL_VERSION){
                activeFloatingcity.EnableHotColdFloatingCity(vin, addr);
            }

            if(count == -1 && !isLocal)
                mnodeman.RelayFloatingcityEntry(vin, addr, vchSig, sigTime, pubkey, pubkey2, count, current, lastUpdated, protocolVersion, donationAddress, donationPercentage);

        } else {
            LogPrintf("dsee - Rejected floatingcity entry %s\n", addr.ToString().c_str());

            int nDoS = 0;
            if (state.IsInvalid(nDoS))
            {
                LogPrintf("dsee - %s from %s %s was not accepted into the memory pool\n", tx.GetHash().ToString().c_str(),
                    pfrom->addr.ToString().c_str(), pfrom->cleanSubVer.c_str());
                if (nDoS > 0)
                    Misbehaving(pfrom->GetId(), nDoS);
            }
        }
    }

    else if (strCommand == "dseep") { //MNengine Election Entry Ping

        CTxIn vin;
        vector<unsigned char> vchSig;
        int64_t sigTime;
        bool stop;
        vRecv >> vin >> vchSig >> sigTime >> stop;

        //LogPrintf("dseep - Received: vin: %s sigTime: %lld stop: %s\n", vin.ToString().c_str(), sigTime, stop ? "true" : "false");

        if (sigTime > GetAdjustedTime() + 60 * 60) {
            LogPrintf("dseep - Signature rejected, too far into the future %s\n", vin.ToString().c_str());
            return;
        }

        if (sigTime <= GetAdjustedTime() - 60 * 60) {
            LogPrintf("dseep - Signature rejected, too far into the past %s - %d %d \n", vin.ToString().c_str(), sigTime, GetAdjustedTime());
            return;
        }

        // see if we have this floatingcity
        CFloatingcity* pmn = this->Find(vin);
        if(pmn != NULL && pmn->protocolVersion >= MIN_POOL_PEER_PROTO_VERSION)
        {
            // LogPrintf("dseep - Found corresponding mn for vin: %s\n", vin.ToString().c_str());
            // take this only if it's newer
            if(pmn->lastDseep < sigTime)
            {
                std::string strMessage = pmn->addr.ToString() + boost::lexical_cast<std::string>(sigTime) + boost::lexical_cast<std::string>(stop);

                std::string errorMessage = "";
                if(!mnEngineSigner.VerifyMessage(pmn->pubkey2, vchSig, strMessage, errorMessage))
                {
                    LogPrintf("dseep - Got bad floatingcity address signature %s \n", vin.ToString().c_str());
                    //Misbehaving(pfrom->GetId(), 100);
                    return;
                }

                pmn->lastDseep = sigTime;

                if(!pmn->UpdatedWithin(FLOATINGCITY_MIN_DSEEP_SECONDS))
                {
                    if(stop) pmn->Disable();
                    else
                    {
                        pmn->UpdateLastSeen();
                        pmn->Check();
                        if(!pmn->IsEnabled()) return;
                    }
                    mnodeman.RelayFloatingcityEntryPing(vin, vchSig, sigTime, stop);
                }
            }
            return;
        }

        LogPrint("floatingcity", "dseep - Couldn't find floatingcity entry %s\n", vin.ToString().c_str());

        std::map<COutPoint, int64_t>::iterator i = mWeAskedForFloatingcityListEntry.find(vin.prevout);
        if (i != mWeAskedForFloatingcityListEntry.end())
        {
            int64_t t = (*i).second;
            if (GetTime() < t) return; // we've asked recently
        }

        // ask for the dsee info once from the node that sent dseep

        LogPrintf("dseep - Asking source node for missing entry %s\n", vin.ToString().c_str());
        pfrom->PushMessage("dseg", vin);
        int64_t askAgain = GetTime()+ FLOATINGCITY_MIN_DSEEP_SECONDS;
        mWeAskedForFloatingcityListEntry[vin.prevout] = askAgain;

    } else if (strCommand == "mvote") { //Floatingcity Vote

        CTxIn vin;
        vector<unsigned char> vchSig;
        int nVote;
        vRecv >> vin >> vchSig >> nVote;

        // see if we have this Floatingcity
        CFloatingcity* pmn = this->Find(vin);
        if(pmn != NULL)
        {
            if((GetAdjustedTime() - pmn->lastVote) > (60*60))
            {
                std::string strMessage = vin.ToString() + boost::lexical_cast<std::string>(nVote);

                std::string errorMessage = "";
                if(!mnEngineSigner.VerifyMessage(pmn->pubkey2, vchSig, strMessage, errorMessage))
                {
                    LogPrintf("mvote - Got bad Floatingcity address signature %s \n", vin.ToString().c_str());
                    return;
                }

                pmn->nVote = nVote;
                pmn->lastVote = GetAdjustedTime();

                //send to all peers
                LOCK(cs_vNodes);
                BOOST_FOREACH(CNode* pnode, vNodes)
                    pnode->PushMessage("mvote", vin, vchSig, nVote);
            }

            return;
        }

    } else if (strCommand == "dseg") { //Get floatingcity list or specific entry

        CTxIn vin;
        vRecv >> vin;

        if(vin == CTxIn()) { //only should ask for this once
            //local network
            if(!pfrom->addr.IsRFC1918() && Params().NetworkID() == CChainParams::MAIN)
            {
                std::map<CNetAddr, int64_t>::iterator i = mAskedUsForFloatingcityList.find(pfrom->addr);
                if (i != mAskedUsForFloatingcityList.end())
                {
                    int64_t t = (*i).second;
                    if (GetTime() < t) {
                        Misbehaving(pfrom->GetId(), 34);
                        LogPrintf("dseg - peer already asked me for the list\n");
                        return;
                    }
                }

                int64_t askAgain = GetTime() + FLOATINGCITYS_DSEG_SECONDS;
                mAskedUsForFloatingcityList[pfrom->addr] = askAgain;
            }
        } //else, asking for a specific node which is ok

        int count = this->size();
        int i = 0;

        BOOST_FOREACH(CFloatingcity& mn, vFloatingcities) {

            if(mn.addr.IsRFC1918()) continue; //local network

            if(mn.IsEnabled())
            {
                LogPrint("floatingcity", "dseg - Sending floatingcity entry - %s \n", mn.addr.ToString().c_str());
                if(vin == CTxIn()){
                    pfrom->PushMessage("dsee", mn.vin, mn.addr, mn.sig, mn.sigTime, mn.pubkey, mn.pubkey2, count, i, mn.lastTimeSeen, mn.protocolVersion, mn.donationAddress, mn.donationPercentage);
                } else if (vin == mn.vin) {
                    pfrom->PushMessage("dsee", mn.vin, mn.addr, mn.sig, mn.sigTime, mn.pubkey, mn.pubkey2, count, i, mn.lastTimeSeen, mn.protocolVersion, mn.donationAddress, mn.donationPercentage);
                    LogPrintf("dseg - Sent 1 floatingcity entries to %s\n", pfrom->addr.ToString().c_str());
                    return;
                }
                i++;
            }
        }

        LogPrintf("dseg - Sent %d floatingcity entries to %s\n", i, pfrom->addr.ToString().c_str());
    }

}

void CFloatingcityMan::RelayFloatingcityEntry(const CTxIn vin, const CService addr, const std::vector<unsigned char> vchSig, const int64_t nNow, const CPubKey pubkey, const CPubKey pubkey2, const int count, const int current, const int64_t lastUpdated, const int protocolVersion, CScript donationAddress, int donationPercentage)
{
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes)
        pnode->PushMessage("dsee", vin, addr, vchSig, nNow, pubkey, pubkey2, count, current, lastUpdated, protocolVersion, donationAddress, donationPercentage);
}

void CFloatingcityMan::RelayFloatingcityEntryPing(const CTxIn vin, const std::vector<unsigned char> vchSig, const int64_t nNow, const bool stop)
{
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes)
        pnode->PushMessage("dseep", vin, vchSig, nNow, stop);
}

void CFloatingcityMan::Remove(CTxIn vin)
{
    LOCK(cs);

    vector<CFloatingcity>::iterator it = vFloatingcities.begin();
    while(it != vFloatingcities.end()){
        if((*it).vin == vin){
            LogPrint("floatingcity", "CFloatingcityMan: Removing Floatingcity %s - %i now\n", (*it).addr.ToString().c_str(), size() - 1);
            vFloatingcities.erase(it);
            break;
        }
    }
}

std::string CFloatingcityMan::ToString() const
{
    std::ostringstream info;

    info << "floatingcities: " << (int)vFloatingcities.size() <<
            ", peers who asked us for floatingcity list: " << (int)mAskedUsForFloatingcityList.size() <<
            ", peers we asked for floatingcity list: " << (int)mWeAskedForFloatingcityList.size() <<
            ", entries in Floatingcity list we asked for: " << (int)mWeAskedForFloatingcityListEntry.size() <<
            ", nDsqCount: " << (int)nDsqCount;

    return info.str();
}
