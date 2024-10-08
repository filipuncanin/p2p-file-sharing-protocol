Project Overview
Implementation of a simple file-sharing protocol on a peer-to-peer network.

This project addresses the problem of an overloaded server and congested paths on a centralized network by dividing files into segments. Once a segment is received, each computer becomes a server that can transfer that segment, thus distributing the load. However, the network includes one centralized server responsible for coordinating segment transfers.

1. Key Features
Each file is divided into 16KB segments, and a checksum is calculated for each segment. These details are stored in a single file on the coordinating server.
The logic on the coordinating server tracks which computers have available copies of each file segment.
The file transfer process begins with the application requesting segment location data from the server one segment at a time. After receiving the location of a segment, the application transfers it from the specified computer to the local machine. Upon successful transfer, the coordinating server is notified that the segment is now available on the local machine, and the process continues with the next segment. The checksum is verified after transferring each segment.
2. Concept of the Solution
The first step is to launch the server application, which is responsible for managing the entire network. Upon startup, the server prepares the necessary environment for further operation and client servicing.

When the user starts the application, they are connected to the centralized server. Upon requesting to download a file, the application sends a message to the server informing it of the requested file. The server then responds by providing the resource from which the file should be downloaded. If no peer on the network possesses the requested file, the download will be executed directly from the server. Otherwise, the file will be retrieved from another peer within the network.

3. Description of the Solution
When the central server is started, the first step is to go through all the files stored in the system, divide them into 16KB segments, calculate the checksum for each segment, and store all the resulting data in a specific folder. Afterward, the server makes all the necessary preparations for listening to incoming client connections. Each client that connects is assigned a separate thread that will handle further interactions. Additionally, a thread is created to monitor the presence of all registered clients, ensuring that no client has disconnected from the network. If a disconnected client is found, it is removed from the list of peers from which segments can be obtained.

When the client application is started, it connects to the central server and sends information about the segments it possesses, allowing the server to register the client as a source for those segments. A separate thread is also launched to handle incoming connections and transfers in case of direct peer-to-peer (P2P) file sharing.

The client can send a "GET filename" command to the central server to request the desired file. The server searches through its archive, which contains data on the files and their available sources, selects the most appropriate source, and sends this information back to the client. If no peer on the network has the requested file, the client initiates the transfer directly from the central server. If the server returns the address of a peer on the network from which the file should be obtained, the client launches a separate thread that connects to the designated peer and performs the transfer. After the transfer is completed, whether from the server or a peer, the checksum is verified to ensure a correct transfer. If the transfer is successful, the client informs the server that it can now be added to the list of available sources for that file.

4. Testing
To test the system, follow these steps:

Start the central server and several client applications. Initially, none of the clients will possess any files.
One of the clients requests a file. The server informs the client that the file can only be downloaded from the server, and the transfer is executed.
Another client requests the same file, but since the file is now available on both the server and the first client, the download will be performed from the first client.
The first client then requests a different file, which is downloaded from the server. Manually delete some segments of this file from the first client.
The second client requests this file. For some segments, the source will be the server, and for others, the first client. The test verifies whether the combined transfer from both sources works correctly.
Manually open the downloaded files to ensure that the content has been preserved during transfer.
Now that both the first and second clients possess the file, have a third client request the same file. Observe how the server determines from which source the file should be downloaded.
The system functions as expected, and the peer-to-peer network functionality has been demonstrated successfully.
