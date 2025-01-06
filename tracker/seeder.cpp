#include <string>
using namespace std;

#include "seeder.h"

Seeder::Seeder() {
    socketId = "";
    isSharing = false;
}

Seeder::Seeder(string socket) {
    socketId = socket;
    isSharing = true;
}
