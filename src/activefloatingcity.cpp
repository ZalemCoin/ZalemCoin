// Copyright (c) 2009-2012 The Darkcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "protocol.h"
#include "activefloatingcity.h"
#include "floatingcityman.h"
#include <boost/lexical_cast.hpp>
#include "clientversion.h"

//
// Bootup the floatingcity, look for a 10,000 ZLM input and register on the network
//
void CActiveFloatingcity::ManageStatus()
{
    std::string errorMessage;

    if (fDebug) LogPrintf("CActiveFloatingcity::ManageStatus() - Begin\n");

    if(!fFloatingCity) return;

    //need correct adjusted time to send ping
    bool fIsInitialDownload = IsInitialBlockDownload();
    if(fIsInitialDownload) {
        status = FLOATINGCITY_SYNC_IN_PROCESS;
        LogPrintf("CActiveFloatingcity::ManageStatus() - Sync in progress. Must wait until sync is complete to start floatingcity.\n");
        return;
    }

    if(status == FLOATINGCITY_INPUT_TOO_NEW || status == FLOATINGCITY_NOT_CAPABLE || status == FLOATINGCITY_SYNC_IN_PROCESS){
        status = FLOATINGCITY_NOT_PROCESSED;
    }

    if(status == FLOATINGCITY_NOT_PROCESSED) {
        if(strFloatingCityAddr.empty()) {
            if(!GetLocal(service)) {
                notCapableReason = "Can't detect external address. Please use the floatingcityaddr configuration option.";
                status = FLOATINGCITY_NOT_CAPABLE;
                LogPrintf("CActiveFloatingcity::ManageStatus() - not capable: %s\n", notCapableReason.c_str());
                return;
            }
        } else {
        	service = CService(strFloatingCityAddr, true);
        }

        LogPrintf("CActiveFloatingcity::ManageStatus() - Checking inbound connection to '%s'\n", service.ToString().c_str());


            if(!ConnectNode((CAddress)service, service.ToString().c_str())){
                notCapableReason = "Could not connect to " + service.ToString();
                status = FLOATINGCITY_NOT_CAPABLE;
                LogPrintf("CActiveFloatingcity::ManageStatus() - not capable: %s\n", notCapableReason.c_str());
                return;
            }


        if(pwalletMain->IsLocked()){
            notCapableReason = "Wallet is locked.";
            status = FLOATINGCITY_NOT_CAPABLE;
            LogPrintf("CActiveFloatingcity::ManageStatus() - not capable: %s\n", notCapableReason.c_str());
            return;
        }

        // Set defaults
        status = FLOATINGCITY_NOT_CAPABLE;
        notCapableReason = "Unknown. Check debug.log for more information.\n";

        // Choose coins to use
        CPubKey pubKeyCollateralAddress;
        CKey keyCollateralAddress;

        if(GetFloatingCityVin(vin, pubKeyCollateralAddress, keyCollateralAddress)) {

            if(GetInputAge(vin) < FLOATINGCITY_MIN_CONFIRMATIONS){
                notCapableReason = "Input must have least " + boost::lexical_cast<string>(FLOATINGCITY_MIN_CONFIRMATIONS) +
                        " confirmations - " + boost::lexical_cast<string>(GetInputAge(vin)) + " confirmations";
                LogPrintf("CActiveFloatingcity::ManageStatus() - %s\n", notCapableReason.c_str());
                status = FLOATINGCITY_INPUT_TOO_NEW;
                return;
            }

            LogPrintf("CActiveFloatingcity::ManageStatus() - Is capable floating city!\n");

            status = FLOATINGCITY_IS_CAPABLE;
            notCapableReason = "";

            pwalletMain->LockCoin(vin.prevout);

            // send to all nodes
            CPubKey pubKeyFloatingcity;
            CKey keyFloatingcity;

            if(!mnEngineSigner.SetKey(strFloatingCityPrivKey, errorMessage, keyFloatingcity, pubKeyFloatingcity))
            {
            	LogPrintf("ActiveFloatingcity::Dseep() - Error upon calling SetKey: %s\n", errorMessage.c_str());
            	return;
            }

            /* donations are not supported in Zalem-Coin.conf */
            CScript donationAddress = CScript();
            int donationPercentage = 0;

            if(!Register(vin, service, keyCollateralAddress, pubKeyCollateralAddress, keyFloatingcity, pubKeyFloatingcity, donationAddress, donationPercentage, errorMessage)) {
                LogPrintf("CActiveFloatingcity::ManageStatus() - Error on Register: %s\n", errorMessage.c_str());
            }

            return;
        } else {
            notCapableReason = "Could not find suitable coins!";
        	LogPrintf("CActiveFloatingcity::ManageStatus() - Could not find suitable coins!\n");
        }
    }

    //send to all peers
    if(!Dseep(errorMessage)) {
    	LogPrintf("CActiveFloatingcity::ManageStatus() - Error on Ping: %s\n", errorMessage.c_str());
    }
}

