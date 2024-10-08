#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <WinSock2.h>
#include <stdio.h>
#include <conio.h>
#include <filesystem>
#include <fstream>
#include <windows.h>
#include <io.h>
#include <sstream>

#include "FSM.h"

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

const char* MAIN_SERVER_IP = "127.0.0.1";  // IP adresa glavnog servera
const int MAIN_SERVER_PORT = 12345;         // Port glavnog servera
const int PEER_SERVER_PORT = 54321;         // Port na kojem klijent očekuje druge klijente

struct PeerInfo {
    SOCKET socket;
    string ipAddress;
    int port;
};

vector<PeerInfo> peers;

FSM peerFSM;

// Poziva se pri povezivanju na server i salje spisak svih segmenata koji se nalaze
// na klijentskoj strani kako bi server mogao da azurira spisak na njegovoj strani
void SendFileListToServer(SOCKET serverSocket, const string& folderPath) {
    struct _finddata_t fileInfo;
    intptr_t fileHandle;

    // Pretraživanje prvog fajla u folderu
    if ((fileHandle = _findfirst((folderPath + "/*").c_str(), &fileInfo)) == -1L) {
        cerr << "Neuspesno otvaranje foldera." << endl;
        return;
    }

    // Prođi kroz folder i pravi spisak fajlova
    vector<string> fileList;
    do {
        if (fileInfo.attrib & _A_ARCH) {  // Da li je regularan fajl
            fileList.push_back(fileInfo.name);
        }
    } while (_findnext(fileHandle, &fileInfo) == 0);
    _findclose(fileHandle);

    // Kreiraj string sa spiskom fajlova
    string fileListString = "";
    for (const auto& fileName : fileList) {
        if(fileListString == "")
            fileListString += fileName;
        else
            fileListString += "\n" + fileName;
    }

    // Pošalji spisak fajlova serveru
    if (send(serverSocket, fileListString.c_str(), fileListString.length(), 0) == SOCKET_ERROR) {
        cerr << "Neuspesno slanje spiska fajlova serveru." << endl;
    }
}

void ListFiles(const string& folderPath) {
    struct _finddata_t fileInfo;
    intptr_t fileHandle;

    // Pretraživanje prvog fajla u folderu
    if ((fileHandle = _findfirst((folderPath + "/*").c_str(), &fileInfo)) == -1L) {
        cerr << "Neuspesno otvaranje foldera." << endl;
        return;
    }

    // Prođi kroz folder i ispisi svaki
    vector<string> fileList;
    do {
        if (fileInfo.attrib & _A_ARCH) {  // Da li je regularan fajl
            cout << fileInfo.name << endl;
        }
    } while (_findnext(fileHandle, &fileInfo) == 0);
    _findclose(fileHandle);
}

std::string CalculateChecksum(const std::vector<char>& data) {
    int sum = 0;
    for (char c : data) {
        sum += static_cast<int>(c);
    }

    std::stringstream stream;
    stream << std::hex << sum;
    return stream.str();
}

void ListenForPeerConnection(int port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize WinSock" << std::endl;
        return;
    }

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create listening socket" << std::endl;
        WSACleanup();
        return;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port+1);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed" << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed" << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return;
    }

    std::cout << "Waiting for peer connection on port: " << port << std::endl;

    while (true) {
        SOCKET clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed" << std::endl;
            continue;
        }

        // POVEZANI
        cout << "Connected" << endl;
        // Prihvatanje naziva datoteke
        char buffer[1024];
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        buffer[bytesRead] = '\0';
        string str(buffer);

        // Otvaranje datoteke za slanje
        string filename = "PeerSegments//" + str + ".dat";

        ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            std::cerr << "Error opening file." << std::endl;
        }

        // Dobijanje veličine datoteke
        std::streampos fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        // Slanje veličine datoteke klijentu
        send(clientSocket, reinterpret_cast<char*>(&fileSize), sizeof(fileSize), 0);

        // Slanje binarne datoteke klijentu
        char buffer1[1024 * 16];
        cout << "Zapoceto slanje " << str << endl;
        while (!file.eof()) {
            file.read(buffer1, sizeof(buffer1));
            send(clientSocket, buffer1, file.gcount(), 0);
        }
        cout << "Zavrseno slanje " << endl;
        //bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        //buffer[bytesRead] = '\0';
        //string str(buffer);
    }

    closesocket(listenSocket);
    WSACleanup();
}

