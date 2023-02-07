Compile
-------
gcc -o vm vm.c -lm

Process of smart contract functionality
---------------------------------------

1. Write the contract in Solidity or another language - https://vyper.readthedocs.io/en/stable/
2. Convert the code into virtual machine compatible bytecode. 
3. With a Gutenberg datatype, compression and techniques can also be implemented with the transaction. Such as https://github.com/Arachnid/evmdis & https://github.com/ConsenSys/mythril
4. Put the bytecode on the blockchain as a regular transaction and bytecode as transaction data, a unique contract address is returned. 
5. Every transaction has a data field for including smart contract bytecode or storing general data on blockchain.
6. The smart-contract is then a transaction and undergoes traditional blockchain processing, broadcasted, mined, blocked and added to blockchain.
7. The smart contract is deployed and is now accessible at the contract address provided, a transaction id. 
8. The owner can run the functions in the contract against the virtual machine and update the state of the code, calls to functions requires the owner through public private key encryption. New transactions are records on the smart contract state.
9. Further, calls to functions must process the transaction history to determine the current contract state and update the state in the latest transaction or the previous last transaction holds the current state. 
10. Smart contracts, from ledger to state machine. The ledger is used to record and keep track of the state of the code, new transactions call functions that update the code state. 
11. Functions are executed by vm.c

VM runs bytecode which are hex encoded values, they correspond to opcode instruction set, not x86 but a more financial based CPU instruction set. There is no need to decode the hex to get the opcode, mapping the 2byte hex value to the opcode in a swtich case is enough, e.g. Hex: 01 is always ADD...

All the codes are here, the stack column is the hex values and every instruction must be coded. (which byw I am not overly joyed in doing)
https://ethereum.org/en/developers/docs/evm/opcodes/

Solc - Compiler to code smart contracts
----------------------------------------
Solidity Compiler - https://imt.cx/solc - The compiler produces bytecode that can be fed to vm.c for testing purposes.

Bytecode for HelloWorld
-----------------------
60606040526040805190810160405280600c81526020017f48656c6c6f2c20576f726c642100000000000000000000000000000000000000008152506000908051906020019061004f929190610056565b50610105565b828054600181600116156101000203166002900490600052602060002090601f016020900481019282601f1061009c57805160ff19168380011785556100c9565b828001600101855582156100c9579182015b828111156100c95782518255916020019190600101906100ae565b5b5090506100d791906100db565b5090565b61010191905b808211156101005760008160009055506001016100e6565b5090565b90565b610432806101136000396000f300606060405263ffffffff7c0100000000000000000000000000000000000000000000000000000000600035041663c6888fa18114610047578063e2179b8e14610065575b600080fd5b341561005257600080fd5b61005a6100c4565b60405160208082528190810183818151815260200191508051906020019080838360005b8381101561009657808201518382015260200161007e565b50505050905090810190601f1680156100c35780820380516001836020036101000a031916815260200191505b5092505050604051809

Solidity Bytecode for HelloWorld
--------------------------------
608060405234801561001057600080fd5b5060e58061001f6000396000f3fe6080604052348015600f57600080fd5b506004361060285760003560e01c8063c605f76c14602d575b600080fd5b604080518082018252600d81526c48656c6c6f2c20576f726c642160981b60208201529051605a91906063565b60405180910390f35b600060208083528351808285015260005b81811015608e578581018301518582016040015282016074565b506000604082860101526040601f19601f830116850101925050509291505056fea26469706673582212205e06adeae992ded0cd6300f2a129793cc19fcf3f6890b24cb868ccdcdb5e179464736f6c63430008120033

```
// SPDX-License-Identifier: MIT
pragma solidity ^0.8.0;
contract MyContract {
    function helloWorld() public pure returns (string memory) {
        return "Hello, World!";
    }
}
```
Assembly Version
----------------
```
    /* "":56:181  contract MyContract {... */
  mstore(0x40, 0x80)
  callvalue
  dup1
  iszero
  tag_1
  jumpi
  0x00
  dup1
  revert
tag_1:
  pop
  dataSize(sub_0)
  dup1
  dataOffset(sub_0)
  0x00...
 ```
 
https://ethervm.io/decompile

Diassembly into vm executable opcodes...
<br />
label_0000:
// Inputs[1] { @0008  memory[0x40:0x60] }<br />
0000    60  PUSH1 0x60<br />
0002    60  PUSH1 0x40<br />
0004    52  MSTORE<br />
0005    60  PUSH1 0x40<br />
0007    80  DUP1<br />
0008    51  MLOAD<br />
0009    90  SWAP1<br />
000A    81  DUP2<br />
000B    01  ADD<br />
000C    60  PUSH1 0x40<br />
.....
