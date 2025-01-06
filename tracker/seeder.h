#pragma once

#include <string>
using namespace std;

class Seeder {
    public:
    string socketId;
    bool isSharing;
    Seeder();
    Seeder(string socket);
};