void PeerTransfer(int port, string segment) {
    SOCKET peerServerSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (peerServerSocket == INVALID_SOCKET) {
        cerr << "Neuspesno kreiranje klijentskog soketa za povezivanje na peer." << endl;
        return;
    }

    // Postavljanje adrese glavnog servera
    sockaddr_in peerServerAddr;
    peerServerAddr.sin_family = AF_INET;
    peerServerAddr.sin_addr.s_addr = inet_addr(MAIN_SERVER_IP);
    peerServerAddr.sin_port = htons(port+1);

    // Povezivanje na glavni server
    if (connect(peerServerSocket, (sockaddr*)&peerServerAddr, sizeof(peerServerAddr)) == SOCKET_ERROR) {
        cerr << "Neuspesno povezivanje na peer." << endl;
        closesocket(peerServerSocket);
        return;
    }

    // Dobijanje informacija o lokalnom kraju soketa
    sockaddr_in localAddr;
    int localAddrSize = sizeof(localAddr);
    if (getsockname(peerServerSocket, (sockaddr*)&localAddr, &localAddrSize) == SOCKET_ERROR) {
        cerr << "Greska pri dobijanju informacija o lokalnom kraju soketa." << endl;
        closesocket(peerServerSocket);
        return;
    }

    cout << "Povezan na " << MAIN_SERVER_IP << ":" << port+1 << endl;
    // Slanje naziva segmenta
    send(peerServerSocket, segment.c_str(), segment.length(), 0);

    // Primanje veličine datoteke od servera
    std::streampos fileSize;
    recv(peerServerSocket, reinterpret_cast<char*>(&fileSize), sizeof(fileSize), 0);

    // Otvaranje fajla za primanje
    string fileName = "PeerSegments//" + segment + ".dat";
    ofstream file(fileName, std::ios::binary);

    // Primanje binarne datoteke od servera
    char buffer1[1024 * 16];
    std::streampos bytesRead = 0;
    while (bytesRead < fileSize) {
        int bytesReceived = recv(peerServerSocket, buffer1, sizeof(buffer1), 0);
        file.write(buffer1, bytesReceived);
        bytesRead += bytesReceived;
    }
    cout << "Preuzeto." << endl;

    closesocket(peerServerSocket);
}