// Send stop dseep to network for remote floatingcity
bool CActiveFloatingcity::StopFloatingCity(std::string strService, std::string strKeyFloatingcity, std::string& errorMessage) {
	CTxIn vin;
    CKey keyFloatingcity;
    CPubKey pubKeyFloatingcity;

    if(!mnEngineSigner.SetKey(strKeyFloatingcity, errorMessage, keyFloatingcity, pubKeyFloatingcity)) {
    	LogPrintf("CActiveFloatingcity::StopFloatingCity() - Error: %s\n", errorMessage.c_str());
		return false;
	}

    if (GetFloatingCityVin(vin, pubKeyFloatingcity, keyFloatingcity)){
        LogPrintf("FloatingcityStop::VinFound: %s\n", vin.ToString());
    }

	return StopFloatingCity(vin, CService(strService, true), keyFloatingcity, pubKeyFloatingcity, errorMessage);
}

// Send stop dseep to network for main floatingcity
bool CActiveFloatingcity::StopFloatingCity(std::string& errorMessage) {
	if(status != FLOATINGCITY_IS_CAPABLE && status != FLOATINGCITY_REMOTELY_ENABLED) {
		errorMessage = "floatingcity is not in a running status";
    	LogPrintf("CActiveFloatingcity::StopFloatingCity() - Error: %s\n", errorMessage.c_str());
		return false;
	}

	status = FLOATINGCITY_STOPPED;

    CPubKey pubKeyFloatingcity;
    CKey keyFloatingcity;

    if(!mnEngineSigner.SetKey(strFloatingCityPrivKey, errorMessage, keyFloatingcity, pubKeyFloatingcity))
    {
    	LogPrintf("Register::ManageStatus() - Error upon calling SetKey: %s\n", errorMessage.c_str());
    	return false;
    }

	return StopFloatingCity(vin, service, keyFloatingcity, pubKeyFloatingcity, errorMessage);
}

// Send stop dseep to network for any floatingcity
bool CActiveFloatingcity::StopFloatingCity(CTxIn vin, CService service, CKey keyFloatingcity, CPubKey pubKeyFloatingcity, std::string& errorMessage) {
   	pwalletMain->UnlockCoin(vin.prevout);
	return Dseep(vin, service, keyFloatingcity, pubKeyFloatingcity, errorMessage, true);
}

bool CActiveFloatingcity::Dseep(std::string& errorMessage) {
	if(status != FLOATINGCITY_IS_CAPABLE && status != FLOATINGCITY_REMOTELY_ENABLED) {
		errorMessage = "floatingcity is not in a running status";
    	LogPrintf("CActiveFloatingcity::Dseep() - Error: %s\n", errorMessage.c_str());
		return false;
	}

    CPubKey pubKeyFloatingcity;
    CKey keyFloatingcity;

    if(!mnEngineSigner.SetKey(strFloatingCityPrivKey, errorMessage, keyFloatingcity, pubKeyFloatingcity))
    {
    	LogPrintf("CActiveFloatingcity::Dseep() - Error upon calling SetKey: %s\n", errorMessage.c_str());
    	return false;
    }

	return Dseep(vin, service, keyFloatingcity, pubKeyFloatingcity, errorMessage, false);
}

