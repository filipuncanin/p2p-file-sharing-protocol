#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <WinSock2.h>
#include <fstream>
#include <sstream>
#include <filesystem>

#include "Segmentation.h"
#include "FSM.h"

#pragma comment(lib, "Ws2_32.lib")

using namespace std;
namespace fs = std::filesystem;

const int PORT = 12345;

struct ClientInfo {
    SOCKET socket;
    string ipAddress;
    int port;
};

vector<ClientInfo> clients;

FSM serverFSM;

void SendPeerListToClient(SOCKET clientSocket) {
    string peerList;

    // Popuni string peerList sa informacijama o registrovanim peer-ovima
    for (const auto& peer : clients) {
        peerList += "\n" + peer.ipAddress + ":" + to_string(peer.port) + ";";
    }

    // Pošalji peerList klijentu
    send(clientSocket, peerList.c_str(), peerList.length(), 0);
}

// Funkcija za proveru i ažuriranje segments_summary.txt fajla
void UpdateSegmentsSummary(const std::string& clientMessage, const std::string& clientAddress, int clientPort) {
    std::ifstream inFile("Segments\\segments_summary.txt");
    std::vector<std::string> lines;

    // Čitanje linija iz fajla
    std::string line;
    while (std::getline(inFile, line)) {
        lines.push_back(line);
    }

    inFile.close();

    // Provera svakog reda iz klijentove poruke
    std::istringstream messageStream(clientMessage);
    std::string segment;
    while (std::getline(messageStream, segment)) {
        // Provera svakog reda iz segments_summary.txt fajla
        for (size_t i = 0; i < lines.size(); ++i) {
            // Uklanjanje '\n' na kraju linije iz fajla
            std::string lineCopy = lines[i];
            lineCopy.erase(std::remove_if(lineCopy.begin(), lineCopy.end(), [](unsigned char x) { return std::isspace(x); }), lineCopy.end());

            if (lineCopy.find(segment) != std::string::npos) {
                // Ako pronađemo odgovarajući segment, ubacujemo IP adresu i port ispod njega
                // proveravamo da li se ipadresa:port vec nalaze tu
                bool found = false;
                bool t = true;
                int j = i + 1;
                if (j == lines.size()) {
                    t = false;
                }
                while (t)
                {
                    if (lines[j].find("segment") != string::npos || j == lines.size() - 1)
                        t = false;
                    if (lines[j].find(clientAddress + ":" + std::to_string(clientPort)) != string::npos)
                    {
                        found = true;
                        t = false;
                    }
                    j++;
                }

                if(!found)
                    lines.insert(lines.begin() + i + 1, clientAddress + ":" + std::to_string(clientPort));
                break;  // Prekidamo petlju ako smo pronašli odgovarajući segment i ubacili vrednost
            }
        }
    }

    // Upisivanje izmenjenog sadržaja u fajl
    std::ofstream outFile("Segments\\segments_summary.txt");
    for (const auto& updatedLine : lines) {
        outFile << updatedLine << std::endl;
    }

    outFile.close();
}

