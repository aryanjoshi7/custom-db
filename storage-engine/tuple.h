#ifndef TUPLE
#define TUPLE

#include <string>
#include <fstream>
#include <vector>

class tuple {
    public:
    std::vector<int> offsets;
    int totalSize = 0;
    tuple (std::string filename) {
        std::string line;
        std::ifstream stream (filename);
        if (!stream.is_open()) {
            std::cerr << "Failed to open the file." << std::endl;
            return;
        }
        while (getline(stream, line)) {
            if (line == "int") {
                offsets.push_back(4);
                totalSize += 4;
            } else if (line == "string") {
                offsets.push_back(64);
                totalSize += 64;
            } else if (line == "bool") {
                offsets.push_back(1);
                totalSize += 1;

            }
        }
        std::reverse(offsets.begin() + 1, offsets.end());
    }
};

#endif