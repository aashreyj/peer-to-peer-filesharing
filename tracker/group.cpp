#include <string>
using namespace std;

#include "group.h"
#include "user.h"

Group::Group() {
    groupId = "";
    ownerId = "";
}

Group::Group(string gid, string owner)
{
    groupId = gid;
    ownerId = owner;
    members.insert(owner);
}

void Group::join(User user)
{
    joiningRequests.insert(user.userId);
}

void Group::leave(User &user)
{
    if (ownerId == user.userId)
    {
        // choose new owner from list of members
        int randomIndex = rand() % members.size();
        auto itr = members.begin();
        for (int i = 0; i < randomIndex; i++)
            itr++;
        ownerId = *itr;
    }
    members.erase(user.userId);
    user.groups.erase(groupId);
}

unordered_set<string> Group::listJoiningRequests()
{
    return joiningRequests;
}

void Group::acceptJoiningRequest(User &user)
{
    joiningRequests.erase(user.userId);
    members.insert(user.userId);
    user.groups.insert(groupId);
}

void Group::addFile(TorrentFile file) {
    files[file.path] = file;
}
