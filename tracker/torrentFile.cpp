#include <string>
using namespace std;

#include "torrentFile.h"

TorrentFile::TorrentFile(string filePath, int pieces, string hashSeq)
{
    path = filePath;
    numPieces = pieces;
    hashSequence = hashSeq;
}
