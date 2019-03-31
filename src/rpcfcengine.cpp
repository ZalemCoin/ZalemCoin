// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Darkcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "chain.h"
#include "db.h"
#include "init.h"
#include "activefloatingcity.h"
#include "floatingcityman.h"
#include "floatingcityconfig.h"
#include "rpcserver.h"
#include <boost/lexical_cast.hpp>
//#include "amount.h"
#include "util.h"
//#include "utilmoneystr.h"

#include <fstream>
using namespace json_spirit;
using namespace std;

void SendMoney(const CTxDestination &address, CAmount nValue, CWalletTx& wtxNew, AvailableCoinsType coin_type=ALL_COINS)
{
    // Check amount
    if (nValue <= 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid amount");

    if (nValue > pwalletMain->GetBalance())
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Insufficient funds");

    string strError;
    if (pwalletMain->IsLocked())
    {
        strError = "Error: Wallet locked, unable to create transaction!";
        LogPrintf("SendMoney() : %s", strError);
        throw JSONRPCError(RPC_WALLET_ERROR, strError);
    }

    // Parse Zalem-Coin address
    CScript scriptPubKey = GetScriptForDestination(address);

    // Create and send the transaction
    CReserveKey reservekey(pwalletMain);
    int64_t nFeeRequired;
    std::string sNarr;
    if (!pwalletMain->CreateTransaction(scriptPubKey, nValue, sNarr, wtxNew, reservekey, nFeeRequired, NULL))
    {
        if (nValue + nFeeRequired > pwalletMain->GetBalance())
            strError = strprintf("Error: This transaction requires a transaction fee of at least %s because of its amount, complexity, or use of recently received funds!", FormatMoney(nFeeRequired));
        LogPrintf("SendMoney() : %s\n", strError);
        throw JSONRPCError(RPC_WALLET_ERROR, strError);
    }
    if (!pwalletMain->CommitTransaction(wtxNew, reservekey))
        throw JSONRPCError(RPC_WALLET_ERROR, "Error: The transaction was rejected! This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.");
}


Value floatingcity(const Array& params, bool fHelp)
{
    string strCommand;
    if (params.size() >= 1)
        strCommand = params[0].get_str();

    if (fHelp  ||
        (strCommand != "count" && strCommand != "current" && strCommand != "debug" && strCommand != "genkey" && strCommand != "enforce" && strCommand != "list" && strCommand != "list-conf"
        	&& strCommand != "start" && strCommand != "start-alias" && strCommand != "start-many" && strCommand != "status" && strCommand != "stop" && strCommand != "stop-alias"
                && strCommand != "stop-many" && strCommand != "winners" && strCommand != "connect" && strCommand != "outputs" && strCommand != "vote-many" && strCommand != "vote"))
        throw runtime_error(
                "floatingcity \"command\"... ( \"passphrase\" )\n"
                "Set of commands to execute floatingcity related actions\n"
                "\nArguments:\n"
                "1. \"command\"        (string or set of strings, required) The command to execute\n"
                "2. \"passphrase\"     (string, optional) The wallet passphrase\n"
                "\nAvailable commands:\n"
                "  count        - Print number of all known floatingcities (optional: 'enabled', 'both')\n"
                "  current      - Print info on current floatingcity winner\n"
                "  debug        - Print floatingcity status\n"
                "  genkey       - Generate new floatingcityprivkey\n"
                "  enforce      - Enforce floatingcity payments\n"
                "  list         - Print list of all known floatingcities (see floatingcitylist for more info)\n"
                "  list-conf    - Print floatingcity.conf in JSON format\n"
                "  outputs      - Print floatingcity compatible outputs\n"
                "  start        - Start floatingcity configured in Zalem-Coin.conf\n"
                "  start-alias  - Start single floatingcity by assigned alias configured in floatingcity.conf\n"
                "  start-many   - Start all floatingcities configured in floatingcity.conf\n"
                "  status       - Print floatingcity status information\n"
                "  stop         - Stop floatingcity configured in Zalem-Coin.conf\n"
                "  stop-alias   - Stop single floatingcity by assigned alias configured in floatingcity.conf\n"
                "  stop-many    - Stop all floatingcities configured in floatingcity.conf\n"
                "  winners      - Print list of floatingcity winners\n"
                "  vote-many    - Vote on a Zalem-Coin initiative\n"
                "  vote         - Vote on a Zalem-Coin initiative\n"
                );

    if (strCommand == "stop")
    {
        if(!fFloatingCity) return "you must set floatingcity=1 in the configuration";

        if(pwalletMain->IsLocked()) {
            SecureString strWalletPass;
            strWalletPass.reserve(100);

            if (params.size() == 2){
                strWalletPass = params[1].get_str().c_str();
            } else {
                throw runtime_error(
                    "Your wallet is locked, passphrase is required\n");
            }

            if(!pwalletMain->Unlock(strWalletPass)){
                return "incorrect passphrase";
            }
        }

        std::string errorMessage;
        if(!activeFloatingcity.StopFloatingCity(errorMessage)) {
        	return "stop failed: " + errorMessage;
        }
        pwalletMain->Lock();

        if(activeFloatingcity.status == FLOATINGCITY_STOPPED) return "successfully stopped floatingcity";
        if(activeFloatingcity.status == FLOATINGCITY_NOT_CAPABLE) return "not capable floatingcity";

        return "unknown";
    }

    if (strCommand == "stop-alias")
    {
	    if (params.size() < 2){
			throw runtime_error(
			"command needs at least 2 parameters\n");
	    }

	    std::string alias = params[1].get_str().c_str();

    	if(pwalletMain->IsLocked()) {
    		SecureString strWalletPass;
    	    strWalletPass.reserve(100);

			if (params.size() == 3){
				strWalletPass = params[2].get_str().c_str();
			} else {
				throw runtime_error(
				"Your wallet is locked, passphrase is required\n");
			}

			if(!pwalletMain->Unlock(strWalletPass)){
				return "incorrect passphrase";
			}
        }

    	bool found = false;

		Object statusObj;
		statusObj.push_back(Pair("alias", alias));

    	BOOST_FOREACH(CFloatingcityConfig::CFloatingcityEntry mne, floatingcityConfig.getEntries()) {
    		if(mne.getAlias() == alias) {
    			found = true;
    			std::string errorMessage;
    			bool result = activeFloatingcity.StopFloatingCity(mne.getIp(), mne.getPrivKey(), errorMessage);

				statusObj.push_back(Pair("result", result ? "successful" : "failed"));
    			if(!result) {
   					statusObj.push_back(Pair("errorMessage", errorMessage));
   				}
    			break;
    		}
    	}

    	if(!found) {
    		statusObj.push_back(Pair("result", "failed"));
    		statusObj.push_back(Pair("errorMessage", "could not find alias in config. Verify with list-conf."));
    	}

    	pwalletMain->Lock();
    	return statusObj;
    }

    if (strCommand == "stop-many")
    {
    	if(pwalletMain->IsLocked()) {
			SecureString strWalletPass;
			strWalletPass.reserve(100);

			if (params.size() == 2){
				strWalletPass = params[1].get_str().c_str();
			} else {
				throw runtime_error(
				"Your wallet is locked, passphrase is required\n");
			}

			if(!pwalletMain->Unlock(strWalletPass)){
				return "incorrect passphrase";
			}
		}

		int total = 0;
		int successful = 0;
		int fail = 0;


		Object resultsObj;

		BOOST_FOREACH(CFloatingcityConfig::CFloatingcityEntry mne, floatingcityConfig.getEntries()) {
			total++;

			std::string errorMessage;
			bool result = activeFloatingcity.StopFloatingCity(mne.getIp(), mne.getPrivKey(), errorMessage);

			Object statusObj;
			statusObj.push_back(Pair("alias", mne.getAlias()));
			statusObj.push_back(Pair("result", result ? "successful" : "failed"));

			if(result) {
				successful++;
			} else {
				fail++;
				statusObj.push_back(Pair("errorMessage", errorMessage));
			}

			resultsObj.push_back(Pair("status", statusObj));
		}
		pwalletMain->Lock();

		Object returnObj;
		returnObj.push_back(Pair("overall", "Successfully stopped " + boost::lexical_cast<std::string>(successful) + " floatingcities, failed to stop " +
				boost::lexical_cast<std::string>(fail) + ", total " + boost::lexical_cast<std::string>(total)));
		returnObj.push_back(Pair("detail", resultsObj));

		return returnObj;

    }

    if (strCommand == "list")
    {
        Array newParams(params.size() - 1);
        std::copy(params.begin() + 1, params.end(), newParams.begin());
        return floatingcitylist(newParams, fHelp);
    }

    if (strCommand == "count")
    {
        if (params.size() > 2){
            throw runtime_error(
                "too many parameters\n");
        }

        if (params.size() == 2)
        {
            if(params[1] == "enabled") return mnodeman.CountEnabled();
            if(params[1] == "both") return boost::lexical_cast<std::string>(mnodeman.CountEnabled()) + " / " + boost::lexical_cast<std::string>(mnodeman.size());
        }
        return mnodeman.size();
    }

    if (strCommand == "start")
    {
        if(!fFloatingCity) return "you must set floatingcity=1 in the configuration";

        if(pwalletMain->IsLocked()) {
            SecureString strWalletPass;
            strWalletPass.reserve(100);

            if (params.size() == 2){
                strWalletPass = params[1].get_str().c_str();
            } else {
                throw runtime_error(
                    "Your wallet is locked, passphrase is required\n");
            }

            if(!pwalletMain->Unlock(strWalletPass)){
                return "incorrect passphrase";
            }
        }

        if(activeFloatingcity.status != FLOATINGCITY_REMOTELY_ENABLED && activeFloatingcity.status != FLOATINGCITY_IS_CAPABLE){
            activeFloatingcity.status = FLOATINGCITY_NOT_PROCESSED; // TODO: consider better way
            std::string errorMessage;
            activeFloatingcity.ManageStatus();
            pwalletMain->Lock();
        }

        if(activeFloatingcity.status == FLOATINGCITY_REMOTELY_ENABLED) return "floatingcity started remotely";
        if(activeFloatingcity.status == FLOATINGCITY_INPUT_TOO_NEW) return "floatingcity input must have at least 15 confirmations";
        if(activeFloatingcity.status == FLOATINGCITY_STOPPED) return "floatingcity is stopped";
        if(activeFloatingcity.status == FLOATINGCITY_IS_CAPABLE) return "successfully started floatingcity";
        if(activeFloatingcity.status == FLOATINGCITY_NOT_CAPABLE) return "not capable floatingcity: " + activeFloatingcity.notCapableReason;
        if(activeFloatingcity.status == FLOATINGCITY_SYNC_IN_PROCESS) return "sync in process. Must wait until client is synced to start.";

        return "unknown";
    }

    if (strCommand == "start-alias")
    {
	    if (params.size() < 2){
			throw runtime_error(
			"command needs at least 2 parameters\n");
	    }

	    std::string alias = params[1].get_str().c_str();

    	if(pwalletMain->IsLocked()) {
    		SecureString strWalletPass;
    	    strWalletPass.reserve(100);

			if (params.size() == 3){
				strWalletPass = params[2].get_str().c_str();
			} else {
				throw runtime_error(
				"Your wallet is locked, passphrase is required\n");
			}

			if(!pwalletMain->Unlock(strWalletPass)){
				return "incorrect passphrase";
			}
        }

    	bool found = false;

		Object statusObj;
		statusObj.push_back(Pair("alias", alias));

    	BOOST_FOREACH(CFloatingcityConfig::CFloatingcityEntry mne, floatingcityConfig.getEntries()) {
    		if(mne.getAlias() == alias) {
    			found = true;
    			std::string errorMessage;
                std::string strDonateAddress = "";
                std::string strDonationPercentage = "";

                bool result = activeFloatingcity.Register(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), strDonateAddress, strDonationPercentage, errorMessage);
  
                statusObj.push_back(Pair("result", result ? "successful" : "failed"));
    			if(!result) {
					statusObj.push_back(Pair("errorMessage", errorMessage));
				}
    			break;
    		}
    	}

    	if(!found) {
    		statusObj.push_back(Pair("result", "failed"));
    		statusObj.push_back(Pair("errorMessage", "could not find alias in config. Verify with list-conf."));
    	}

    	pwalletMain->Lock();
    	return statusObj;

    }

    if (strCommand == "start-many")
    {
    	if(pwalletMain->IsLocked()) {
			SecureString strWalletPass;
			strWalletPass.reserve(100);

			if (params.size() == 2){
				strWalletPass = params[1].get_str().c_str();
			} else {
				throw runtime_error(
				"Your wallet is locked, passphrase is required\n");
			}

			if(!pwalletMain->Unlock(strWalletPass)){
				return "incorrect passphrase";
			}
		}

		std::vector<CFloatingcityConfig::CFloatingcityEntry> mnEntries;
		mnEntries = floatingcityConfig.getEntries();

		int total = 0;
		int successful = 0;
		int fail = 0;

		Object resultsObj;

		BOOST_FOREACH(CFloatingcityConfig::CFloatingcityEntry mne, floatingcityConfig.getEntries()) {
			total++;

			std::string errorMessage;
            std::string strDonateAddress = "";
            std::string strDonationPercentage = "";

            bool result = activeFloatingcity.Register(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), strDonateAddress, strDonationPercentage, errorMessage);

			Object statusObj;
			statusObj.push_back(Pair("alias", mne.getAlias()));
			statusObj.push_back(Pair("result", result ? "succesful" : "failed"));

			if(result) {
				successful++;
			} else {
				fail++;
				statusObj.push_back(Pair("errorMessage", errorMessage));
			}

			resultsObj.push_back(Pair("status", statusObj));
		}
		pwalletMain->Lock();

		Object returnObj;
		returnObj.push_back(Pair("overall", "Successfully started " + boost::lexical_cast<std::string>(successful) + " floatingcities, failed to start " +
				boost::lexical_cast<std::string>(fail) + ", total " + boost::lexical_cast<std::string>(total)));
		returnObj.push_back(Pair("detail", resultsObj));

		return returnObj;
    }

    if (strCommand == "debug")
    {
        if(activeFloatingcity.status == FLOATINGCITY_REMOTELY_ENABLED) return "floatingcity started remotely";
        if(activeFloatingcity.status == FLOATINGCITY_INPUT_TOO_NEW) return "floatingcity input must have at least 15 confirmations";
        if(activeFloatingcity.status == FLOATINGCITY_IS_CAPABLE) return "successfully started floatingcity";
        if(activeFloatingcity.status == FLOATINGCITY_STOPPED) return "floatingcity is stopped";
        if(activeFloatingcity.status == FLOATINGCITY_NOT_CAPABLE) return "not capable floatingcity: " + activeFloatingcity.notCapableReason;
        if(activeFloatingcity.status == FLOATINGCITY_SYNC_IN_PROCESS) return "sync in process. Must wait until client is synced to start.";

        CTxIn vin = CTxIn();
        CPubKey pubkey = CScript();
        CKey key;
        bool found = activeFloatingcity.GetFloatingCityVin(vin, pubkey, key);
        if(!found){
            return "Missing floatingcity input, please look at the documentation for instructions on floatingcity creation";
        } else {
            return "No problems were found";
        }
    }

    if (strCommand == "create")
    {

        return "Not implemented yet, please look at the documentation for instructions on floatingcity creation";
    }

    if (strCommand == "current")
    {
        CFloatingcity* winner = mnodeman.GetCurrentFloatingCity(1);
        if(winner) {
            Object obj;
            CScript pubkey;
            pubkey.SetDestination(winner->pubkey.GetID());
            CTxDestination address1;
            ExtractDestination(pubkey, address1);
            CZalemCoinAddress address2(address1);

            obj.push_back(Pair("IP:port",       winner->addr.ToString().c_str()));
            obj.push_back(Pair("protocol",      (int64_t)winner->protocolVersion));
            obj.push_back(Pair("vin",           winner->vin.prevout.hash.ToString().c_str()));
            obj.push_back(Pair("pubkey",        address2.ToString().c_str()));
            obj.push_back(Pair("lastseen",      (int64_t)winner->lastTimeSeen));
            obj.push_back(Pair("activeseconds", (int64_t)(winner->lastTimeSeen - winner->sigTime)));
            return obj;
        }

        return "unknown";
    }

    if (strCommand == "genkey")
    {
        CKey secret;
        secret.MakeNewKey(false);

        return CZalemCoinSecret(secret).ToString();
    }

    if (strCommand == "winners")
    {
        Object obj;
        std::string strMode = "addr";
    
        if (params.size() >= 1) strMode = params[0].get_str();

        for(int nHeight = pindexBest->nHeight-10; nHeight < pindexBest->nHeight+20; nHeight++)
        {
            CScript payee;
            CTxIn vin;
            if(floatingcityPayments.GetBlockPayee(nHeight, payee, vin)){
                CTxDestination address1;
                ExtractDestination(payee, address1);
                CZalemCoinAddress address2(address1);

                if(strMode == "addr")
                    obj.push_back(Pair(boost::lexical_cast<std::string>(nHeight),       address2.ToString().c_str()));

                if(strMode == "vin")
                    obj.push_back(Pair(boost::lexical_cast<std::string>(nHeight),       vin.ToString().c_str()));

            } else {
                obj.push_back(Pair(boost::lexical_cast<std::string>(nHeight),       ""));
            }
        }

        return obj;
    }

    if(strCommand == "enforce")
    {
        return (uint64_t)enforceFloatingcityPaymentsTime;
    }

    if(strCommand == "connect")
    {
        std::string strAddress = "";
        if (params.size() == 2){
            strAddress = params[1].get_str().c_str();
        } else {
            throw runtime_error(
                "Floatingcity address required\n");
        }

        CService addr = CService(strAddress);

        if(ConnectNode((CAddress)addr, NULL, true)){
            return "successfully connected";
        } else {
            return "error connecting";
        }
    }

    if(strCommand == "list-conf")
    {
    	std::vector<CFloatingcityConfig::CFloatingcityEntry> mnEntries;
    	mnEntries = floatingcityConfig.getEntries();

        Object resultObj;

        BOOST_FOREACH(CFloatingcityConfig::CFloatingcityEntry mne, floatingcityConfig.getEntries()) {
    		Object mnObj;
    		mnObj.push_back(Pair("alias", mne.getAlias()));
    		mnObj.push_back(Pair("address", mne.getIp()));
    		mnObj.push_back(Pair("privateKey", mne.getPrivKey()));
    		mnObj.push_back(Pair("txHash", mne.getTxHash()));
    		mnObj.push_back(Pair("outputIndex", mne.getOutputIndex()));
    		resultObj.push_back(Pair("floatingcity", mnObj));
    	}

    	return resultObj;
    }

    if (strCommand == "outputs"){
        // Find possible candidates
        vector<COutput> possibleCoins = activeFloatingcity.SelectCoinsFloatingcity();

        Object obj;
        BOOST_FOREACH(COutput& out, possibleCoins) {
            obj.push_back(Pair(out.tx->GetHash().ToString().c_str(), boost::lexical_cast<std::string>(out.i)));
        }

        return obj;

    }

    if(strCommand == "vote-many")
    {
        std::vector<CFloatingcityConfig::CFloatingcityEntry> mnEntries;
        mnEntries = floatingcityConfig.getEntries();

        if (params.size() != 2)
            throw runtime_error("You can only vote 'yay' or 'nay'");

        std::string vote = params[1].get_str().c_str();
        if(vote != "yay" && vote != "nay") return "You can only vote 'yay' or 'nay'";
        int nVote = 0;
        if(vote == "yay") nVote = 1;
        if(vote == "nay") nVote = -1;

        int success = 0;
        int failed = 0;

        Object resultObj;

        BOOST_FOREACH(CFloatingcityConfig::CFloatingcityEntry mne, floatingcityConfig.getEntries()) {
            std::string errorMessage;
            std::vector<unsigned char> vchFloatingCitySignature;
            std::string strFloatingCitySignMessage;

            CPubKey pubKeyCollateralAddress;
            CKey keyCollateralAddress;
            CPubKey pubKeyFloatingcity;
            CKey keyFloatingcity;

            if(!mnEngineSigner.SetKey(mne.getPrivKey(), errorMessage, keyFloatingcity, pubKeyFloatingcity)){
                printf(" Error upon calling SetKey for %s\n", mne.getAlias().c_str());
                failed++;
                continue;
            }

            CFloatingcity* pmn = mnodeman.Find(pubKeyFloatingcity);
            if(pmn == NULL)
            {
                printf("Can't find floatingcity by pubkey for %s\n", mne.getAlias().c_str());
                failed++;
                continue;
            }

            std::string strMessage = pmn->vin.ToString() + boost::lexical_cast<std::string>(nVote);

            if(!mnEngineSigner.SignMessage(strMessage, errorMessage, vchFloatingCitySignature, keyFloatingcity)){
                printf(" Error upon calling SignMessage for %s\n", mne.getAlias().c_str());
                failed++;
                continue;
            }

            if(!mnEngineSigner.VerifyMessage(pubKeyFloatingcity, vchFloatingCitySignature, strMessage, errorMessage)){
                printf(" Error upon calling VerifyMessage for %s\n", mne.getAlias().c_str());
                failed++;
                continue;
            }

            success++;

            //send to all peers
            LOCK(cs_vNodes);
            BOOST_FOREACH(CNode* pnode, vNodes)
                pnode->PushMessage("mvote", pmn->vin, vchFloatingCitySignature, nVote);
        }

        return("Voted successfully " + boost::lexical_cast<std::string>(success) + " time(s) and failed " + boost::lexical_cast<std::string>(failed) + " time(s).");
    }

    if(strCommand == "vote")
    {
        std::vector<CFloatingcityConfig::CFloatingcityEntry> mnEntries;
        mnEntries = floatingcityConfig.getEntries();

        if (params.size() != 2)
            throw runtime_error("You can only vote 'yay' or 'nay'");

        std::string vote = params[1].get_str().c_str();
        if(vote != "yay" && vote != "nay") return "You can only vote 'yay' or 'nay'";
        int nVote = 0;
        if(vote == "yay") nVote = 1;
        if(vote == "nay") nVote = -1;

        // Choose coins to use
        CPubKey pubKeyCollateralAddress;
        CKey keyCollateralAddress;
        CPubKey pubKeyFloatingcity;
        CKey keyFloatingcity;

        std::string errorMessage;
        std::vector<unsigned char> vchFloatingCitySignature;
        std::string strMessage = activeFloatingcity.vin.ToString() + boost::lexical_cast<std::string>(nVote);

        if(!mnEngineSigner.SetKey(strFloatingCityPrivKey, errorMessage, keyFloatingcity, pubKeyFloatingcity))
            return(" Error upon calling SetKey");

        if(!mnEngineSigner.SignMessage(strMessage, errorMessage, vchFloatingCitySignature, keyFloatingcity))
            return(" Error upon calling SignMessage");

        if(!mnEngineSigner.VerifyMessage(pubKeyFloatingcity, vchFloatingCitySignature, strMessage, errorMessage))
            return(" Error upon calling VerifyMessage");

        //send to all peers
        LOCK(cs_vNodes);
        BOOST_FOREACH(CNode* pnode, vNodes)
            pnode->PushMessage("mvote", activeFloatingcity.vin, vchFloatingCitySignature, nVote);

    }

    if(strCommand == "status")
    {
        std::vector<CFloatingcityConfig::CFloatingcityEntry> mnEntries;
        mnEntries = floatingcityConfig.getEntries();

        CScript pubkey;
        pubkey = GetScriptForDestination(activeFloatingcity.pubKeyFloatingcity.GetID());
        CTxDestination address1;
        ExtractDestination(pubkey, address1);
        CZalemCoinAddress address2(address1);

        Object mnObj;
        CFloatingcity *pmn = mnodeman.Find(activeFloatingcity.vin);

        mnObj.push_back(Pair("vin", activeFloatingcity.vin.ToString().c_str()));
        mnObj.push_back(Pair("service", activeFloatingcity.service.ToString().c_str()));
        mnObj.push_back(Pair("status", activeFloatingcity.status));
        //mnObj.push_back(Pair("pubKeyFloatingcity", address2.ToString().c_str()));
        if (pmn) mnObj.push_back(Pair("pubkey", CZalemCoinAddress(pmn->pubkey.GetID()).ToString()));
        mnObj.push_back(Pair("notCapableReason", activeFloatingcity.notCapableReason.c_str()));

        return mnObj;
    }

    return Value::null;
}

