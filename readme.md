# IPC Publish - Subscribe model
## Communication schema
Server creates two types of communication tunnels:
- **Main tunnel** to handle users login / register requests
- **Private tunnel** with each client where the communication between server and client occurs.

IPC codes:
| Code | Description | Source | Tunnel |
|---|---|---|---|
|*random generated*|User register / login request| Client | Main tunnel |
|*received from user*|User id assignment| Server | Main tunnel |
|1|User send message request | Client | Private tunnel |
|2|Incoming message | Server | Private tunnel |
|3|New topic subscription | Client | Private tunnel |
|4|Block user request | Client | Private tunnel |
|5|Unblock user request | Client | Private tunnel |

## Build
### Server
`gcc server.c -o s.out -Wall -lpthread`
### Client
`gcc client.c -o c.out -Wall -lpthread`


# Todo list:

## Server side:
- [x] User login / registration
- [x] Server message request handler
- [x] Saving changes to clients.data file
- [x] Server subscription request handler
- [x] Block request handler
- [x] Unblock request handler
- [x] Blocking client on too many login attempts

## Client side:
- [x] User login / registration
- [x] User send message request
- [x] User subscription request
- [x] Block user request
- [x] Unblock user request
- [x] waitForUserInput() function implementation
- [x] Client behaviour on login block 
- [x] Improve UI

# Updates:
## 29.01.2021 v2
  - Cleaned files
  - Added variable logging in server
## 29.01.2021 v1
  - Checking whether *serverData* directory exists. If not, it's being created
  - Blocking user on too many failed login attempts
  - Cleaned up warnings during compilation
  - Added sync message receiver
## 28.01.2021 v1
  - Added unblock functionality
  - Improved UI
  - Used new IPC code "5"
  - Improved waiting for user input thanks to ignoring newline character in scanf inputs using *%\*c*
## 27.01.2021 v1
  - Fixed *checkIfBlocked* function
  - Implemented user blocking funtion - client side
  - Implemented user blocking funtion - server side
  - Used new IPC code "4"
## 26.01.2021 v2
  - Moved structures to separate header file
  - Waiting for user input is kinda working. But only for main thread. The trick for other threads is to tell user to type 0 to conitnue
  - **!BUGFIX!** Messages are not being trimmed no more (fix from 26.01.2021)
## 26.01.2021
  - **!BUG!** Messages are being trimmed after last space. Client-side error caused by scanf splittin message on white spaces. Need to think of a solution to that
  - Subscription requests and handling are probably fully working at this point. Need to test more in the future
