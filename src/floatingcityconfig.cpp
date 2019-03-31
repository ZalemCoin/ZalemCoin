
#include "net.h"
#include "floatingcityconfig.h"
#include "util.h"
#include <base58.h>

CFloatingcityConfig floatingcityConfig;

void CFloatingcityConfig::add(std::string alias, std::string ip, std::string privKey, std::string txHash, std::string outputIndex) {
    CFloatingcityEntry cme(alias, ip, privKey, txHash, outputIndex);
    entries.push_back(cme);
}

bool CFloatingcityConfig::read(boost::filesystem::path path) {
    boost::filesystem::ifstream streamConfig(GetFloatingcityConfigFile());
    if (!streamConfig.good()) {
        return true; // No floatingcity.conf file is OK
    }

    for(std::string line; std::getline(streamConfig, line); )
    {
        if(line.empty()) {
            continue;
        }
        std::istringstream iss(line);
        std::string alias, ip, privKey, txHash, outputIndex;
        iss.str(line);
        iss.clear();
        if (!(iss >> alias >> ip >> privKey >> txHash >> outputIndex)) {
            LogPrintf("Could not parse floatingcity.conf line: %s\n", line.c_str());
            streamConfig.close();
            return false;
        }

        add(alias, ip, privKey, txHash, outputIndex);
    }

    streamConfig.close();
    return true;
}
