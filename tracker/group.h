#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
using namespace std;

#include "torrentFile.h"

class User;

class Group
{
public:
    string groupId;
    string ownerId;
    unordered_set<string> members;
    unordered_set<string> joiningRequests;
    unordered_map<string, TorrentFile> files;
    Group();
    Group(string gid, string owner);
    void join(User user);
    void leave(User &user);
    unordered_set<string> listJoiningRequests();
    void acceptJoiningRequest(User &user);
    void addFile(TorrentFile file);
};
