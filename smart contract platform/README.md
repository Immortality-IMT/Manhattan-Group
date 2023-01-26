Process of smart contract functionality
---------------------------------------

1. Write the contract in Python. For example vyper as a compiler solution - https://vyper.readthedocs.io/en/stable/
2. Convert the Python into bytecode. Convert to the virtual machine compatible format which is stripped down version of bytecode. 
3. Compression can also be implemented. Such as https://github.com/Arachnid/evmdis & https://github.com/ConsenSys/mythril
4. Put the bytecode onto the blockchain as a transaction and bytecode as data. A unique contract address is returned. 
5. Every transaction has a data field for including smart contract bytecode or storing general data on blockchain.
6. The smart-contract is a transaction and undergoes traditional blockchain processing, broadcasted, mined, blocked and added to blockchain.
7. Once the transaction is mined, the smart contract is deployed and is now accessible at the contract address provided.

Bytecode for HelloWorld
-----------------------
60606040526040805190810160405280600c81526020017f48656c6c6f2c20576f726c642100000000000000000000000000000000000000008152506000908051906020019061004f929190610056565b50610105565b828054600181600116156101000203166002900490600052602060002090601f016020900481019282601f1061009c57805160ff19168380011785556100c9565b828001600101855582156100c9579182015b828111156100c95782518255916020019190600101906100ae565b5b5090506100d791906100db565b5090565b61010191905b808211156101005760008160009055506001016100e6565b5090565b90565b610432806101136000396000f300606060405263ffffffff7c0100000000000000000000000000000000000000000000000000000000600035041663c6888fa18114610047578063e2179b8e14610065575b600080fd5b341561005257600080fd5b61005a6100c4565b60405160208082528190810183818151815260200191508051906020019080838360005b8381101561009657808201518382015260200161007e565b50505050905090810190601f1680156100c35780820380516001836020036101000a031916815260200191505b5092505050604051809

EVM Bytecode for HelloWorld
---------------------------
0x60606040526040805190810160405280600c81526020017f48656c6c6f2c20576f726c642100000000000000000000000000000000000000008152506000908051906020019061004f929190610056565b50610105565b8280546001816001161561

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

6. Each subsequent call to functions in the smart contract are transactions. Only the owner can sign and execute functions.
