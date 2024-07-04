#ifndef P4FMT_H
#define P4FMT_H

#include <filesystem>
#include <sstream>

std::stringstream getFormattedOutput(std::filesystem::path inputFile);

#endif  // P4FMT_H
