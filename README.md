# [DLockers Client](https://eddytheco.github.io/DLockersClient/)


## Proof of Concept on implementing decentralized applications on the IOTA network.

This application can be seen as a decentralized client that allows you to book a locker  by paying with cryptos.
In order to use the application one needs to set the address of the node to connect.
The Proof of Work has to be performed by the node (by setting the JWT for protected routes, by enabling PoW in the node...).
In principle it will also work for the mainnet(or private network) by setting the node to a mainnet one(I have not tried).
This application is meant to be used on the testnet.
If using the mainnet **you are the ONLY responsible for the eventual loss of your funds**.

## How to use it

In order to book a locker the client needs to 'connect' to a [server](https://eddytheco.github.io/DLockersServer/).
For that one sets the server id at the settings window. 

One selects the day and hours of the booking and press book.
 

The communication between server and client relies on creating outputs on the ledger.
Because of that, the client needs an storage deposit of funds to be able to book on the server.
This storage deposit plus the price of the booking will be asked to you the first time you book.
The deposit is always own by the client and in the next booking you will only have to pay the price of the booking.
This proof of concept uses a random seed for the address creation(if reload the page the client loses its funds).

If the book and payment is accepted by the server, the server will send the client a NFT signed by the server.
By presenting this NFT to the server you can open the locker.
To do this, this application send the NFT to a server address but with expiration time in the past, so you continue owing the NFT
and can reuse it as any times you want.


The client is not intended to 'store funds' it is only and interface between the wallet and the server.

### CORS header 'Access-Control-Allow-Origin' missing

When using the browser application and your node, the API request could be denied with the return 'Reason: CORS header 'Access-Control-Allow-Origin' missing'.
In that case, one needs to set the Access-Control-Allow-Origin header's value as explained [here](https://developer.mozilla.org/en-US/docs/Web/HTTP/CORS/Errors/CORSMissingAllowOrigin).

If you use the docker setup of Hornet just add 

```
- "traefik.http.middlewares.cors.headers.customResponseHeaders.Access-Control-Allow-Origin=https://eddytheco.github.io"
- "traefik.http.routers.hornet.middlewares=cors"
```
to docker-compose.yml in the traefik section. Such that browser API requests from https://eddytheco.github.io are accepted  by your node.
