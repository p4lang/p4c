#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_EXCEPTIONS_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_EXCEPTIONS_H_

#include <absl/strings/str_cat.h>

#include "lib/exceptions.h"

namespace P4Tools::P4Testgen {

/// This class indicates a feature that is not implemented in P4Testgen.
/// Paths with this unimplemented feature should be skipped
class TestgenUnimplemented final : public Util::P4CExceptionBase {
 public:
    template <typename... Args>
    explicit TestgenUnimplemented(const char *format, Args &&...args)
        : P4CExceptionBase(format, std::forward<Args>(args)...) {
        // Check if output is redirected and if so, then don't color text so that
        // escape characters are not present
        message = absl::StrCat(Util::cerr_colorize(Util::ANSI_BLUE), "Not yet implemented",
                               Util::cerr_clear_colors(), ":\n", message);
    }

    template <typename... Args>
    TestgenUnimplemented(int line, const char *file, const char *format, Args &&...args)
        : P4CExceptionBase(format, std::forward<Args>(args)...) {
        message = absl::StrCat(
            "In file: ", file, ":", line, "\n", Util::cerr_colorize(Util::ANSI_BLUE),
            "Unimplemented compiler support", Util::cerr_clear_colors(), ": ", message);
    }
};

#define TESTGEN_UNIMPLEMENTED(...)                                   \
    do {                                                             \
        throw TestgenUnimplemented(__LINE__, __FILE__, __VA_ARGS__); \
    } while (0)

}  // namespace P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_EXCEPTIONS_H_ */
