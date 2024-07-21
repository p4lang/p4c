#ifndef BACKENDS_P4FMT_P4FMT_H_
#define BACKENDS_P4FMT_P4FMT_H_

#include <filesystem>
#include <sstream>

namespace p4c::P4Fmt {

/// Formats a P4 program from the input file, returns formatted output
std::stringstream getFormattedOutput(std::filesystem::path inputFile);

}  // namespace p4c::P4Fmt

#endif /* BACKENDS_P4FMT_P4FMT_H_ */
