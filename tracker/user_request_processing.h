#pragma once

#include <unordered_map>
#include <string>
using namespace std;

class Group;
class User;

void processClientRequest(int clientSocket, string command, unordered_map<int, string> &clientUserMapping, unordered_map<string, User> &userMapping, unordered_map<string, Group> &groupMapping);