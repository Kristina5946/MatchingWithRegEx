#include "pch.h"
#include "RegExTree.h"
#include "MatchLogic.h"
#include "Error.h"
#include <fstream>
#include <iostream>
#include <unordered_set>
#include <vector>
#include <string>

namespace {

    const size_t NFA_STATE_LIMIT = 10000;

    bool readTextFile(const std::string& filepath, std::string& content)
    {
        std::ifstream input(filepath.c_str());
        if (!input.is_open()) {
            return false;
        }

        content.clear();
        std::string line;
        bool firstLine = true;
        bool hasMoreLines = static_cast<bool>(std::getline(input, line));
        while (hasMoreLines) {
            if (!firstLine) {
                content.push_back('\n');
            }
            content += line;
            firstLine = false;
            hasMoreLines = static_cast<bool>(std::getline(input, line));
        }
        return true;
    }

   

int main(int argc, char* argv[])
{
    // 1: Проверить количество аргументов командной строки
    if (argc < 4) {
        std::cerr << "Usage: MatchingWithRegEx <regex.txt> <string.txt> <report.html>" << std::endl;
        return 1;
    }

    std::string regexFilePath = argv[1];
    std::string inputFilePath = argv[2];
    std::string outputFilePath = argv[3];

    // 2: Считать регулярное выражение из файла шаблона
    std::string regexTemplate;
    if (!readTextFile(regexFilePath, regexTemplate)) {
        std::cerr << "Cannot read regex template file." << std::endl;
        return 1;
    }

    
    return 0;
}
