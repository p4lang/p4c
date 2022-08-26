#include "extensions/testgen/common/version.h"

#include <iostream>

void printVersion(const char* binaryName) {
    std::cout << binaryName << std::endl
              << "Version"
              << " " << BF_P4TOOLS_VERSION_MAJOR << "." << BF_P4TOOLS_VERSION_MINOR << "."
              << BF_P4TOOLS_VERSION_PATCH << BF_P4TOOLS_VERSION_HASH << std::endl;
}
