#include <string>
using namespace std;

#include "user.h"

User::User()
{
    userId = "";
    passwd = "";
}

User::User(string uid, string pass)
{
    userId = uid;
    passwd = pass;
}

bool User::loginAttempt(string uid, string pass)
{
    if (uid == userId && pass == passwd)
        return true;
    return false;
}