bool CActiveFloatingcity::Dseep(CTxIn vin, CService service, CKey keyFloatingcity, CPubKey pubKeyFloatingcity, std::string &retErrorMessage, bool stop) {
    std::string errorMessage;
    std::vector<unsigned char> vchFloatingCitySignature;
    std::string strFloatingCitySignMessage;
    int64_t masterNodeSignatureTime = GetAdjustedTime();

    std::string strMessage = service.ToString() + boost::lexical_cast<std::string>(masterNodeSignatureTime) + boost::lexical_cast<std::string>(stop);

    if(!mnEngineSigner.SignMessage(strMessage, errorMessage, vchFloatingCitySignature, keyFloatingcity)) {
    	retErrorMessage = "sign message failed: " + errorMessage;
    	LogPrintf("CActiveFloatingcity::Dseep() - Error: %s\n", retErrorMessage.c_str());
        return false;
    }

    if(!mnEngineSigner.VerifyMessage(pubKeyFloatingcity, vchFloatingCitySignature, strMessage, errorMessage)) {
    	retErrorMessage = "Verify message failed: " + errorMessage;
    	LogPrintf("CActiveFloatingcity::Dseep() - Error: %s\n", retErrorMessage.c_str());
        return false;
    }

    // Update Last Seen timestamp in floatingcity list
    CFloatingcity* pmn = mnodeman.Find(vin);
    if(pmn != NULL)
    {
        if(stop)
            mnodeman.Remove(pmn->vin);
        else
            pmn->UpdateLastSeen();
    } else {
    	// Seems like we are trying to send a ping while the floatingcity is not registered in the network
        retErrorMessage = "MNengine Floatingcity List doesn't include our floatingcity, Shutting down floatingcity pinging service! " + vin.ToString();
    	LogPrintf("CActiveFloatingcity::Dseep() - Error: %s\n", retErrorMessage.c_str());
        status = FLOATINGCITY_NOT_CAPABLE;
        notCapableReason = retErrorMessage;
        return false;
    }

    //send to all peers
    LogPrintf("CActiveFloatingcity::Dseep() - RelayFloatingcityEntryPing vin = %s\n", vin.ToString().c_str());
    mnodeman.RelayFloatingcityEntryPing(vin, vchFloatingCitySignature, masterNodeSignatureTime, stop);

    return true;
}

bool CActiveFloatingcity::Register(std::string strService, std::string strKeyFloatingcity, std::string txHash, std::string strOutputIndex, std::string strDonationAddress, std::string strDonationPercentage, std::string& errorMessage) {
    CTxIn vin;
    CPubKey pubKeyCollateralAddress;
    CKey keyCollateralAddress;
    CPubKey pubKeyFloatingcity;
    CKey keyFloatingcity;
    CScript donationAddress = CScript();
    int donationPercentage = 0;

    if(!mnEngineSigner.SetKey(strKeyFloatingcity, errorMessage, keyFloatingcity, pubKeyFloatingcity))
    {
        LogPrintf("CActiveFloatingcity::Register() - Error upon calling SetKey: %s\n", errorMessage.c_str());
        return false;
    }

    if(!GetFloatingCityVin(vin, pubKeyCollateralAddress, keyCollateralAddress, txHash, strOutputIndex)) {
        errorMessage = "could not allocate vin";
        LogPrintf("CActiveFloatingcity::Register() - Error: %s\n", errorMessage.c_str());
        return false;
    }
    CZalemCoinAddress address;
    if (strDonationAddress != "")
    {
        if(!address.SetString(strDonationAddress))
        {
            LogPrintf("ActiveFloatingcity::Register - Invalid Donation Address\n");
            return false;
        }
        donationAddress.SetDestination(address.Get());

        try {
            donationPercentage = boost::lexical_cast<int>( strDonationPercentage );
        } catch( boost::bad_lexical_cast const& ) {
            LogPrintf("ActiveFloatingcity::Register - Invalid Donation Percentage (Couldn't cast)\n");
            return false;
        }

        if(donationPercentage < 0 || donationPercentage > 100)
        {
            LogPrintf("ActiveFloatingcity::Register - Donation Percentage Out Of Range\n");
            return false;
        }
    }

	return Register(vin, CService(strService, true), keyCollateralAddress, pubKeyCollateralAddress, keyFloatingcity, pubKeyFloatingcity, donationAddress, donationPercentage, errorMessage);
}

