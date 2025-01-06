#pragma once

#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
using namespace std;

#include "clientFile.h"

void handleDownloadFromPeers(int trackerSocket, string userCommand, string mySocket);
