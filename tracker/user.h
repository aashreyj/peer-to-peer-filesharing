#pragma once

#include <string>
#include <unordered_set>
using namespace std;

class User
{
private:
    string passwd;

public:
    string userId;
    unordered_set<string> groups;
    User();
    User(string uid, string pass);
    bool loginAttempt(string uid, string pass);
};