bool CActiveFloatingcity::Register(CTxIn vin, CService service, CKey keyCollateralAddress, CPubKey pubKeyCollateralAddress, CKey keyFloatingcity, CPubKey pubKeyFloatingcity, CScript donationAddress, int donationPercentage, std::string &retErrorMessage) {
    std::string errorMessage;
    std::vector<unsigned char> vchFloatingCitySignature;
    std::string strFloatingCitySignMessage;
    int64_t masterNodeSignatureTime = GetAdjustedTime();

    std::string vchPubKey(pubKeyCollateralAddress.begin(), pubKeyCollateralAddress.end());
    std::string vchPubKey2(pubKeyFloatingcity.begin(), pubKeyFloatingcity.end());

    std::string strMessage = service.ToString() + boost::lexical_cast<std::string>(masterNodeSignatureTime) + vchPubKey + vchPubKey2 + boost::lexical_cast<std::string>(PROTOCOL_VERSION) + donationAddress.ToString() + boost::lexical_cast<std::string>(donationPercentage);

    if(!mnEngineSigner.SignMessage(strMessage, errorMessage, vchFloatingCitySignature, keyCollateralAddress)) {
		retErrorMessage = "sign message failed: " + errorMessage;
		LogPrintf("CActiveFloatingcity::Register() - Error: %s\n", retErrorMessage.c_str());
		return false;
    }

    if(!mnEngineSigner.VerifyMessage(pubKeyCollateralAddress, vchFloatingCitySignature, strMessage, errorMessage)) {
		retErrorMessage = "Verify message failed: " + errorMessage;
		LogPrintf("CActiveFloatingcity::Register() - Error: %s\n", retErrorMessage.c_str());
		return false;
	}

    CFloatingcity* pmn = mnodeman.Find(vin);
    if(pmn == NULL)
    {
        LogPrintf("CActiveFloatingcity::Register() - Adding to floatingcity list service: %s - vin: %s\n", service.ToString().c_str(), vin.ToString().c_str());
        CFloatingcity mn(service, vin, pubKeyCollateralAddress, vchFloatingCitySignature, masterNodeSignatureTime, pubKeyFloatingcity, PROTOCOL_VERSION, donationAddress, donationPercentage);
        mn.ChangeNodeStatus(false);
        mn.UpdateLastSeen(masterNodeSignatureTime);
        mnodeman.Add(mn);
    }

    //send to all peers
    LogPrintf("CActiveFloatingcity::Register() - RelayElectionEntry vin = %s\n", vin.ToString().c_str());
    mnodeman.RelayFloatingcityEntry(vin, service, vchFloatingCitySignature, masterNodeSignatureTime, pubKeyCollateralAddress, pubKeyFloatingcity, -1, -1, masterNodeSignatureTime, PROTOCOL_VERSION, donationAddress, donationPercentage);

    return true;
}

bool CActiveFloatingcity::GetFloatingCityVin(CTxIn& vin, CPubKey& pubkey, CKey& secretKey) {
	return GetFloatingCityVin(vin, pubkey, secretKey, "", "");
}

bool CActiveFloatingcity::GetFloatingCityVin(CTxIn& vin, CPubKey& pubkey, CKey& secretKey, std::string strTxHash, std::string strOutputIndex) {
    CScript pubScript;

    // Find possible candidates
    vector<COutput> possibleCoins = SelectCoinsFloatingcity();
    COutput *selectedOutput;

    // Find the vin
	if(!strTxHash.empty()) {
		// Let's find it
		uint256 txHash(strTxHash);
        int outputIndex = boost::lexical_cast<int>(strOutputIndex);
		bool found = false;
		BOOST_FOREACH(COutput& out, possibleCoins) {
			if(out.tx->GetHash() == txHash && out.i == outputIndex)
			{
				selectedOutput = &out;
				found = true;
				break;
			}
		}
		if(!found) {
			LogPrintf("CActiveFloatingcity::GetFloatingCityVin - Could not locate valid vin\n");
			return false;
		}
	} else {
		// No output specified,  Select the first one
		if(possibleCoins.size() > 0) {
			selectedOutput = &possibleCoins[0];
		} else {
			LogPrintf("CActiveFloatingcity::GetFloatingCityVin - Could not locate specified vin from possible list\n");
			return false;
		}
    }

	// At this point we have a selected output, retrieve the associated info
	return GetVinFromOutput(*selectedOutput, vin, pubkey, secretKey);
}

bool CActiveFloatingcity::GetFloatingCityVinForPubKey(std::string collateralAddress, CTxIn& vin, CPubKey& pubkey, CKey& secretKey) {
	return GetFloatingCityVinForPubKey(collateralAddress, vin, pubkey, secretKey, "", "");
}

