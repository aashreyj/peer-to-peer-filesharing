#pragma once

#include <string>
#include <unordered_map>
#include <vector>
using namespace std;

#include "clientFile.h"

void handlePeerDownloadRequest(int peerSocket, string peerCommand);
void handleUploadToTracker(int trackerSocket, vector<string> userCommand, string socketString);
