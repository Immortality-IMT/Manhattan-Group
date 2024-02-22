# Manhattan Group (DeSci Blockchain)

<img align="right" src="https://imt.cx/assets/img/logo/mhg.png" width="200" alt="Manhattan Group Blockchain">

[Immortality Coin](https://imt.cx) is also backed by a dedicated blockchain. Code named: Manhattan Group. Not live.

Run the makefile with make, requires Dilithium2

This is DeSci, learn more, watch the video: https://youtu.be/-DeMklVWNdA

 5 differences and advancements to blockchain.

 1. PoUW - Proof of Useful Work, mining is resource intensive and the proof of work is largely pointless. Proof of Useful Work hashes useful work such as building A.I.
 2. Miner's Dream - the blockchain is mined at a variable rate, resulting in consistently the highest profits for miners relative to other coins. Hash rate stabilization and no halving could mean earnings are consistent.
 3. Moonshots - a portion of the miner's fee is restricted and miners donate to worthy science endeavours, so that sci-fi moonshot dialogue can accompany the crypto.
 4. Programmable Wallets - wallets as smart contracts, so features like refunds, cooling-off periods, buyer protection and scam protection can be selected with transactions.
 5. Dilithium2 quantum resistance.

Along with a bankless financial infrastructure, Manhattan Group seeks to fund future technologies with blockchain and crypto. Technologies that unlock technologies, watershed technologies, upstream technologies such as AGI that have far-reaching upgrade potential. How?

```We suggest doubling the miner's block reward and transaction fee to fund DeSci. The extra miner's block reward comes with a restriction, it must be donated to watershed technologies.```

#### Some projects that DeSci should prioritize
- Open infrastructure for building A.I. and Open source A.I. Unsanitized, uncensored, open source AGI.
- The Big Brain Project - Dish brain projects
- Space Tourism, indestructible human capsule guaranteeing safety
- Cure Aging, live forever, cure cancer, feel good
- Renewable energy, turn the world's deserts into solar power stations and 100% proof of work from renewables
- More computer power, manufacture faster personal computers and offer them at great prices

#### Consensus: PoUW
Proof-of-Useful-Work (PoUW): A relatively new concept in blockchain technology that aims to address limitations of traditional Proof-of-Work (PoW) by using miners to perform computations with real-world value. Contributes to an open A.I. infrastructure, distributed computing solving real world problems, building A.I. Resource intensive and profitable. Proof-of-Activity/Importance/Capacity - better nodes are preferred by criteria. Some minimum computing capacity or uptime to be preferenced in the network.

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

