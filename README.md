# Manhattan Group (DeSci Blockchain)

<img align="right" src="https://imt.cx/assets/img/logo/mhg.png" width="200" alt="Manhattan Group Blockchain">

[Immortality Coin](https://imt.cx) has a dedicated blockchain project. Code-name, Manhattan Group.

We are DeSci, watch the video here... https://youtu.be/-DeMklVWNdA

Along with a bankless financial infrastructure, Manhattan Group seeks to fund future technologies with blockchain and crypto. Technologies that unlock technologies, watershed technologies, upstream technologies such as AGI that have far-reaching upgrade potential. How?

```We suggest doubling the miner's block reward and transaction fee to fund DeSci. The extra miner's block reward comes with a restriction, it must be donated to watershed technologies.```

#### Some projects that DeSci should prioritize
- Open source A.I. such as GPT-Neo and GPT-J. Unsantized, uncensored, open source AGI.
- The Big Brain Project - Dish brain projects
- Space Tourism, indestructible human capsule guaranteeing safety
- Cure Aging, live forever, cure cancer, feel good
- Renewable energy, turn the world's deserts into solar power stations and 100% proof of work from renewables
- More computer power, manufacture faster personal computers and offer them at great prices

If you are interested in being part of the moderation of the project, contact us.

#### Compile: make wallet node_discovery propagation vm
The compilation produces execution files in each directory. Dilithium2 requires OQS lib, see below.

The whitepaper is being written at https://imt.cx/blockchain.html

There is no expediency in the production of this blockchain, we shall understand and solve all the problems in the whitepaper before we begin coding.

<img align="right" src="https://github.com/Immortality-IMT/Manhattan-Project/blob/main/blockchain_cryptocurrency/screenshot_wallet.png" alt="Manhattan Project Blockchain Screenshot">

Immortality Coin
Homepage: https://imt.cx

The project is broken up into separate parts and each are independent of each other. So work on what you want to work on. At release, we shall bring it all together.

Part 1: Node discovery system of the peer to peer network<br />
Part 2: Propagation and latency of information to the peer to peer network<br />
Part 3: Is the blockchain stuff, mining, wallets, crunching numbers...<br />
Part 4: Smart contract platform<br />
Part 5: API's, application specific program interfaces that make it easy for industry to interface with our blockchain'<br />

The implementation of Dilithium2 requires OQS lib
--------------------------------------------------
APT...

 sudo apt install astyle cmake gcc ninja-build libssl-dev python3-pytest python3-pytest-xdist<br>
 git clone -b main https://github.com/open-quantum-safe/liboqs.git<br>
 cd liboqs<br>

build:<br>

 mkdir build && cd build<br>
 cmake -GNinja -DBUILD_SHARED_LIBS=ON ..<br>
 ninja<br>
 ninja install<br>

if /usr/local/lib is not in $PATH<br>
 
 export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib<br>

