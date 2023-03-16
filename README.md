# [DLockers Client](https://eddytheco.github.io/DLockersClient/wasm/)


## Proof of Concept on implementing decentralized applications on the IOTA network.

This application can be seen as a decentralized client that allows you to book a locker  by paying with Shimmer.
In order to use the application one needs to set the address of the node to connect.
The Proof of Work has to be performed by the node (by setting the JWT for protected routes, by enabling PoW in the node...).
In principle it will also work for the shimmer mainnet(or private network) by setting the node to a mainnet one(I have not tried).
This application is meant to be used on the testnet.
If using the mainnet **you are the ONLY responsible for the eventual loss of your funds**.

## How to use it

In order to book a locker the client needs to 'connect' to a [server](https://eddytheco.github.io/DLockersServer/wasm/).
For that one sets the server id at the initial window. 

One selects the day and hours of the booking.

The client will ask you to set the pin need it to open the locker on the server.
For the propose of this proof of concept  the  pin is a number of 5 digits but in real applications the pin should be a
large string.
  

The communication between server and client relies on creating outputs on the ledger.
Because of that, the client needs an storage deposit of funds to be able to book on the server.
This storage deposit plus the price of the booking will be asked to you the first time you book.
The deposit is always own by the client and in the next booking you will only have to pay the price of the booking.
This proof of concept uses a random seed for the address creation(if reload the page the client loses its funds).


The client is not intended to 'store funds' it is only and interface between the wallet and the server.