void HandleClient(SOCKET clientSocket, const string& clientIP, int clientPort) {
    string msg;
    while (true) {
        char buffer[1024];
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';

            // Proveri zahtev od klijenta
            string request(buffer);
            if (request == "GET PEERS") {
                // Klijent želi listu registrovanih peer-ova
                SendPeerListToClient(clientSocket);
            }
            else if (request.find("segment") != string::npos && request.find("REQ") == string::npos)
            {
                // Ovo se poziva pri prikljucivanju klijenta serveru kako bi se azurirala datoteka segments_summary.txt
                UpdateSegmentsSummary(request, clientIP, clientPort);
            }
            else if (request.find("GET") != string::npos)
            {
                // Parsiranje poruke i dobijanje naziva_datoteke
                string message(buffer);
                size_t pos = message.find(" ");
                string filename = message.substr(pos + 1);

                cout << "===== Zahtevan prenos datoteke: " << filename << " =====" << endl;

                // Otvori datoteku "Segments/segments_summary.txt" i ispisi odgovarajuće linije
                ifstream file("Segments//segments_summary.txt");
                string line;
                vector<string> sources; // ip adrese koje mozemo da koristimo
                string checksumValue;  // kontrolna suma koja se poredi pri slanju
                string selectedSource;
                bool foundAny = false;

                msg = "SendStart";
                send(clientSocket, msg.c_str(), msg.length(), 0);

                while (std::getline(file, line)) {
                    size_t found = line.find(filename);
                    if (found != string::npos) {
                        foundAny = true;

                        cout << line << endl;

                        // Izdvoji vrednost kontrolne sume iz istog reda
                        size_t checksumPos = line.find("Checksum: ");
                        if (checksumPos != string::npos) {
                            checksumValue = line.substr(checksumPos + 10);
                            //cout << "Vrednost kontrolne sume: " << checksumValue << endl;
                        }

                        // Pratimo trenutni položaj u fajlu
                        std::streampos currentPos = file.tellg();

                        // Čitaj i ispisuj sve redove do narednog reda sa substringom "segment"
                        while (std::getline(file, line) && line.find("segment") == string::npos) {
                            cout << line << endl;
                            sources.push_back(line);
                        }

                        // Odaberi slučajan source ili postavi vrednost na "server" ukoliko je vektor prazan
                        if (!sources.empty() && !(sources.size() == 1 && sources[0] == clientIP + ":" + to_string(clientPort))) {
                            std::srand(std::time(0)); // Postavi seed za srand() na trenutno vreme
                            do {
                                int randomIndex = rand() % sources.size();
                                selectedSource = sources[randomIndex];
                            } while (selectedSource == clientIP + ":" + to_string(clientPort));
                        }
                        else {
                            selectedSource = "server";
                        }

                        selectedSource += " " + checksumValue;
                        send(clientSocket, selectedSource.c_str(), selectedSource.length(), 0);

                        // Postavi fajl na prethodni položaj
                        file.seekg(currentPos);

                        // Isprazni vektor za sledeće pojavljivanje
                        sources.clear();
                    }
                }
                if (!foundAny) {
                    msg = "SendEnd - Nema trazenog fajla!";
                }
                else
                    msg = "SendEnd";

                send(clientSocket, msg.c_str(), msg.length(), 0);
            }
            else if (request.find("REQ") != string::npos)
            {
                // Parsiranje poruke i dobijanje naziva_datoteke
                string message(buffer);
                size_t pos = message.find(" ");
                string filename = message.substr(pos + 1);
                string filename_temp = filename;

                cout << filename << endl;
                filename = "Segments//" + filename + ".dat";

                // Otvaranje fajla za slanje
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
                char buffer[1024 * 16];
                cout << "Zapoceto slanje " << filename_temp << endl;
                while (!file.eof()) {
                    file.read(buffer, sizeof(buffer));
                    send(clientSocket, buffer, file.gcount(), 0);
                }
            }
            else if (request.find("PeerReceivedUnSuccessfully") != string::npos)
            {
                string message(buffer);
                size_t pos = message.find(" ");
                string filename = message.substr(pos + 1);
                cout << "Neuspesno slanje " << filename << endl;
            }
            else if (request.find("PeerReceivedSuccessfully") != string::npos)
            {
                string message(buffer);
                size_t pos = message.find(" ");
                string filename = message.substr(pos + 1);
                cout << "Zavrseno slanje segment_" << filename << endl;
                // Dodajemo klijenta u fajl segments_summary.txt kao dostupni peer
                UpdateSegmentsSummary(filename, clientIP, clientPort);   // message sadrzi segment koji je preuzet
            }
            else {
                // Nepriznata komanda, možete dodati odgovarajuću logiku
                cerr << "Nepoznata komanda od klijenta: " << request << endl;
                msg = "Nepoznata komanda";
                send(clientSocket, msg.c_str(), msg.length(), 0);
            }
        }
    }

    // Zatvaranje soketa klijenta

    closesocket(clientSocket);

    // Ukloni klijenta iz vektora
    for (auto it = clients.begin(); it != clients.end(); ++it) {
        if (it->socket == clientSocket) {
            clients.erase(it);
            break;
        }
    }
}

