#include <sstream>
#include <string>
#include <vector>
using namespace std;

#include "constants.h"
#include "tokenize.h"

// tokenize the input on the basis of delimiter
vector<string> tokenizeInput(string input, char delimiter)
{
    vector<string> result;
    string token;
    istringstream tokenStream(input);
    while (getline(tokenStream, token, delimiter))
        result.push_back(token);
    return result;
}
