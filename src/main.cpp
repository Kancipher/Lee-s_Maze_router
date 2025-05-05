#include <iostream>
#include <string>
#include <fstream>
#include "functions.cpp"
using namespace std;

int main(int argc, char* argv[])
{
    if (argc < 2) {
        cerr << "Usage: mazerouter <input_file>\n";
        return 1;
    }

    string filename = argv[1];
    readfile(filename);
    // route();


    return 0;

}