bool CActiveFloatingcity::GetFloatingCityVinForPubKey(std::string collateralAddress, CTxIn& vin, CPubKey& pubkey, CKey& secretKey, std::string strTxHash, std::string strOutputIndex) {
    CScript pubScript;

    // Find possible candidates
    vector<COutput> possibleCoins = SelectCoinsFloatingcityForPubKey(collateralAddress);
    COutput *selectedOutput;

    // Find the vin
	if(!strTxHash.empty()) {
		// Let's find it
		uint256 txHash(strTxHash);
        int outputIndex = boost::lexical_cast<int>(strOutputIndex);
		bool found = false;
		BOOST_FOREACH(COutput& out, possibleCoins) {
			if(out.tx->GetHash() == txHash && out.i == outputIndex)
			{
				selectedOutput = &out;
				found = true;
				break;
			}
		}
		if(!found) {
			LogPrintf("CActiveFloatingcity::GetFloatingCityVinForPubKey - Could not locate valid vin\n");
			return false;
		}
	} else {
		// No output specified,  Select the first one
		if(possibleCoins.size() > 0) {
			selectedOutput = &possibleCoins[0];
		} else {
			LogPrintf("CActiveFloatingcity::GetFloatingCityVinForPubKey - Could not locate specified vin from possible list\n");
			return false;
		}
    }

	// At this point we have a selected output, retrieve the associated info
	return GetVinFromOutput(*selectedOutput, vin, pubkey, secretKey);
}


// Extract floatingcity vin information from output
bool CActiveFloatingcity::GetVinFromOutput(COutput out, CTxIn& vin, CPubKey& pubkey, CKey& secretKey) {

    CScript pubScript;

	vin = CTxIn(out.tx->GetHash(),out.i);
    pubScript = out.tx->vout[out.i].scriptPubKey; // the inputs PubKey

	CTxDestination address1;
    ExtractDestination(pubScript, address1);
    CZalemCoinAddress address2(address1);

    CKeyID keyID;
    if (!address2.GetKeyID(keyID)) {
        LogPrintf("CActiveFloatingcity::GetFloatingCityVin - Address does not refer to a key\n");
        return false;
    }

    if (!pwalletMain->GetKey(keyID, secretKey)) {
        LogPrintf ("CActiveFloatingcity::GetFloatingCityVin - Private key for address is not known\n");
        return false;
    }

    pubkey = secretKey.GetPubKey();
    return true;
}

// get all possible outputs for running floatingcity
vector<COutput> CActiveFloatingcity::SelectCoinsFloatingcity()
{
    vector<COutput> vCoins;
    vector<COutput> filteredCoins;

    // Retrieve all possible outputs
    pwalletMain->AvailableCoinsMN(vCoins);

    // Filter
    BOOST_FOREACH(const COutput& out, vCoins)
    {
        if(out.tx->vout[out.i].nValue == FloatingcityCollateral(pindexBest->nHeight)*COIN) { //exactly
        	filteredCoins.push_back(out);
        }
    }
    return filteredCoins;
}

// get all possible outputs for running floatingcity for a specific pubkey
vector<COutput> CActiveFloatingcity::SelectCoinsFloatingcityForPubKey(std::string collateralAddress)
{
    CZalemCoinAddress address(collateralAddress);
    CScript scriptPubKey;
    scriptPubKey.SetDestination(address.Get());
    vector<COutput> vCoins;
    vector<COutput> filteredCoins;

    // Retrieve all possible outputs
    pwalletMain->AvailableCoins(vCoins);

    // Filter
    BOOST_FOREACH(const COutput& out, vCoins)
    {
        if(out.tx->vout[out.i].scriptPubKey == scriptPubKey && out.tx->vout[out.i].nValue == FloatingcityCollateral(pindexBest->nHeight)*COIN) { //exactly
        	filteredCoins.push_back(out);
        }
    }
    return filteredCoins;
}

// when starting a floatingcity, this can enable to run as a hot wallet with no funds
bool CActiveFloatingcity::EnableHotColdFloatingCity(CTxIn& newVin, CService& newService)
{
    if(!fFloatingCity) return false;

    status = FLOATINGCITY_REMOTELY_ENABLED;
    notCapableReason = "";

    //The values below are needed for signing dseep messages going forward
    this->vin = newVin;
    this->service = newService;

    LogPrintf("CActiveFloatingcity::EnableHotColdFloatingCity() - Enabled! You may shut down the cold daemon.\n");

    return true;
}
