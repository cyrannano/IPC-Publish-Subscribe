# IPC Publish - Subscribe model
## Communication schema
Server creates two types of communication tunnels:
- **Main tunnel** to handle users login / register requests
- **Private tunnel** with each client where the communication between server and client occurs.

IPC codes:
| Code | Description | Source | Tunnel |
|---|---|---|---|
|0|User register / login request| Server | Main tunnel |
|1|User send message request | Client | Private tunnel |
|2|Incoming message | Server | Private tunnel |
|3|New topic subscription | Client | Private tunnel |

## Build
### Server
`gcc server.c -o s.out -Wall -lpthread`
### Client
`gcc client.c -o c.out -Wall`


# Todo list:

## Server side:
- [x] User login / registration
- [x] Server message request handler
- [x] Saving changes to clients.data file
- [x] Server subscription request handler
- [ ] Block request handler

## Client side:
- [x] User login / registration
- [x] User send message request
- [x] User subscription request
- [ ] Block user request
- [ ] Improve UI
- [ ] waitForUserInput() function implementation

# Updates:
## 26.01.2020
  - **!BUG!** Messages are being trimmed after last space. Client-side error caused by scanf splittin message on white spaces. Need to think of a solution to that
  - Subscription requests and handling are probably fully working at this point. Need to test more in the future