# Peer-to-Peer File Sharing Protocol

## Project Overview

This project implements a simple file-sharing protocol on a peer-to-peer (P2P) network. It addresses the problem of overloaded servers and congested paths in a centralized network by dividing files into segments. Once a segment is received, each peer becomes a server that can distribute that segment, achieving load distribution. The network includes a central server responsible for coordinating segment transfers.

## 1. Key Features

- Files are divided into 16KB segments, with a checksum calculated for each segment. These details are stored in a file on the coordinating server.
- The server maintains a record of which peers hold available copies of each file segment.
- The file transfer process involves the client requesting segment location data from the server, transferring segments from the identified peer to the local machine, verifying the transfer using the checksum, and updating the server about the availability of the segment on the local machine.

## 2. Concept of the Solution

To start, the server application must be launched to manage the entire network. Upon startup, the server prepares the environment and handles client requests.

When a user starts the client application, it connects to the central server. If the client requests a file, the server provides information on which peer holds the requested file. If no peer has the file, the client retrieves it from the server; otherwise, it downloads the file from another peer in the network.

## 3. Description of the Solution

When the central server is started:
- It scans all the files in the system, divides them into 16KB segments, calculates a checksum for each segment, and stores this information in a specific folder.
- It prepares to listen for incoming client connections, assigning each connected client a dedicated thread for communication.
- A separate thread monitors client connections, removing any disconnected peers from the list of available segment sources.

When the client application starts:
- It connects to the central server and sends information about the segments it holds, allowing the server to list the client as a source for these segments.
- A dedicated thread is launched to handle incoming connections for P2P transfers.

When a client sends a `GET filename` command to the server, the server:
- Searches its archive for the file and provides the client with the best source (either a peer or the server).
- If a peer holds the file, the client starts a new thread to download it from the peer. The checksum is verified to ensure a valid transfer, and the server is notified to update the list of available sources for that file.

## 4. Testing

To verify the functionality, follow these steps:

1. Start the central server and several client applications. Initially, no client will hold any files.
2. One client requests a file, and the server informs the client that it can only be downloaded from the server. The transfer is completed.
3. A second client requests the same file, but now that the first client holds it, the file is transferred from the first client instead of the server.
4. The first client requests another file from the server. Manually delete some segments from the file on the first client.
5. The second client requests the same file. Some segments will be downloaded from the server, and others from the first client. Verify that combined downloading works correctly.
6. Manually inspect the downloaded files to ensure that the content is correct and intact.
7. With both the first and second clients holding the file, have a third client request the same file. Observe how the server determines the source for the file transfer.

The system behaves as expected, demonstrating the successful implementation of the P2P network.

---

