#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <time.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>
using namespace std;

#include "../tracker/constants.h"
#include "../tracker/tokenize.h"
#include "clientFile.h"
#include "clientOperations.h"
#include "peerDownload.h"

#define DEFAULT_TRACKER_NUM 0

// map files that the peer is aware of
unordered_map<string, ClientFile> fileMapping;

// get socket information from file
vector<string> getSocketInfo(string filename)
{
    // open file in read mode
    int fileFD = open(filename.c_str(), O_RDONLY);
    if (fileFD < 0)
    {
        perror("Tracker Config");
        exit(1);
    }

    // read entire file into buffer
    char buffer[64];
    read(fileFD, buffer, 64);
    close(fileFD);

    // tokenize on newline and return trackerNum line
    vector<string> bufferVector = tokenizeInput(buffer, '\n');
    string socket = bufferVector[DEFAULT_TRACKER_NUM];

    return tokenizeInput(socket, ':');
}

// get a socket to listen to incoming requests from peers
int getPeerSocket(int port)
{
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    // specifying the address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // binding socket.
    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        perror("Socket Bind");
        exit(1);
    }

    // listening to the assigned socket
    if (listen(serverSocket, MAX_THREADS) < 0)
    {
        perror("Socket Listener");
        exit(1);
    }

    return serverSocket;
}

// get a socket to talk to the tracker
int getTrackerSocket(string IP, int port)
{
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = inet_addr(IP.c_str());

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("Tracker connection");
        exit(1);
    }
    return sock;
}

// handle communication from peers
void handlePeerConnection(int peerSocket)
{
    while (true)
    {
        // accept new connections on the peerSocket
        int currentPeerSocket = accept(peerSocket, nullptr, nullptr);
        if (currentPeerSocket < 0)
        {
            perror("Accept connection");
            continue;
        }

        // receiving data
        char peerMessage[ARGS_BUFFER_SIZE];
        memset(peerMessage, 0, sizeof(peerMessage));
        int bytesReceived = recv(currentPeerSocket, peerMessage, ARGS_BUFFER_SIZE, 0);
        peerMessage[bytesReceived] = '\0';
        if (!strlen(peerMessage))
        {
            cout << "LOGGER: Peer Disconnected or sent empty message.\n";
            break;
        }

        // create a new thread to handle communication with the peer
        thread t(handlePeerDownloadRequest, currentPeerSocket, string(peerMessage));
        t.detach();
    }
}

int main(int argc, char *argv[])
{
    srand(time(0));
    if (argc != 3)
    {
        cerr << "Invalid number of arguments provided.\n";
        exit(1);
    }

    // parse CLI arguments and get a socket that needs to be listened to
    string socketString = string(argv[1]);
    int port;
    try
    {
        port = stoi(tokenizeInput(socketString, ':')[1]);
    }
    catch (...)
    {
        cerr << "Invalid port provided.\n";
        exit(1);
    }
    int peerSocket = getPeerSocket(port);
    cout << "Listening to peers on port " << port << "...\n";
    thread t(handlePeerConnection, peerSocket);
    t.detach();

    // parse tracker_info file and get a socket to talk to the tracker
    vector<string> trackerConfig = getSocketInfo(string(argv[2]));
    int trackerSocket = getTrackerSocket(trackerConfig[0], stoi(trackerConfig[1]));
    cout << "Connected to tracker " << DEFAULT_TRACKER_NUM << ".\n";

    size_t bufferSize = 0;
    while (true)
    {
        string userInput;
        char trackerResponse[ARGS_BUFFER_SIZE];

        // get command from user
        getline(cin, userInput);
        if (!userInput.length())
            continue;

        // tokenize command
        vector<string> innerCommand = tokenizeInput(userInput, ' ');
        if (innerCommand[0] == "download_file")
        {
            // handle download from peer command
            thread t(handleDownloadFromPeers, trackerSocket, userInput, string(argv[1]));
            t.detach();
        }
        else if (innerCommand[0] == "upload_file")
        {
            // handle upload to tracker command
            thread t(handleUploadToTracker, trackerSocket, innerCommand, string(argv[1]));
            t.detach();
        }
        else if (innerCommand[0] == "show_downloads")
        {
            // handle show_downloads command
            for (auto itr : fileMapping)
                if (itr.second.fileStatus != 'L')
                    cout << "[" << itr.second.fileStatus << "] " << itr.second.groupName << " " << itr.second.path << "\n";
        }
        else
        {
            if (innerCommand[0] == "login" || innerCommand[0] == "logout" || innerCommand[0] == "stop_sharing")
            {
                userInput += " ";
                userInput += string(argv[1]);
            }
            userInput += "|";
            // send the command to the tracker and receive a response
            send(trackerSocket, userInput.c_str(), userInput.length(), 0);
            if (innerCommand[0] == "quit")
            {
                cout << "Exiting...\n";
                break;
            }
            int bytesReceived = recv(trackerSocket, trackerResponse, ARGS_BUFFER_SIZE, 0);
            trackerResponse[bytesReceived] = '\0';
            cout << "Response from Tracker => " << trackerResponse << "\n";
        }
    }

    close(peerSocket);
    close(trackerSocket);
    return 0;
}
