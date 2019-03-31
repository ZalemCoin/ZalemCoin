


#include "bignum.h"
#include "sync.h"
#include "net.h"
#include "key.h"
#include "util.h"
#include "script.h"
#include "base58.h"
#include "protocol.h"
#include "activefloatingcity.h"
#include "floatingcityman.h"
#include "spork.h"
#include <boost/lexical_cast.hpp>
#include "floatingcityman.h"

using namespace std;
using namespace boost;

std::map<uint256, CFloatingcityScanningError> mapFloatingcityScanningErrors;
CFloatingcityScanning mnscan;

/* 
    Floatingcity - Proof of Service 

    -- What it checks

    1.) Making sure Floatingcities have their ports open
    2.) Are responding to requests made by the network

    -- How it works

    When a block comes in, DoFloatingcityPOS is executed if the client is a 
    floatingcity. Using the deterministic ranking algorithm up to 1% of the floatingcity 
    network is checked each block. 

    A port is opened from Floatingcity A to Floatingcity B, if successful then nothing happens. 
    If there is an error, a CFloatingcityScanningError object is propagated with an error code.
    Errors are applied to the Floatingcities and a score is incremented within the floatingcity object,
    after a threshold is met, the floatingcity goes into an error state. Each cycle the score is 
    decreased, so if the floatingcity comes back online it will return to the list. 

    Floatingcities in a error state do not receive payment. 

    -- Future expansion

    We want to be able to prove the nodes have many qualities such as a specific CPU speed, bandwidth,
    and dedicated storage. E.g. We could require a full node be a computer running 2GHz with 10GB of space.

*/

void ProcessMessageFloatingcityPOS(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    if(!IsSporkActive(SPORK_7_FLOATINGCITY_SCANNING)) return;
    if(IsInitialBlockDownload()) return;

    if (strCommand == "mnse") //Floatingcity Scanning Error
    {

        CDataStream vMsg(vRecv);
        CFloatingcityScanningError mnse;
        vRecv >> mnse;

        CInv inv(MSG_FLOATINGCITY_SCANNING_ERROR, mnse.GetHash());
        pfrom->AddInventoryKnown(inv);

        if(mapFloatingcityScanningErrors.count(mnse.GetHash())){
            return;
        }
        mapFloatingcityScanningErrors.insert(make_pair(mnse.GetHash(), mnse));

        if(!mnse.IsValid())
        {
            LogPrintf("FloatingcityPOS::mnse - Invalid object\n");   
            return;
        }

        CFloatingcity* pmnA = mnodeman.Find(mnse.vinFloatingcityA);
        if(pmnA == NULL) return;
        if(pmnA->protocolVersion < MIN_FLOATINGCITY_POS_PROTO_VERSION) return;

        int nBlockHeight = pindexBest->nHeight;
        if(nBlockHeight - mnse.nBlockHeight > 10){
            LogPrintf("FloatingcityPOS::mnse - Too old\n");
            return;   
        }

        // Lowest floatingcities in rank check the highest each block
        int a = mnodeman.GetFloatingcityRank(mnse.vinFloatingcityA, mnse.nBlockHeight, MIN_FLOATINGCITY_POS_PROTO_VERSION);
        if(a == -1 || a > GetCountScanningPerBlock())
        {
            if(a != -1) LogPrintf("FloatingcityPOS::mnse - FloatingcityA ranking is too high\n");
            return;
        }

        int b = mnodeman.GetFloatingcityRank(mnse.vinFloatingcityB, mnse.nBlockHeight, MIN_FLOATINGCITY_POS_PROTO_VERSION, false);
        if(b == -1 || b < mnodeman.CountFloatingcitiesAboveProtocol(MIN_FLOATINGCITY_POS_PROTO_VERSION)-GetCountScanningPerBlock())
        {
            if(b != -1) LogPrintf("FloatingcityPOS::mnse - FloatingcityB ranking is too low\n");
            return;
        }

        if(!mnse.SignatureValid()){
            LogPrintf("FloatingcityPOS::mnse - Bad floatingcity message\n");
            return;
        }

        CFloatingcity* pmnB = mnodeman.Find(mnse.vinFloatingcityB);
        if(pmnB == NULL) return;

        if(fDebug) LogPrintf("ProcessMessageFloatingcityPOS::mnse - nHeight %d FloatingcityA %s FloatingcityB %s\n", mnse.nBlockHeight, pmnA->addr.ToString().c_str(), pmnB->addr.ToString().c_str());

        pmnB->ApplyScanningError(mnse);
        mnse.Relay();
    }
}

// Returns how many floatingcities are allowed to scan each block
int GetCountScanningPerBlock()
{
    return std::max(1, mnodeman.CountFloatingcitiesAboveProtocol(MIN_FLOATINGCITY_POS_PROTO_VERSION)/100);
}


