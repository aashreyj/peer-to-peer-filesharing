#include <iostream>
#include <math.h>
#include <string>
#include <string.h>
#include <sys/socket.h>
#include <unordered_map>
#include <vector>
using namespace std;

#include "constants.h"
#include "group.h"
#include "tokenize.h"
#include "torrentFile.h"
#include "user.h"
#include "user_request_processing.h"

void sendMessageToClient(int clientSocket, const string message)
{
    cout << "RESPONSE: " << message << "\n";
    send(clientSocket, message.c_str(), message.size(), 0);
}

void processClientRequest(int clientSocket, string command, unordered_map<int, string> &clientUserMapping,
                          unordered_map<string, User> &userMapping, unordered_map<string, Group> &groupMapping)
{
    vector<string> innerCommand = tokenizeInput(command, ' ');
    int numArgs = innerCommand.size();

    if (innerCommand[0] == "create_user")
    {
        // handle create_user command

        // check num of args and extract to string variables
        if (numArgs != 3)
        {
            // return error message to client
            sendMessageToClient(clientSocket, "Invalid number of arguments provided.");
            return;
        }

        string userId = innerCommand[1];
        string passwd = innerCommand[2];

        // check if user already exists
        if (userMapping.find(userId) != userMapping.end())
        {
            // return error to client
            sendMessageToClient(clientSocket, "Requested user already exists.");
            return;
        }

        // else create new user and add entry to mapping structures
        User newUser(userId, passwd);
        userMapping[userId] = newUser;

        // return success message to client
        sendMessageToClient(clientSocket, "Successfully created new user.");
    }
    else if (innerCommand[0] == "login")
    {
        // handle login command

        // check num of args and extract to string variables
        if (numArgs != 4)
        {
            sendMessageToClient(clientSocket, "Invalid number of arguments provided.");
            return;
        }

        string userId = innerCommand[1];
        string passwd = innerCommand[2];
        string clientSocketString = innerCommand[3];

        // check if user exists
        if (userMapping.find(userId) == userMapping.end())
        {
            // return error to client
            sendMessageToClient(clientSocket, "User does not exist.");
            return;
        }

        // return message if user is already logged in
        if (clientUserMapping.find(clientSocket) != clientUserMapping.end())
        {
            // return error to client
            sendMessageToClient(clientSocket, "User is already logged in.");
            return;
        }

        User &user = userMapping[userId];

        // try authentication with provided password
        // and return response to the client
        if (!user.loginAttempt(userId, passwd))
        {
            // failure
            sendMessageToClient(clientSocket, "Unable to login: invalid credentials");
            return;
        }

        // login the user
        clientUserMapping[clientSocket] = userId;

        // mark files as sharable
        for (auto itr : user.groups)
        {
            for (auto &iter : groupMapping[itr].files)
            {
                if (iter.second.seeders.find(clientSocketString) != iter.second.seeders.end())
                    iter.second.seeders[clientSocketString].isSharing = true;
            }
        }

        // return successful message to client
        sendMessageToClient(clientSocket, "Successfully logged in.");
    }
    else if (innerCommand[0] == "create_group")
    {
        // handle create_group command

        // check num of args and extract to string variables
        if (numArgs != 2)
        {
            // return error to client
            sendMessageToClient(clientSocket, "Invalid number of arguments provided.");
            return;
        }

        string groupId = innerCommand[1];

        // check if group exists
        if (groupMapping.find(groupId) != groupMapping.end())
        {
            // return error to client
            sendMessageToClient(clientSocket, "Group already exists.");
            return;
        }

        // check if owner is logged in
        if (clientUserMapping.find(clientSocket) == clientUserMapping.end())
        {
            // return error to client
            sendMessageToClient(clientSocket, "User is not logged in.");
            return;
        }

        User &user = userMapping[clientUserMapping[clientSocket]];

        // create group and add to mapping structures
        groupMapping[groupId] = Group(groupId, user.userId);
        user.groups.insert(groupId);

        // return success to client
        sendMessageToClient(clientSocket, "Successfully created new group.");
    }
    else if (innerCommand[0] == "join_group")
    {
        // handle join_group command

        // check num of args and extract to string variables
        if (numArgs != 2)
        {
            // return error to client
            sendMessageToClient(clientSocket, "Invalid number of arguments provided.");
            return;
        }
        string groupId = innerCommand[1];

        // check if user is logged in
        if (clientUserMapping.find(clientSocket) == clientUserMapping.end())
        {
            // return error to client
            sendMessageToClient(clientSocket, "User is not logged in.");
            return;
        }

        // check if group exists
        if (groupMapping.find(groupId) == groupMapping.end())
        {
            // return error to client
            sendMessageToClient(clientSocket, "Group does not exist.");
            return;
        }

        // check if user is already a member of the group
        User &user = userMapping[clientUserMapping[clientSocket]];
        Group &group = groupMapping[groupId];
        if (group.members.find(user.userId) != group.members.end())
        {
            sendMessageToClient(clientSocket, "User is already a member of the group.");
            return;
        }

        // check if user's joining request is pending
        if (group.joiningRequests.find(user.userId) != group.joiningRequests.end())
        {
            // return error to client
            sendMessageToClient(clientSocket, "User joining request is pending.");
            return;
        }

        // create new joining request for the user
        group.join(user);

        // return success to client
        sendMessageToClient(clientSocket, "Successfully submitted joining request.");
    }
    else if (innerCommand[0] == "leave_group")
    {
        // handle leave_group command

        // check num of args and extract to string variables
        if (numArgs != 2)
        {
            // return error to client
            sendMessageToClient(clientSocket, "Invalid number of arguments provided.");
            return;
        }
        string groupId = innerCommand[1];

        // check if user is logged in
        if (clientUserMapping.find(clientSocket) == clientUserMapping.end())
        {
            // return error to client
            sendMessageToClient(clientSocket, "User is not logged in.");
            return;
        }

        // check if group exists
        if (groupMapping.find(groupId) == groupMapping.end())
        {
            // return error to client
            sendMessageToClient(clientSocket, "Group does not exist.");
            return;
        }

        User &user = userMapping[clientUserMapping[clientSocket]];
        Group &group = groupMapping[groupId];

        // check if the user is not a member of the group
        if (group.members.find(user.userId) == group.members.end())
        {
            // return error to client
            sendMessageToClient(clientSocket, "User is not a member of the group.");
            return;
        }

        // check if the user is the last member of the group
        if (group.members.size() == 1 && group.ownerId == user.userId)
        {
            // return error to client
            sendMessageToClient(clientSocket, "User is the owner and last member of the group.");
            return;
        }

        // remove user from the group
        group.leave(user);

        // return success to client
        sendMessageToClient(clientSocket, "Successfully left the group.");
    }
    else if (innerCommand[0] == "list_requests")
    {
        // handle list_requests command

        if (numArgs != 2)
        {
            // return error to client
            sendMessageToClient(clientSocket, "Invalid number of arguments provided.");
            return;
        }
        string groupId = innerCommand[1];

        // check if user is logged in
        if (clientUserMapping.find(clientSocket) == clientUserMapping.end())
        {
            // return error to client
            sendMessageToClient(clientSocket, "User is not logged in.");
            return;
        }

        // check if group exists
        if (groupMapping.find(groupId) == groupMapping.end())
        {
            // return error to client
            sendMessageToClient(clientSocket, "Group does not exist.");
            return;
        }

        User &user = userMapping[clientUserMapping[clientSocket]];
        Group &group = groupMapping[groupId];

        // check if the user is the owner of the group
        if (group.ownerId != user.userId)
        {
            // return error to client
            sendMessageToClient(clientSocket, "User is not owner of the group.");
            return;
        }

        // check if any requests are present
        if (group.listJoiningRequests().empty())
        {
            sendMessageToClient(clientSocket, "No joining requests exist.");
            return;
        }

        // collect response and return to the client
        string responseString = "";
        for (auto itr : group.listJoiningRequests())
        {
            responseString += itr;
            responseString += ";";
        }
        responseString.pop_back();

        // return response to user
        sendMessageToClient(clientSocket, responseString);
    }
    else if (innerCommand[0] == "accept_request")
    {
        // handle accept request command

        // check num of args and extract to string variables
        if (numArgs != 3)
        {
            // return error to client
            sendMessageToClient(clientSocket, "Invalid number of arguments provided.");
            return;
        }

        string groupId = innerCommand[1];
        string userId = innerCommand[2];

        // check if user is logged in
        if (clientUserMapping.find(clientSocket) == clientUserMapping.end())
        {
            // return error to client
            sendMessageToClient(clientSocket, "User is not logged in.");
            return;
        }

        // check if group exists
        if (groupMapping.find(groupId) == groupMapping.end())
        {
            // return error to client
            sendMessageToClient(clientSocket, "Group does not exist.");
            return;
        }

        // check if requested user exists
        if (userMapping.find(userId) == userMapping.end())
        {
            // return error to client
            sendMessageToClient(clientSocket, "Requested user does not exist.");
            return;
        }

        User &user = userMapping[clientUserMapping[clientSocket]];
        Group &group = groupMapping[groupId];
        User &requestedUser = userMapping[userId];

        // check if the user is the owner of the group
        if (group.ownerId != user.userId)
        {
            // return error to client
            sendMessageToClient(clientSocket, "User is not the owner of the group.");
            return;
        }

        // check if the joining request exists
        if (group.joiningRequests.find(userId) == group.joiningRequests.end())
        {
            sendMessageToClient(clientSocket, "User has not submitted joining request for the group.");
            return;
        }

        // accept the joining request
        group.acceptJoiningRequest(requestedUser);

        // return success message to user
        sendMessageToClient(clientSocket, "Successfully added the user to the group.");
    }
    else if (innerCommand[0] == "list_groups")
    {
        // handle list_groups command

        if (numArgs != 1)
        {
            // return error to client
            sendMessageToClient(clientSocket, "Invalid number of arguments provided.");
            return;
        }

        // check if any groups exist
        if (groupMapping.empty())
        {
            sendMessageToClient(clientSocket, "No groups exist.");
            return;
        }

        // check if user is logged in
        if (clientUserMapping.find(clientSocket) == clientUserMapping.end())
        {
            // return error to client
            sendMessageToClient(clientSocket, "User is not logged in.");
            return;
        }

        // collect all groups and return to the user
        string response = "";
        for (auto itr : groupMapping)
        {
            response += itr.first;
            response += ";";
        }
        response.pop_back();

        // return response to user
        sendMessageToClient(clientSocket, response);
    }
    else if (innerCommand[0] == "logout")
    {
        // handle logout request

        if (numArgs != 2)
        {
            // return error to client
            sendMessageToClient(clientSocket, "Invalid number of arguments provided.");
            return;
        }

        string clientSocketString = innerCommand[1];

        // check if the user is logged in
        if (clientUserMapping.find(clientSocket) == clientUserMapping.end())
        {
            // return error to client
            sendMessageToClient(clientSocket, "User is not logged in.");
            return;
        }

        User &user = userMapping[clientUserMapping[clientSocket]];

        // mark all files where the user is a seeder as not sharing
        for (auto itr : user.groups)
        {
            for (auto &iter : groupMapping[itr].files)
            {
                if (iter.second.seeders.find(clientSocketString) != iter.second.seeders.end())
                    iter.second.seeders[clientSocketString].isSharing = false;
            }
        }

        // logout the user
        clientUserMapping.erase(clientSocket);

        // return success message to client
        sendMessageToClient(clientSocket, "Successfully logged out.");
    }
    else if (innerCommand[0] == "list_files")
    {
        // handle list_files command

        if (numArgs != 2)
        {
            // return error to client
            sendMessageToClient(clientSocket, "Invalid number of arguments provided");
            return;
        }

        // check if any groups exist
        if (groupMapping.empty())
        {
            sendMessageToClient(clientSocket, "No groups exist.");
            return;
        }
        string groupId = innerCommand[1];

        // check if user is logged in
        if (clientUserMapping.find(clientSocket) == clientUserMapping.end())
        {
            // return error to client
            sendMessageToClient(clientSocket, "User is not logged in.");
            return;
        }

        Group group = groupMapping[groupId];
        User user = userMapping[clientUserMapping[clientSocket]];

        // check if user is a member of the group
        if (group.members.find(user.userId) == group.members.end())
        {
            sendMessageToClient(clientSocket, "User is not member of the group.");
            return;
        }

        // collect response and return to the client
        string response = "";

        // check if any files are shared
        if (!group.files.size())
        {
            // return error to client
            sendMessageToClient(clientSocket, "No files are shared.");
            return;
        }

        // get all sharable files
        for (auto itr : group.files)
        {
            bool isAtLeastOneSharing = false;
            for (auto itrNew : itr.second.seeders)
            {
                if (itrNew.second.isSharing)
                {
                    isAtLeastOneSharing = true;
                    break;
                }
            }

            if (isAtLeastOneSharing)
            {
                response += itr.first;
                response += ";";
            }
        }

        // if there are no seeders for any files
        if (!response.length())
        {
            // return error to client
            sendMessageToClient(clientSocket, "No seeders for any shared file.");
            return;
        }

        response.pop_back();
        // return response to user
        sendMessageToClient(clientSocket, response);
    }
    else if (innerCommand[0] == "stop_sharing")
    {
        // handle stop_sharing request

        if (numArgs != 4)
        {
            // return error to client
            sendMessageToClient(clientSocket, "Invalid number of arguments provided.");
            return;
        }

        // check if user is logged in
        if (clientUserMapping.find(clientSocket) == clientUserMapping.end())
        {
            // return error to client
            sendMessageToClient(clientSocket, "User is not logged in.");
            return;
        }

        string groupId = innerCommand[1];
        // check if group exists
        if (groupMapping.find(groupId) == groupMapping.end())
        {
            // return error to client
            sendMessageToClient(clientSocket, "Group does not exist.");
            return;
        }

        User user = userMapping[clientUserMapping[clientSocket]];
        Group &group = groupMapping[groupId];

        // check if user is member of group
        if (group.members.find(user.userId) == group.members.end())
        {
            sendMessageToClient(clientSocket, "User is not member of the group.");
            return;
        }

        // check if file is shared
        string fileName = innerCommand[2];
        if (group.files.find(fileName) == group.files.end())
        {
            // return error to client
            sendMessageToClient(clientSocket, "File is not shared in the group.");
            return;
        }

        string clientSocketString = innerCommand[3];

        // check if user is seeder
        if (group.files[fileName].seeders.find(clientSocketString) == group.files[fileName].seeders.end())
        {
            // return error to client
            sendMessageToClient(clientSocket, "User is not a seeder of the file.");
            return;
        }

        // set isSharing to false
        group.files[fileName].seeders[clientSocketString].isSharing = false;

        // return success message to client
        sendMessageToClient(clientSocket, "Successfully stopped sharing.");
    }
    else if (innerCommand[0] == "upload_file")
    {
        // handle stop_sharing request

        if (numArgs != 4)
        {
            // return error to client
            sendMessageToClient(clientSocket, "Invalid number of arguments provided.");
            return;
        }

        // check if user is logged in
        if (clientUserMapping.find(clientSocket) == clientUserMapping.end())
        {
            // return error to client
            sendMessageToClient(clientSocket, "User is not logged in.");
            return;
        }

        string groupId = innerCommand[2];
        // check if group exists
        if (groupMapping.find(groupId) == groupMapping.end())
        {
            // return error to client
            sendMessageToClient(clientSocket, "Group does not exist.");
            return;
        }

        User &user = userMapping[clientUserMapping[clientSocket]];
        Group &group = groupMapping[groupId];

        // check if user is member of group
        if (group.members.find(user.userId) == group.members.end())
        {
            sendMessageToClient(clientSocket, "User is not member of the group.");
            return;
        }

        vector<string> decodedCommand = tokenizeInput(innerCommand[3], ';');
        string sockString = decodedCommand[0];
        int pieces = stoi(decodedCommand[1]);
        string hashSeq = decodedCommand[2];

        // check if file is shared
        // then add user to seeders list
        string fileName = innerCommand[1];
        if (group.files.find(fileName) != group.files.end())
        {
            // if the user is already a seeder, make isSharing true
            if (group.files[fileName].seeders.find(sockString) != group.files[fileName].seeders.end())
            {
                group.files[fileName].seeders[sockString].isSharing = true;
                sendMessageToClient(clientSocket, "You are now seeding the uploaded file.");
                return;
            }
            Seeder newSeeder = Seeder(sockString);
            group.files[fileName].seeders[sockString] = newSeeder;
            sendMessageToClient(clientSocket, "Successfully added to the seeders' list.");
            return;
        }

        Seeder newSeeder = Seeder(sockString);
        TorrentFile newFile = TorrentFile(fileName, pieces, hashSeq);
        newFile.seeders[sockString] = newSeeder;
        group.addFile(newFile);

        // return success message to client
        sendMessageToClient(clientSocket, "You are now seeding the uploaded file.");
    }
    else if (innerCommand[0] == "download_file")
    {
        // handle stop_sharing request

        if (numArgs != 3)
        {
            // return error to client
            sendMessageToClient(clientSocket, "Invalid number of arguments provided.|");
            return;
        }

        // check if user is logged in
        if (clientUserMapping.find(clientSocket) == clientUserMapping.end())
        {
            // return error to client
            sendMessageToClient(clientSocket, "User is not logged in.|");
            return;
        }

        string groupId = innerCommand[1];
        // check if group exists
        if (groupMapping.find(groupId) == groupMapping.end())
        {
            // return error to client
            sendMessageToClient(clientSocket, "Group does not exist.|");
            return;
        }

        User user = userMapping[clientUserMapping[clientSocket]];
        Group group = groupMapping[groupId];

        // check if user is member of group
        if (group.members.find(user.userId) == group.members.end())
        {
            sendMessageToClient(clientSocket, "User is not member of the group.|");
            return;
        }

        // check if file is shared
        string fileName = innerCommand[2];
        if (group.files.find(fileName) == group.files.end())
        {
            sendMessageToClient(clientSocket, "The specified file is not shared in the group.|");
            return;
        }

        // check if there are any seeders for the file
        bool isAtLeastOneSeeder = false;
        for (auto itr : group.files[fileName].seeders)
        {
            if (itr.second.isSharing)
            {
                isAtLeastOneSeeder = true;
                break;
            }
        }

        // send error to client if there are no seeders for the file
        if (!isAtLeastOneSeeder)
        {
            sendMessageToClient(clientSocket, "There are no seeders for the specified file.|");
            return;
        }

        // collect response and send to the client
        string response = "";
        TorrentFile file = group.files[fileName];
        response += to_string(file.numPieces);
        response += ";";
        response += file.hashSequence;
        response += ";";
        for (auto itr : file.seeders)
        {
            if (itr.second.isSharing)
                response += itr.first;
            response += ";";
        }
        response.pop_back();

        // send the information to the client
        cout << "RESPONSE: " << response << "\n";
        response += "|";
        int numPiecesTransfer = ceil(response.length() * 1.0 / ARGS_BUFFER_SIZE);
        for (int i = 0; i < numPiecesTransfer; i++)
        {
            string sendString = response.substr(i * ARGS_BUFFER_SIZE, ARGS_BUFFER_SIZE);
            send(clientSocket, sendString.c_str(), sendString.length(), 0);
        }
    }
    else if (innerCommand[0] == "quit")
        return;
    else
        sendMessageToClient(clientSocket, "Invalid request received.");
}