void ConnectToMainServer() {
    // Kreiranje soketa za glavni server
    SOCKET mainServerSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (mainServerSocket == INVALID_SOCKET) {
        cerr << "Neuspesno kreiranje klijentskog soketa za glavni server." << endl;
        return;
    }

    // Postavljanje adrese glavnog servera
    sockaddr_in mainServerAddr;
    mainServerAddr.sin_family = AF_INET;
    mainServerAddr.sin_addr.s_addr = inet_addr(MAIN_SERVER_IP);
    mainServerAddr.sin_port = htons(MAIN_SERVER_PORT);

    // Povezivanje na glavni server
    if (connect(mainServerSocket, (sockaddr*)&mainServerAddr, sizeof(mainServerAddr)) == SOCKET_ERROR) {
        cerr << "Neuspesno povezivanje na glavni server." << endl;
        closesocket(mainServerSocket);
        return;
    }

    // Dobijanje informacija o lokalnom kraju soketa
    sockaddr_in localAddr;
    int localAddrSize = sizeof(localAddr);
    if (getsockname(mainServerSocket, (sockaddr*)&localAddr, &localAddrSize) == SOCKET_ERROR) {
        cerr << "Greska pri dobijanju informacija o lokalnom kraju soketa." << endl;
        closesocket(mainServerSocket);
        return;
    }

    cout << "Povezan na glavni server." << endl;

    peerFSM.setState(State::PEER_CONNECTED);

    // =======================================================================
    thread ListenForPeerConnectionThread(ListenForPeerConnection, ntohs(localAddr.sin_port));
    ListenForPeerConnectionThread.detach();

    // Slanje serveru informacije o tome koje datoteke posedujemo
    SendFileListToServer(mainServerSocket, "PeerSegments");

    // GET PEERS - vraca spisak konektovanih klijenata
    // GET naziv_datoteke - zahtev za transferom datoteke
    // GET FILES - vraca datoteke koje poseduje
    // GET STATE - vraca stanje FSM automata
    while (true) {
        string request;
        string msg;
        while (request.empty()) {
            getline(cin, request);
            if (request == "GET STATE") {
                peerFSM.getState();
                request = "\0";
            }
            else if (request == "GET FILES") {
                ListFiles("PeerSegments");
                request = "\0";
            }
        }
        if (send(mainServerSocket, request.c_str(), request.length(), 0) == SOCKET_ERROR) {
            cerr << "Neuspesno slanje zahteva." << endl;
            closesocket(mainServerSocket);
            return;
        }

        // Čitanje odgovora od servera
        char buffer[1024];
        int bytesRead = recv(mainServerSocket, buffer, sizeof(buffer), 0);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';

            // Ukoliko je komanda bila GET naziv_datoteke ...
            string str(buffer);
            string myStr;
            if (str.find("SendStart") != string::npos)
            {
                vector<string> sources;
                while (true) {
                    // u svakoj iteraciji se dobija odakle da se preuzme pojedinacan segment
                    bytesRead = recv(mainServerSocket, buffer, sizeof(buffer), 0);
                    buffer[bytesRead] = '\0';
                    string str(buffer);

                    if (str.find("SendEnd") != string::npos)
                    {
                        if(str.find("Nema trazenog fajla!") != string::npos)
                            cout << buffer << endl;
                        break;
                    }
                    else
                        sources.push_back(str);
                }

                size_t pos = request.find(" ");
                string requestedFile = request.substr(pos + 1);
                // prolazak kroz sve 
                for (size_t i = 0; i < sources.size(); i++) {
                    msg = "segment_" + requestedFile + "_" + to_string(i);
                    cout << msg << " - dobavljanje od: ";

                    // U narednom delu je potrebno obaviti prenos podataka
                    size_t pozicijaRazmaka = sources[i].find(' ');
                    string checkS = sources[i].substr(pozicijaRazmaka + 1);
                    sources[i].erase(pozicijaRazmaka + 1);

                    if (sources[i].find("server") != string::npos) {
                        cout << sources[i] << endl;
                        peerFSM.setState(State::PEER_SERVER_TRANSFER);

                        request = "REQ " + msg + "\0";
                        if (send(mainServerSocket, request.c_str(), request.length(), 0) == SOCKET_ERROR) {
                            cerr << "Neuspesno slanje zahteva." << endl;
                            closesocket(mainServerSocket);
                            return;
                        }

                        // Primanje veličine datoteke od servera
                        std::streampos fileSize;
                        recv(mainServerSocket, reinterpret_cast<char*>(&fileSize), sizeof(fileSize), 0);

                        // Otvaranje fajla za primanje
                        string fileName = "PeerSegments//" + msg + ".dat";
                        ofstream file(fileName, std::ios::binary);

                        // Primanje binarne datoteke od servera
                        char buffer1[1024 * 16];
                        std::streampos bytesRead = 0;
                        while (bytesRead < fileSize) {
                            int bytesReceived = recv(mainServerSocket, buffer1, sizeof(buffer1), 0);
                            file.write(buffer1, bytesReceived);
                            bytesRead += bytesReceived;
                        }

                        // Read the content of the file into a vector
                        ifstream file1(fileName, std::ios::binary);
                        if (!file.is_open()) {
                            std::cerr << "Error opening file: " << fileName << std::endl;
                        }

                        // Racunanje kontrolne sume primljenog fajla
                        vector<char> buffer2(1024 * 16, 0);
                        file1.read(buffer2.data(), 1024 * 16);
                        std::string checksum = CalculateChecksum(buffer2);
                        if (checkS != checksum)
                        {
                            cout << "Kontrolne sume se ne poklapaju!" << endl;
                            msg = "PeerReceivedUnSuccessfully " + requestedFile + "_" + to_string(i);
                        }
                        else
                            msg = "PeerReceivedSuccessfully " + requestedFile + "_" + to_string(i);
                        send(mainServerSocket, msg.c_str(), msg.length(), 0);

                        peerFSM.setState(State::PEER_CONNECTED);
                    }
                    else {
                        peerFSM.setState(State::PEER_2_PEER_TRANSFER);
                        cout << sources[i] << endl;
                        size_t indeks = sources[i].find(":");
                        string ip = sources[i].substr(0, indeks);
                        string port = sources[i].substr(indeks + 1);
                        int port_int = stoi(port);

                        thread peerTransferThread(PeerTransfer, port_int, msg);
                        peerTransferThread.join();

                        // Javi serveru da doda klijenta u spisak onih koji poseduju
                        SendFileListToServer(mainServerSocket, "PeerSegments");
                        peerFSM.setState(State::PEER_CONNECTED);
                    }
                }
            }
            else // Ispisivanje poruke od servera
                cout << "Poruka od servera:" << buffer << endl;
        }
    }

    // Zatvaranje soketa ka glavnom serveru
    closesocket(mainServerSocket);
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "Neuspesno pokretanje WinSock-a." << endl;
        return -1;
    }

    // Pokretanje thread-a za povezivanje na glavni server
    thread connectThread(ConnectToMainServer);

    // Čekanje da se oba thread-a završe
    connectThread.join();

    // Ciscenje WinSock-a
    WSACleanup();

    return 0;
}
