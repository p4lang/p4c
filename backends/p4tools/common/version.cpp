#include "backends/p4tools/common/version.h"

#include <iostream>

void printVersion(const char *binaryName) {
    std::cout << binaryName << std::endl
              << "Version"
              << " " << P4TOOLS_VERSION_MAJOR << "." << P4TOOLS_VERSION_MINOR << "."
              << P4TOOLS_VERSION_PATCH << P4TOOLS_VERSION_HASH << std::endl;
}
