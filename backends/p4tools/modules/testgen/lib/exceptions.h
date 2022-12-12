#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_EXCEPTIONS_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_EXCEPTIONS_H_

#include "lib/exceptions.h"

namespace P4Tools {

namespace P4Testgen {

/// This class indicates a feature that is not implemented in P4Testgen.
/// Paths with this unimplemented feature should be skipped
class TestgenUnimplemented final : public Util::P4CExceptionBase {
 public:
    template <typename... T>
    explicit TestgenUnimplemented(const char *format, T... args)
        : P4CExceptionBase(format, args...) {
        // Check if output is redirected and if so, then don't color text so that
        // escape characters are not present
        message = cstring(Util::cerr_colorize(Util::ANSI_BLUE)) + "Not yet implemented" +
                  Util::cerr_clear_colors() + ":\n" + message;
    }

    template <typename... T>
    TestgenUnimplemented(int line, const char *file, const char *format, T... args)
        : P4CExceptionBase(format, args...) {
        message = cstring("In file: ") + file + ":" + Util::toString(line) + "\n" +
                  Util::cerr_colorize(Util::ANSI_BLUE) + "Unimplemented compiler support" +
                  Util::cerr_clear_colors() + ": " + message;
    }
};

template <class... Args>
[[noreturn]] inline auto TESTGEN_UNIMPLEMENTED(Args&&... args) {
    throw TestgenUnimplemented(__LINE__, __FILE__, std::forward<Args>(args)...);
}

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_EXCEPTIONS_H_ */
