#ifndef TUPLE
#define TUPLE

#include <string>
#include <fstream>
#include <vector>
#include <unordered_map>
class tuple {
    public:
    std::vector<int> offsets;
    std::unordered_map<std::string, std::pair<int, int>> names; // offset, size
    int totalSize = 0;
    std::unordered_map<std::string, int> typeSizes;
    tuple (std::string filename) {
        typeSizes = {{"int", 4}, {"string", 64}, {"bool", 1}}; 
        std::ifstream stream (filename);
        if (!stream.is_open()) {
            std::cerr << "Failed to open the file." << std::endl;
            return;
        }
        std::string fieldName, fieldType;
        while (stream >> fieldName >> fieldType) {
            if (!typeSizes.count(fieldType)) {
                std::cout << "unknown field type" << std::endl;
                continue;
            }
            names[fieldName] = {totalSize, typeSizes[fieldType]};
            offsets.push_back(totalSize);   
            totalSize += typeSizes[fieldType];
        }
        //std::reverse(offsets.begin() + 1, offsets.end());
    }

    int getFieldSize(std::string fieldName) {

    }
};

#endif