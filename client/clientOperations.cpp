#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <math.h>
#include <openssl/sha.h>
#include <string>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>
using namespace std;

typedef unsigned char Byte;

#include "clientFile.h"
#include "../tracker/constants.h"
#include "../tracker/tokenize.h"

extern unordered_map<string, ClientFile> fileMapping;

void handlePeerDownloadRequest(int peerSocket, string peerCommand)
{
    vector<string> innerCommand = tokenizeInput(peerCommand, ';');
    int numArgs = innerCommand.size();

    // check number of arguments
    if (numArgs != 2)
    {
        cerr << "Invalid number of arguments provided.\n";
        return;
    }
    // get file path and index
    string filePath = innerCommand[0];
    if (fileMapping.find(filePath) == fileMapping.end())
    {
        cerr << "File not seeded.\n";
        return;
    }
    int pieceIndex = stoi(innerCommand[1]);

    // send the response to the peer
    int fileFd = open(filePath.c_str(), O_RDONLY);
    ssize_t bytesRead, bytesSent, totalBytesRead = 0;
    off_t end = lseek(fileFd, 0, SEEK_END);
    lseek(fileFd, pieceIndex * FILE_PIECE_SIZE, SEEK_SET);
    Byte filebuf[ARGS_BUFFER_SIZE];

    while (totalBytesRead < FILE_PIECE_SIZE && lseek(fileFd, 0, SEEK_CUR) < end)
    {
        bytesRead = read(fileFd, filebuf, ARGS_BUFFER_SIZE);
        totalBytesRead += bytesRead;
        bytesSent = send(peerSocket, filebuf, bytesRead, 0);
    }
    // cout << "Sent " << totalBytesRead << " bytes to peer.\n";

    close(fileFd);
    close(peerSocket);
}

void handleUploadToTracker(int trackerSocket, vector<string> userCommand, string socketString)
{
    int numArgs = userCommand.size();
    if (numArgs != 3)
    {
        cerr << "Invalid number of arguments provided.\n";
        return;
    }
    string filePath = userCommand[1];
    string groupName = userCommand[2];

    // check if file exists
    struct stat buffer;
    if (stat(filePath.c_str(), &buffer) < 0)
    {
        perror("Read file");
        return;
    }

    // create new file object
    fileMapping[filePath] = ClientFile(filePath, groupName);

    // create request to be sent to the tracker
    string request = "";
    for (int i = 0; i < numArgs; i++)
    {
        request += string(userCommand[i]);
        request += " ";
    }
    request += socketString;
    request += ";";
    request += to_string(fileMapping[filePath].numPieces);
    request += ";";

    // read the file from local filesystem and generate the hash
    int fileFD = open(filePath.c_str(), O_RDONLY);
    ssize_t bytesRead = 0;
    Byte filebuf[FILE_PIECE_SIZE];
    int end = lseek(fileFD, 0, SEEK_END);
    lseek(fileFD, 0, SEEK_SET);
    while (lseek(fileFD, 0, SEEK_CUR) < end)
    {
        bytesRead = read(fileFD, filebuf, FILE_PIECE_SIZE);
        Byte hash[SHA_DIGEST_LENGTH];
        SHA1(filebuf, bytesRead, hash);
        for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
        {
            char buf[3];
            sprintf(buf, "%02x", hash[i]);
            request += string(buf);
        }
    }
    close(fileFD);
    request += "|";

    // send the request to the tracker
    int piecesToSend = ceil(request.size() * 1.0 / ARGS_BUFFER_SIZE);
    for (int i = 0; i < piecesToSend; i++)
    {
        string sendString = request.substr(i * ARGS_BUFFER_SIZE, ARGS_BUFFER_SIZE);
        send(trackerSocket, sendString.c_str(), sendString.length(), 0);
    }

    char trackerResponse[ARGS_BUFFER_SIZE];
    int bytesReceived = recv(trackerSocket, trackerResponse, ARGS_BUFFER_SIZE, 0);
    trackerResponse[bytesReceived] = '\0';
    cout << "Response from Tracker => " << trackerResponse << "\n";
}
