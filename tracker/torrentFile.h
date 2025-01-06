#pragma once

#include <string>
#include <unordered_map>
using namespace std;

#include "seeder.h"

class TorrentFile {
    public:
    string path;
    int numPieces;
    unordered_map<string, Seeder> seeders;
    string hashSequence;
    TorrentFile() noexcept = default;
    TorrentFile(string filePath, int pieces, string hashSeq);
};
