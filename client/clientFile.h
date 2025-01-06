#pragma once

#include <string>
#include <vector>
using namespace std;

class ClientFile {
    public:
    string path;
    int numPieces;
    char fileStatus;
    string groupName;
    ClientFile() noexcept = default;
    ClientFile(string filePath, string group);
};