void CheckDisconnectedClients() {
    while (true) {
        // Kopiraj vektor klijenata kako bi izbegao problem sa konkurencijom
        vector<ClientInfo> copyOfClients = clients;

        for (const auto& client : copyOfClients) {
            char buffer[1];
            int result = recv(client.socket, buffer, sizeof(buffer), MSG_PEEK);

            if (result == 0 || (result == SOCKET_ERROR && WSAGetLastError() == WSAECONNRESET)) {
                // Klijent je otkacen
                cout << "Klijent " << client.ipAddress << ":" << client.port << " se otkacio." << endl;

                // Ukloni klijenta iz vektora
                for (auto it = clients.begin(); it != clients.end(); ++it) {
                    if (it->socket == client.socket) {
                        closesocket(client.socket);
                        clients.erase(it);
                        break;
                    }
                }

                // Otvori fajl za čitanje i pisanje
                std::fstream file("Segments//segments_summary.txt", std::ios::in);

                // Proveri da li je fajl otvoren uspešno
                if (file.is_open()) {
                    std::string line;
                    std::string ipAddressPort = client.ipAddress + ":" + std::to_string(client.port);

                    // Kreći se kroz fajl i ukloni redove koji sadrže IP adresu i port klijenta
                    std::ofstream tempFile("Segments//segments_summary_temp.txt", std::ios::out);
                    while (std::getline(file, line)) {
                        if (line.find(ipAddressPort) == std::string::npos) {
                            tempFile << line << std::endl;
                        }
                    }

                    // Zatvori fajlove
                    file.close();
                    tempFile.close();

                    // Zamena originalnog fajla sa privremenim
                    std::remove("Segments//segments_summary.txt");
                    std::rename("Segments//segments_summary_temp.txt", "Segments//segments_summary.txt");
                }
                else {
                    std::cerr << "Nije moguće otvoriti fajl 'Segments//segments_summary.txt'." << std::endl;
                }
            }
        }

        // Pauziraj thread na neko vreme
        this_thread::sleep_for(chrono::seconds(5));
    }
}

void CheckFSMState() {
    string request;
    getline(cin, request);
    if (request == "GET STATE") {
        serverFSM.getState(); 
    }

}

int main() {
    std::string inputFolder = "Data"; // Postavi putanju do foldera sa datotekama
    std::string outputFolder = "Segments"; // Postavi putanju do foldera gde želiš sačuvati segmente

    // Segmentacija svih fajlova prilikom pokretanja servera
    Segmentation::segmentFiles(inputFolder, outputFolder);

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "Neuspesno pokretanje WinSock-a." << endl;
        return -1;
    }

    // Kreiranje soketa
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        cerr << "Neuspesno kreiranje server soketa." << endl;
        WSACleanup();
        return -1;
    }

    // Postavljanje adrese servera
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    // Povezivanje soketa sa adresom
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Neuspesno povezivanje server soketa." << endl;
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }

    // Slusanje na soketu
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "Neuspesno pokretanje slusanja na server soketu." << endl;
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }

    cout << "Server je pokrenut i slusa na portu " << PORT << "." << endl;

    // Pokretanje thread-a za proveru otkacenih klijenata
    thread disconnectThread(CheckDisconnectedClients);

    thread checkFSMThread(CheckFSMState);

    while (true) {
        // Prihvatanje klijenta
        sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket == INVALID_SOCKET) {
            cerr << "Neuspesno prihvatanje klijenta." << endl;
            closesocket(serverSocket);
            WSACleanup();
            return -1;
        }

        // Dodavanje klijenta u vektor
        ClientInfo newClient;
        newClient.socket = clientSocket;
        newClient.ipAddress = inet_ntoa(clientAddr.sin_addr);
        newClient.port = ntohs(clientAddr.sin_port);
        clients.push_back(newClient);

        cout << "Client connected... Socket: " << newClient.socket << " ip: " << newClient.ipAddress << " port: " << newClient.port << endl;

        // Pokretanje thread-a za obradu klijenta
        thread clientThread(HandleClient, clientSocket, newClient.ipAddress, newClient.port);
        clientThread.detach();
    }

    // Zatvaranje server soketa
    closesocket(serverSocket);

    // Ciscenje WinSock-a
    WSACleanup();

    return 0;
}
