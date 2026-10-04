#ifndef BACKENDS_P4FMT_P4FMT_H_
#define BACKENDS_P4FMT_P4FMT_H_

#include <filesystem>
#include <optional>
#include <string>

namespace P4::P4Fmt {

/// Formats original source without preprocessing. Unsupported input is returned
/// unchanged unless failOnFallback is set. In that mode, report why formatting
/// is unsupported and return nullopt. I/O failures also return nullopt.
/// Empty and already-formatted files are valid results in either mode.
std::optional<std::string> getFormattedOutput(const std::filesystem::path &inputFile,
                                              bool failOnFallback = false);

}  // namespace P4::P4Fmt

#endif /* BACKENDS_P4FMT_P4FMT_H_ */
