// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The DarkCoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef ACTIVEFLOATINGCITY_H
#define ACTIVEFLOATINGCITY_H

#include "uint256.h"
#include "sync.h"
#include "net.h"
#include "key.h"
#include "floatingcity.h"
#include "main.h"
#include "init.h"
#include "wallet.h"
#include "fcengine.h"

// Responsible for activating the floatingcity and pinging the network
class CActiveFloatingcity
{
public:
	// Initialized by init.cpp
	// Keys for the main floatingcity
	CPubKey pubKeyFloatingcity;

	// Initialized while registering floatingcity
	CTxIn vin;
    CService service;

    int status;
    std::string notCapableReason;

    CActiveFloatingcity()
    {        
        status = FLOATINGCITY_NOT_PROCESSED;
    }

    void ManageStatus(); // manage status of main floatingcity

    bool Dseep(std::string& errorMessage); // ping for main floatingcity
    bool Dseep(CTxIn vin, CService service, CKey key, CPubKey pubKey, std::string &retErrorMessage, bool stop); // ping for any floatingcity

    bool StopFloatingCity(std::string& errorMessage); // stop main floatingcity
    bool StopFloatingCity(std::string strService, std::string strKeyFloatingcity, std::string& errorMessage); // stop remote floatingcity
    bool StopFloatingCity(CTxIn vin, CService service, CKey key, CPubKey pubKey, std::string& errorMessage); // stop any floatingcity

    /// Register remote Floatingcity
    bool Register(std::string strService, std::string strKey, std::string txHash, std::string strOutputIndex, std::string strDonationAddress, std::string strDonationPercentage, std::string& errorMessage); 
    /// Register any Floatingcity
    bool Register(CTxIn vin, CService service, CKey key, CPubKey pubKey, CKey keyFloatingcity, CPubKey pubKeyFloatingcity, CScript donationAddress, int donationPercentage, std::string &retErrorMessage);  

    // get 10,000 ZLM input that can be used for the floatingcity
    bool GetFloatingCityVin(CTxIn& vin, CPubKey& pubkey, CKey& secretKey);
    bool GetFloatingCityVin(CTxIn& vin, CPubKey& pubkey, CKey& secretKey, std::string strTxHash, std::string strOutputIndex);
    bool GetFloatingCityVinForPubKey(std::string collateralAddress, CTxIn& vin, CPubKey& pubkey, CKey& secretKey);
    bool GetFloatingCityVinForPubKey(std::string collateralAddress, CTxIn& vin, CPubKey& pubkey, CKey& secretKey, std::string strTxHash, std::string strOutputIndex);
    vector<COutput> SelectCoinsFloatingcity();
    vector<COutput> SelectCoinsFloatingcityForPubKey(std::string collateralAddress);
    bool GetVinFromOutput(COutput out, CTxIn& vin, CPubKey& pubkey, CKey& secretKey);

    // enable hot wallet mode (run a floatingcity with no funds)
    bool EnableHotColdFloatingCity(CTxIn& vin, CService& addr);
};

#endif
