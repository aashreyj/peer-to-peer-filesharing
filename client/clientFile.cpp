#include <math.h>
#include <string>
#include <sys/stat.h>
using namespace std;

#include "clientFile.h"
#include "../tracker/constants.h"

ClientFile::ClientFile(string filePath, string group)
{
    path = filePath;
    groupName = group;
    fileStatus = 'L';

    // calculate number of pieces using ceil(fileSize / pieceSize)
    struct stat buffer;
    stat(filePath.c_str(), &buffer);
    numPieces = int(ceil((buffer.st_size * 1.0) / FILE_PIECE_SIZE));
}
