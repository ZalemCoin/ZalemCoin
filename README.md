Zalem-Coin [ZLM] integration/staging tree
=====================================

http://zalemcoin.chainpages.xyz/

What is the Zalem-Coin [ZLM] Blockchain?
----------------------------------------
*TODO: Update documentation regarding implemented tech as this section is out of date and much progress and upgrades have been made to mentioned sections...*

### Overview
This project aims to be a community driven, open source, fan made and operated 
blockchain that strives to bring both fans and those who worked hard to make the "Alita - 
Battle Angel" experience/movie a reality, the excitement of 
having your own blockchain and to stimulate interest in the story and support everyone involved. 
We will be actively reaching out to those involved in the movie and of course the fans as well. 
So make sure that you get your free Zalems in the hopes of launching your own Floating City!

If you are affiliated in any way to Alita Battle Angel and would like to contact us please feel free 
to reach out to us as we look forward to hearing from you and sending you some Zalems!  

Visit our website linked above and navigate to the "Communiy" section for more resources!

### Blockchain Technology
The Zalem [ZLM] Blockchain is an experimental smart contract platform protocol that enables 
instant payments to anyone, anywhere in the world in a private, secure manner. 
Zalem [ZLM] uses peer-to-peer blockchain technology developed by Zalem to operate
with no central authority: managing transactions, execution of contracts, and 
issuing money are carried out collectively by the network. Zalem [ZLM] is the name of 
open source software which enables the use of this protocol.

### Custom Difficulty Retarget Algorithm “VRX”
VRX is designed from the ground up to integrate properly with the Velocity parameter enforcement system to ensure users no longer receive orphan blocks.

### Velocity Block Constraint System
Ensuring Insane stays as secure and robust as possible the CryptoCoderz team have implemented what's known as the Velocity block constraint system. This system acts as third and final check for both mined and peer-accepted blocks ensuring that all parameters are strictly enforced.

### Wish (bmw512) Proof-of-Work Algorithm
Wish or bmw512 hashing algorithm is utilized for the Proof-of-Work function and also replaces much of the underlying codebase hashing functions as well that normally are SHA256. By doing so this codebase is able to be both exponentially lighter and more secure in comparison to reference implementations.

Specifications and General info
------------------
Zalem uses 

	libsecp256k1,
	libgmp,
	Boost1.68, OR Boost1.58,  
	Openssl1.02r,
	Berkeley DB 6.2.32,
	QT5.12.1,
	to compile


General Specs

	Block Spacing: 3 Minutes
	Stake Minimum Age: 25 Confirmations (PoS-v3) | 30 Minutes (PoS-v2)
	Port: 30099
	RPC Port: 30097


BUILD LINUX
-----------
### Compiling Zalem "SatoshiCore" daemon on Ubunutu 18.04 LTS Bionic
### Note: guide should be compatible with other Ubuntu versions from 14.04+

### Become poweruser
```
sudo -i
```
### CREATE SWAP FILE FOR DAEMON BUILD (if system has less than 2GB of RAM)
```
cd ~; sudo fallocate -l 3G /swapfile; ls -lh /swapfile; sudo chmod 600 /swapfile; ls -lh /swapfile; sudo mkswap /swapfile; sudo swapon /swapfile; sudo swapon --show; sudo cp /etc/fstab /etc/fstab.bak; echo '/swapfile none swap sw 0 0' | sudo tee -a /etc/fstab
```

### Dependencies install
```
cd ~; sudo apt-get install -y ntp git build-essential libssl-dev libdb-dev libdb++-dev libboost-all-dev libqrencode-dev libcurl4-openssl-dev curl libzip-dev; apt-get update -y; apt-get install -y git make automake build-essential libboost-all-dev; apt-get install -y yasm binutils libcurl4-openssl-dev openssl libssl-dev; sudo apt-get install -y libgmp-dev; sudo apt-get install -y libtool;
```