void CFloatingcityScanning::CleanFloatingcityScanningErrors()
{
    if(pindexBest == NULL) return;

    std::map<uint256, CFloatingcityScanningError>::iterator it = mapFloatingcityScanningErrors.begin();

    while(it != mapFloatingcityScanningErrors.end()) {
        if(GetTime() > it->second.nExpiration){ //keep them for an hour
            LogPrintf("Removing old floatingcity scanning error %s\n", it->second.GetHash().ToString().c_str());

            mapFloatingcityScanningErrors.erase(it++);
        } else {
            it++;
        }
    }

}

// Check other floatingcities to make sure they're running correctly
void CFloatingcityScanning::DoFloatingcityPOSChecks()
{
    if(!fFloatingCity) return;
    if(!IsSporkActive(SPORK_7_FLOATINGCITY_SCANNING)) return;
    if(IsInitialBlockDownload()) return;

    int nBlockHeight = pindexBest->nHeight-5;

    int a = mnodeman.GetFloatingcityRank(activeFloatingcity.vin, nBlockHeight, MIN_FLOATINGCITY_POS_PROTO_VERSION);
    if(a == -1 || a > GetCountScanningPerBlock()){
        // we don't need to do anything this block
        return;
    }

    // The lowest ranking nodes (Floatingcity A) check the highest ranking nodes (Floatingcity B)
    CFloatingcity* pmn = mnodeman.GetFloatingcityByRank(mnodeman.CountFloatingcitiesAboveProtocol(MIN_FLOATINGCITY_POS_PROTO_VERSION)-a, nBlockHeight, MIN_FLOATINGCITY_POS_PROTO_VERSION, false);
    if(pmn == NULL) return;

    // -- first check : Port is open

    if(!ConnectNode((CAddress)pmn->addr, NULL, true)){
        // we couldn't connect to the node, let's send a scanning error
        CFloatingcityScanningError mnse(activeFloatingcity.vin, pmn->vin, SCANNING_ERROR_NO_RESPONSE, nBlockHeight);
        mnse.Sign();
        mapFloatingcityScanningErrors.insert(make_pair(mnse.GetHash(), mnse));
        mnse.Relay();
    }

    // success
    CFloatingcityScanningError mnse(activeFloatingcity.vin, pmn->vin, SCANNING_SUCCESS, nBlockHeight);
    mnse.Sign();
    mapFloatingcityScanningErrors.insert(make_pair(mnse.GetHash(), mnse));
    mnse.Relay();
}

bool CFloatingcityScanningError::SignatureValid()
{
    std::string errorMessage;
    std::string strMessage = vinFloatingcityA.ToString() + vinFloatingcityB.ToString() + 
        boost::lexical_cast<std::string>(nBlockHeight) + boost::lexical_cast<std::string>(nErrorType);

    CFloatingcity* pmn = mnodeman.Find(vinFloatingcityA);

    if(pmn == NULL)
    {
        LogPrintf("CFloatingcityScanningError::SignatureValid() - Unknown Floatingcity\n");
        return false;
    }

    CScript pubkey;
    pubkey.SetDestination(pmn->pubkey2.GetID());
    CTxDestination address1;
    ExtractDestination(pubkey, address1);
    CZalem-CoinAddress address2(address1);

    if(!darkSendSigner.VerifyMessage(pmn->pubkey2, vchFloatingCitySignature, strMessage, errorMessage)) {
        LogPrintf("CFloatingcityScanningError::SignatureValid() - Verify message failed\n");
        return false;
    }

    return true;
}

bool CFloatingcityScanningError::Sign()
{
    std::string errorMessage;

    CKey key2;
    CPubKey pubkey2;
    std::string strMessage = vinFloatingcityA.ToString() + vinFloatingcityB.ToString() + 
        boost::lexical_cast<std::string>(nBlockHeight) + boost::lexical_cast<std::string>(nErrorType);

    if(!darkSendSigner.SetKey(strFloatingCityPrivKey, errorMessage, key2, pubkey2))
    {
        LogPrintf("CFloatingcityScanningError::Sign() - ERROR: Invalid floatingcityprivkey: '%s'\n", errorMessage.c_str());
        return false;
    }

    CScript pubkey;
    pubkey.SetDestination(pubkey2.GetID());
    CTxDestination address1;
    ExtractDestination(pubkey, address1);
    CZalem-CoinAddress address2(address1);
    //LogPrintf("signing pubkey2 %s \n", address2.ToString().c_str());

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchFloatingCitySignature, key2)) {
        LogPrintf("CFloatingcityScanningError::Sign() - Sign message failed");
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pubkey2, vchFloatingCitySignature, strMessage, errorMessage)) {
        LogPrintf("CFloatingcityScanningError::Sign() - Verify message failed");
        return false;
    }

    return true;
}

void CFloatingcityScanningError::Relay()
{
    CInv inv(MSG_FLOATINGCITY_SCANNING_ERROR, GetHash());

    vector<CInv> vInv;
    vInv.push_back(inv);
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes){
        pnode->PushMessage("inv", vInv);
    }
}