Value floatingcitylist(const Array& params, bool fHelp)
{
    std::string strMode = "status";
    std::string strFilter = "";

    if (params.size() >= 1) strMode = params[0].get_str();
    if (params.size() == 2) strFilter = params[1].get_str();

    if (fHelp ||
            (strMode != "activeseconds" && strMode != "donation" && strMode != "full" && strMode != "lastseen" && strMode != "protocol" 
                && strMode != "pubkey" && strMode != "rank" && strMode != "status" && strMode != "addr" && strMode != "votes" && strMode != "lastpaid"))
    {
        throw runtime_error(
                "floatingcitylist ( \"mode\" \"filter\" )\n"
                "Get a list of floatingcities in different modes\n"
                "\nArguments:\n"
                "1. \"mode\"      (string, optional/required to use filter, defaults = status) The mode to run list in\n"
                "2. \"filter\"    (string, optional) Filter results. Partial match by IP by default in all modes, additional matches in some modes\n"
                "\nAvailable modes:\n"
                "  activeseconds  - Print number of seconds floatingcity recognized by the network as enabled\n"
                "  donation       - Show donation settings\n"
                "  full           - Print info in format 'status protocol pubkey vin lastseen activeseconds' (can be additionally filtered, partial match)\n"
                "  lastseen       - Print timestamp of when a floatingcity was last seen on the network\n"
                "  protocol       - Print protocol of a floatingcity (can be additionally filtered, exact match)\n"
                "  pubkey         - Print public key associated with a floatingcity (can be additionally filtered, partial match)\n"
                "  rank           - Print rank of a floatingcity based on current block\n"
                "  status         - Print floatingcity status: ENABLED / EXPIRED / VIN_SPENT / REMOVE / POS_ERROR (can be additionally filtered, partial match)\n"
                "  addr            - Print ip address associated with a floatingcity (can be additionally filtered, partial match)\n"
                "  votes          - Print all floatingcity votes for a Zalem-Coin initiative (can be additionally filtered, partial match)\n"
                "  lastpaid       - The last time a node was paid on the network\n"
                );
    }

    Object obj;
    if (strMode == "rank") {
        std::vector<pair<int, CFloatingcity> > vFloatingcityRanks = mnodeman.GetFloatingcityRanks(pindexBest->nHeight);
        BOOST_FOREACH(PAIRTYPE(int, CFloatingcity)& s, vFloatingcityRanks) {
            std::string strVin = s.second.vin.prevout.ToStringShort();
            if(strFilter !="" && strVin.find(strFilter) == string::npos) continue;
            obj.push_back(Pair(strVin,       s.first));
        }
    } else {
        std::vector<CFloatingcity> vFloatingcities = mnodeman.GetFullFloatingcityVector();
        BOOST_FOREACH(CFloatingcity& mn, vFloatingcities) {
            std::string strVin = mn.vin.prevout.ToStringShort();
            if (strMode == "activeseconds") {
                if(strFilter !="" && strVin.find(strFilter) == string::npos) continue;
                obj.push_back(Pair(strVin,       (int64_t)(mn.lastTimeSeen - mn.sigTime)));
            } else if (strMode == "donation") {
                CTxDestination address1;
                ExtractDestination(mn.donationAddress, address1);
                CZalemCoinAddress address2(address1);

                if(strFilter !="" && address2.ToString().find(strFilter) == string::npos &&
                    strVin.find(strFilter) == string::npos) continue;

                std::string strOut = "";

                if(mn.donationPercentage != 0){
                    strOut = address2.ToString().c_str();
                    strOut += ":";
                    strOut += boost::lexical_cast<std::string>(mn.donationPercentage);
                }
                obj.push_back(Pair(strVin,       strOut.c_str()));
            } else if (strMode == "full") {
                CScript pubkey;
                pubkey.SetDestination(mn.pubkey.GetID());
                CTxDestination address1;
                ExtractDestination(pubkey, address1);
                CZalemCoinAddress address2(address1);

                std::ostringstream addrStream;
                addrStream << setw(21) << strVin;

                std::ostringstream stringStream;
                stringStream << setw(10) <<
                               mn.Status() << " " <<
                               mn.protocolVersion << " " <<
                               address2.ToString() << " " <<
                               mn.addr.ToString() << " " <<
                               mn.lastTimeSeen << " " << setw(8) <<
                               (mn.lastTimeSeen - mn.sigTime) << " " <<
                               mn.nLastPaid;
                std::string output = stringStream.str();
                stringStream << " " << strVin;
                if(strFilter !="" && stringStream.str().find(strFilter) == string::npos &&
                        strVin.find(strFilter) == string::npos) continue;
                obj.push_back(Pair(addrStream.str(), output));
            } else if (strMode == "lastseen") {
                if(strFilter !="" && strVin.find(strFilter) == string::npos) continue;
                obj.push_back(Pair(strVin,       (int64_t)mn.lastTimeSeen));
            } else if (strMode == "protocol") {
                if(strFilter !="" && strFilter != boost::lexical_cast<std::string>(mn.protocolVersion) &&
                    strVin.find(strFilter) == string::npos) continue;
                obj.push_back(Pair(strVin,       (int64_t)mn.protocolVersion));
            } else if (strMode == "pubkey") {
                CScript pubkey;
                pubkey.SetDestination(mn.pubkey.GetID());
                CTxDestination address1;
                ExtractDestination(pubkey, address1);
                CZalemCoinAddress address2(address1);

                if(strFilter !="" && address2.ToString().find(strFilter) == string::npos &&
                    strVin.find(strFilter) == string::npos) continue;
                obj.push_back(Pair(strVin,       address2.ToString().c_str()));
            } else if(strMode == "status") {
                std::string strStatus = mn.Status();
                if(strFilter !="" && strVin.find(strFilter) == string::npos && strStatus.find(strFilter) == string::npos) continue;
                obj.push_back(Pair(strVin,       strStatus.c_str()));
            } else if (strMode == "addr") {
                if(strFilter !="" && mn.vin.prevout.hash.ToString().find(strFilter) == string::npos &&
                    strVin.find(strFilter) == string::npos) continue;
                obj.push_back(Pair(strVin,       mn.addr.ToString().c_str()));
            } else if(strMode == "votes"){
                std::string strStatus = "ABSTAIN";

                //voting lasts 30 days, ignore the last vote if it was older than that
                if((GetAdjustedTime() - mn.lastVote) < (60*60*30*24))
                {
                    if(mn.nVote == -1) strStatus = "NAY";
                    if(mn.nVote == 1) strStatus = "YAY";
                }

                if(strFilter !="" && (strVin.find(strFilter) == string::npos && strStatus.find(strFilter) == string::npos)) continue;
                obj.push_back(Pair(strVin,       strStatus.c_str()));
            } else if(strMode == "lastpaid"){
                if(strFilter !="" && mn.vin.prevout.hash.ToString().find(strFilter) == string::npos &&
                    strVin.find(strFilter) == string::npos) continue;
                obj.push_back(Pair(strVin,      (int64_t)mn.nLastPaid));
            }
        }
    }
    return obj;

}