### Dependencies build and link
```
cd ~; wget http://download.oracle.com/berkeley-db/db-6.2.32.NC.tar.gz; tar zxf db-6.2.32.NC.tar.gz; cd db-6.2.32.NC/build_unix; ../dist/configure --enable-cxx; make; sudo make install; sudo ln -s /usr/local/BerkeleyDB.6.2/lib/libdb-6.2.so /usr/lib/libdb-6.2.so; sudo ln -s /usr/local/BerkeleyDB.6.2/lib/libdb_cxx-6.2.so /usr/lib/libdb_cxx-6.2.so; export BDB_INCLUDE_PATH="/usr/local/BerkeleyDB.6.2/include"; export BDB_LIB_PATH="/usr/local/BerkeleyDB.6.2/lib"
```

### GitHub pull (Source Download)
```
cd ~; git clone https://github.com/ZalemCoin/ZalemCoin
```

### Build Zalem daemon
```
cd ~; cd ~/ZalemCoin/src; chmod a+x obj; chmod a+x leveldb/build_detect_platform; chmod a+x secp256k1; chmod a+x leveldb; chmod a+x ~/ZalemCoin/src; chmod a+x ~/Zalem; make -f makefile.unix USE_UPNP=-; cd ~; cp -r ~/ZalemCoin/src/Zalem-Coind /usr/local/bin/Zalem-Coind;
```

### Create config file for daemon
```
cd ~; sudo ufw allow 30099/tcp; sudo ufw allow 30097/tcp; sudo ufw allow 22/tcp; sudo mkdir ~/.ZLM; cat << "CONFIG" >> ~/.ZLM/Zalem-Coin.conf
listen=1
server=1
daemon=1
testnet=0
rpcuser=ZLMrpcuser
rpcpassword=SomeCrazyVeryVerySecurePasswordHere
rpcport=30097
port=30099
rpcconnect=127.0.0.1
rpcallowip=127.0.0.1
addnode=157.230.107.144
addnode=138.197.161.183
addnode=188.166.109.87
addnode=46.101.73.64
CONFIG
chmod 700 ~/.ZLM/Zalem-Coin.conf; chmod 700 ~/.ZLM; ls -la ~/.ZLM
```

### Run Zalem daemon
```
cd ~; Zalem-Coind; Zalem-Coind getinfo
```

### Troubleshooting
### for basic troubleshooting run the following commands when compiling:
### this is for minupnpc errors compiling

```
make clean -f makefile.unix USE_UPNP=-
make -f makefile.unix USE_UPNP=-
```
### Updating daemon in bin directory
```
cd ~; cp -r ~/ZalemCoin/src/Zalem-Coind /usr/local/bin
```

License
-------

Zalem [ZLM] is released under the terms of the MIT license. See [COPYING](COPYING) for more
information or see https://opensource.org/licenses/MIT.

Development Process
-------------------

The `master` branch is regularly built and tested, but is not guaranteed to be
completely stable. [Tags](https://github.com/CryptoCoderz/ZLM/tags) are created
regularly to indicate new official, stable release versions of Zalem [ZLM].

The contribution workflow is described in [CONTRIBUTING.md](CONTRIBUTING.md).

The developer [mailing list](https://lists.linuxfoundation.org/mailman/listinfo/bitcoin-dev)
should be used to discuss complicated or controversial changes before working
on a patch set.

Developer Discord can be found at:

Testing
-------

Testing and code review is the bottleneck for development; we get more pull
requests than we can review and test on short notice. Please be patient and help out by testing
other people's pull requests, and remember this is a security-critical project where any mistake might cost people
lots of money.

### Automated Testing

Developers are strongly encouraged to write [unit tests](/doc/unit-tests.md) for new code, and to
submit new unit tests for old code. Unit tests can be compiled and run
(assuming they weren't disabled in configure) with: `make check`

There are also [regression and integration tests](/qa) of the RPC interface, written
in Python, that are run automatically on the build server.

### Manual Quality Assurance (QA) Testing

Changes should be tested by somebody other than the developer who wrote the
code. This is especially important for large or high-risk changes. It is useful
to add a test plan to the pull request description if testing the changes is
not straightforward.
