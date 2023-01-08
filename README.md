# [DLockers Client](https://eddytheco.github.io/DLockersClient/wasm/)


## Proof of Concept on implementing decentralized applications on the [IOTA](https://www.iota.org/) network.

This application can be seen as a decentralized client that allows you to book a [locker]  by paying with Shimmer.


The application is set to use the [Shimmer Testnet](https://explorer.shimmer.network/testnet/)
and custom libraries developed by me.
For sending a block to the network the application needs to perform Proof of Work.
My implementation of proof of work it is not optimized and this will take much time on the browser, please be patient 
or do a pull request to this [repo](https://github.com/EddyTheCo/Qpow-IOTA) with a faster implementation.

In order to book a locker the client needs to 'connect' to a [server](https://eddytheco.github.io/DLockersClient/wasm/).
For that one enters the server id. 

One selects the day and hours of the booking(only the first contiguous-selected hours will be considered as a booking).

The client will ask you to set the pin need it to open the locker on the server.
For the propose of this proof of concept  the  pin is a number of 5 digits but in real applications the pin should be a
large string.
  

The communication between server and client relies on creating outputs on the ledger.
Because of that, the client needs an storage deposit of funds to be able to book on the server.
This storage deposit plus the price of the booking will be asked to you the first time you book.
The deposit is always own by the client and in the next booking you will only have to pay the price of the booking.
This proof of concept uses a random seed for the address creation(if reload the page the client loses its funds).
* *For paying to the client one can use a wallet like [firefly](https://firefly.iota.org/).*

The client is not intended to 'store funds' it is only and interface between the wallet and the server.

:warning: This has only be tested on the Firefox browser
