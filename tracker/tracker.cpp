#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <thread>
#include <string>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>
using namespace std;

#include "constants.h"
#include "group.h"
#include "tokenize.h"
#include "user.h"
#include "user_request_processing.h"

vector<string> getSocketInfo(string filename, int trackerNum)
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
    char delimiter[2] = {'\n'};
    read(fileFD, buffer, 64);
    close(fileFD);

    // tokenize on newline and return trackerNum line
    vector<string> config = tokenizeInput(buffer, '\n');
    string configString = config[trackerNum];
    config = tokenizeInput(configString, ':');

    // cleanup and return
    return config;
}

int getServerSocket(int port)
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

void handleClient(int clientSocket, unordered_map<int, string> &clientUserMapping, unordered_map<string, User> &userMapping,
                  unordered_map<string, Group> &groupMapping)
{
    while (true)
    {
        char buffer[ARGS_BUFFER_SIZE + 1];
        string fullMessage;
        int bytesReceived;

        while (true)
        {
            memset(buffer, 0, sizeof(buffer));
            bytesReceived = recv(clientSocket, buffer, ARGS_BUFFER_SIZE, 0);
            if (bytesReceived == 0)
            {
                cout << "LOGGER: Client disconnected.\n";
                close(clientSocket);
                return;
            }
            buffer[bytesReceived] = '\0';

            // Append received chunk to full message
            if (bytesReceived > 0)
                fullMessage.append(buffer, bytesReceived);

            if (buffer[bytesReceived - 1] == '|')
                break;

            // If the client sends "quit", exit the loop
            if (fullMessage == "quit\n" || fullMessage == "quit")
            {
                cout << "LOGGER: Client closed the connection.\n";
                close(clientSocket);
                return;
            }
        }
        if (fullMessage.empty())
            break;
        fullMessage.pop_back();
        cout << "LOGGER: Message received from client: " << fullMessage << "\n";
        thread t(processClientRequest, clientSocket, fullMessage, ref(clientUserMapping), ref(userMapping), ref(groupMapping));
        t.detach();
        // processClientRequest(clientSocket, fullMessage, clientUserMapping, userMapping, groupMapping);
    }
    close(clientSocket);
}

void handleTrackerInput(int serverSocket)
{
    while (true)
    {
        // fetch user input and handle quit command
        string input;
        cin >> input;
        if (input == "quit")
        {
            close(serverSocket);
            exit(0);
        }
    }
}

int main(int argc, char *argv[])
{

    /*VARIABLE DECLARATIONS*/

    // client to user mapping
    unordered_map<int, string> clientUserMapping;

    // mapping of groups that are active
    unordered_map<string, Group> groupMapping;

    // mapping of users that are active
    unordered_map<string, User> userMapping;

    /*BASIC CLI VALIDATION*/
    if (argc != 3)
    {
        cerr << "Invalid number of arguments provided.\n";
        exit(1);
    }
    string configFile = argv[1];
    int trackerNum;
    try
    {
        trackerNum = atoi(argv[2]);
    }
    catch (...)
    {
        cerr << "Invalid tracker number provided.\n";
        exit(1);
    }

    /*SERVER-SIDE SOCKET CREATION*/
    int port = stoi(getSocketInfo(configFile, trackerNum)[1]);
    int serverSocket = getServerSocket(port);
    cout << "Tracker " << trackerNum << " listening on port " << port << "...\n";
    thread input(handleTrackerInput, serverSocket);

    sockaddr_in clientAddress;
    socklen_t clientAddrLen = sizeof(clientAddress);

    while (true)
    {
        // accepting connection request
        int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &clientAddrLen);
        if (clientSocket < 0)
        {
            perror("Accept Connection");
            continue;
        }

        // display client information
        char clientIP[INET_ADDRSTRLEN];
        int clientPort = ntohs(clientAddress.sin_port);
        inet_ntop(AF_INET, &(clientAddress.sin_addr), clientIP, INET_ADDRSTRLEN);
        string clientSocketString = clientIP;
        clientSocketString += ":";
        clientSocketString += to_string(clientPort);
        cout << "LOGGER: Client " << clientIP << " connected from port " << clientPort << "!\n";

        // create new thread to continue the operations performed by this client
        thread t(handleClient, clientSocket, ref(clientUserMapping), ref(userMapping), ref(groupMapping));
        t.detach();
    }
    return 0;
}
