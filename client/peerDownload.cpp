#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <math.h>
#include <netinet/in.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>
#include <unordered_map>
using namespace std;

#include "clientFile.h"
#include "clientOperations.h"
#include "../tracker/constants.h"
#include "../tracker/tokenize.h"

extern unordered_map<string, ClientFile> fileMapping;
typedef unsigned char Byte;

// get a socket to talk to the seeder
int getSeederSocket(string IP, int port)
{
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = inet_addr(IP.c_str());

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        perror("Seeder connection");
    return sock;
}

// get the name of the file given the full path
string getFileName(string filePath)
{
    vector<string> path = tokenizeInput(filePath, '/');
    return path[path.size() - 1];
}

void downloadPieceFromPeer(int pieceIndex, string peerSocket, string filePath, string destPath, string realHash)
{
    string currentIndexPath = getFileName(destPath);
    currentIndexPath += "_";
    currentIndexPath += to_string(pieceIndex);

    char peerSocketBuf[32];
    strcpy(peerSocketBuf, peerSocket.c_str());

    vector<string> config = tokenizeInput(peerSocketBuf, ':');
    string IP = config[0];
    int port = stoi(config[1]);

    int seederSocket = getSeederSocket(IP, port);

    string request = filePath;
    request += ";";
    request += to_string(pieceIndex);
    send(seederSocket, request.c_str(), request.length(), 0);

    Byte pieceContent[FILE_PIECE_SIZE];
    int pieceContentSize = 0;
    int bytesReceived = 0;

    while (true)
    {
        Byte seederResponse[ARGS_BUFFER_SIZE];
        bytesReceived = recv(seederSocket, seederResponse, ARGS_BUFFER_SIZE, 0);
        if (bytesReceived == 0)
            break;
        memcpy(pieceContent + pieceContentSize, seederResponse, bytesReceived);
        pieceContentSize += bytesReceived;
    }
    // cout << "Received " << pieceContentSize << " bytes from peer for piece index " << pieceIndex << ".\n";
    close(seederSocket);

    // verify the hash of the piece
    Byte pieceHash[SHA_DIGEST_LENGTH];
    SHA1(pieceContent, pieceContentSize, pieceHash);
    string calculatedHash = "";
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
    {
        char buf[3];
        sprintf(buf, "%02x", pieceHash[i]);
        calculatedHash += string(buf);
    }

    if (calculatedHash != realHash)
    {
        cerr << "Piece hash does not match: " << calculatedHash << "\n";
        return;
    }

    // open the current small chunk for writing
    int pieceFd = open(currentIndexPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (pieceFd < 0)
        perror(("Index " + to_string(pieceIndex)).c_str());

    write(pieceFd, pieceContent, pieceContentSize);
    close(pieceFd);
}

void handleDownloadFromPeers(int trackerSocket, string userCommand, string mySocket)
{

    ClientFile localFile;

    // validations
    vector<string> userCommandToken = tokenizeInput(userCommand, ' ');

    if (userCommandToken.size() != 4)
    {
        cerr << "Invalid number of arguments provided.\n";
        return;
    }
    vector<string> userCommandVector;
    string request = "";
    int count = 0;
    while (count < 3)
    {
        userCommandVector.push_back(userCommandToken[count]);
        request += userCommandToken[count];
        request += " ";
        count++;
    }
    request.pop_back();
    request += "|";
    userCommandVector.push_back(userCommandToken[count]);
    if (fileMapping.find(userCommandVector[2]) != fileMapping.end())
    {
        cerr << "Requested file is already present on the system.\n";
        return;
    }

    // send user command to the tracker and get the response
    send(trackerSocket, request.c_str(), request.length(), 0);

    string trackerResponseString = "";
    while (true)
    {
        char trackerResponse[ARGS_BUFFER_SIZE];
        int bytesReceived = recv(trackerSocket, trackerResponse, ARGS_BUFFER_SIZE, 0);
        trackerResponse[bytesReceived] = '\0';

        if (bytesReceived == 0)
            break;
        if (bytesReceived > 0)
            trackerResponseString.append(trackerResponse, bytesReceived);

        if (trackerResponse[bytesReceived - 1] == '|')
            break;
    }
    trackerResponseString.pop_back();

    vector<string> trackerResponseTokens = tokenizeInput(trackerResponseString, ' ');
    int num1Args = trackerResponseTokens.size();

    // if error response, print and exit
    if (num1Args > 1)
    {
        cerr << trackerResponseString << "\n";
        return;
    }

    // else parse and get the information
    trackerResponseTokens = tokenizeInput(trackerResponseString, ';');
    localFile.numPieces = stoi(trackerResponseTokens[0]);
    localFile.groupName = userCommandVector[1];
    localFile.path = userCommandVector[2];
    localFile.fileStatus = 'D';
    fileMapping[localFile.path] = localFile;
    string destinationPath = userCommandVector[3];

    vector<string> fileHashSequence(localFile.numPieces);
    string hashSequence = trackerResponseTokens[1];
    for (int i = 0; i < localFile.numPieces; i++)
        fileHashSequence[i] = hashSequence.substr(i * SHA_DIGEST_LENGTH * 2, SHA_DIGEST_LENGTH * 2);

    vector<string> seederSockets;
    for (count = 2; count < trackerResponseTokens.size(); count++)
        seederSockets.push_back(trackerResponseTokens[count]);

    // create a thread to connect to each peer and get the corresponding file piece
    // also check the SHA1 hash before adding the piece to the main file
    vector<thread> fileDownloadThreads;
    cout << "Download in progress...\n";
    for (int i = 0; i < localFile.numPieces; i++)
    {
        int randomSeederIndex = rand() % seederSockets.size();
        thread t(downloadPieceFromPeer, i, seederSockets[randomSeederIndex], localFile.path, destinationPath, fileHashSequence[i]);
        fileDownloadThreads.push_back(move(t));
        if (fileDownloadThreads.size() == MAX_THREADS)
        {
            for (ssize_t j = 0; j < fileDownloadThreads.size(); j++)
                fileDownloadThreads[j].join();
            fileDownloadThreads.clear();
        }
    }

    // once file is complete, write to local filesystem and exit
    for (int i = 0; i < int(fileDownloadThreads.size()); i++)
        fileDownloadThreads[i].join();

    int destFd = open(destinationPath.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (destFd < 0)
    {
        perror("Destination file");
        return;
    }

    string fileName = getFileName(destinationPath);
    fileName += "_";
    bool fileAssemblyFailed = false;

    // combine individual pieces to make main file
    // and remove the individual pieces
    for (int i = 0; i < localFile.numPieces; i++)
    {
        string pieceFile = fileName;
        pieceFile += to_string(i);

        int fd = open(pieceFile.c_str(), O_RDONLY);
        if (fd < 0)
        {
            perror(("File assembly: " + pieceFile).c_str());
            fileAssemblyFailed = true;
            break;
        }
        Byte buffer[FILE_PIECE_SIZE];
        int bytesRead = read(fd, buffer, FILE_PIECE_SIZE);
        write(destFd, buffer, bytesRead);
        close(fd);

        remove(pieceFile.c_str());
    }

    if (!fileAssemblyFailed)
    {
        // make this peer as a seeder
        string requestToTracker = "upload_file ";
        requestToTracker += localFile.path;
        requestToTracker += " ";
        requestToTracker += localFile.groupName;
        requestToTracker += " ";
        requestToTracker += mySocket;
        requestToTracker += ";";
        requestToTracker += to_string(localFile.numPieces);
        requestToTracker += ";";
        requestToTracker += hashSequence;
        requestToTracker += "|";

        int piecesToSend = ceil(requestToTracker.size() * 1.0 / ARGS_BUFFER_SIZE);
        for (int i = 0; i < piecesToSend; i++)
        {
            string sendString = requestToTracker.substr(i * ARGS_BUFFER_SIZE, ARGS_BUFFER_SIZE);
            send(trackerSocket, sendString.c_str(), sendString.length(), 0);
        }

        char newTrackerResponse[ARGS_BUFFER_SIZE];
        int bytesReceivedTracker = recv(trackerSocket, newTrackerResponse, ARGS_BUFFER_SIZE, 0);
        newTrackerResponse[bytesReceivedTracker] = '\0';
        // cout << "Response from Tracker => " << newTrackerResponse << "\n";

        // mark file download as complete
        fileMapping[localFile.path].fileStatus = 'C';
        cout << "File download complete!\n";
    }
    else
    {
        fileMapping.erase(localFile.path);
        cout << "File download failed.\n";
    }

    // cleanup
    fileDownloadThreads.clear();
}
