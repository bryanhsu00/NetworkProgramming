# NetworkProgramming


## Project 1: NPShell
- In this project, you are asked to design a shell with special piping mechanisms.

## Project 2: Remote Working Systems

- In this project, you are going to design 2 servers:
    - np_simple (Single User): Project 1. (Concurrent connection-oriented)
    - np_single_proc (Multiple users): Project 1 + User Pipe + 4 Functions + Broadcast message. (Single-process concurrent)

## Project 3: Remote Batch System

- In this project, you are going to design the remote batch system and the project is divided into two parts:
    - Remote batch system in NP Linux Workstation
    - Remote batch system in Windows 10 (use a MinGW distribution)


## Project 4: Proxy Server

- In this project, you are going to implement SOCKS 4/4A protocol.

- Please refer to NP_Project4_Spec.pdf and NP_Project4.pdf for project details.

- You are strongly recommended to pore over the SOCKS 4/4A protocol (link1/link2). 

- You need to setup your own FTP server for testing.

- Port number is specified by the first argument: ./socks_server [port]

- Reminder:
    - FTP mode should be set to ACTIVE mode.
    - If you run the FTP server on localhost, please ensure you can connect from outside. (e.g., by disabling the firewall)