#include <cstdlib>
#include <exception>
#include <iostream>
#include <vector>

#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/modules/smith/smith.h"
#include "lib/crash.h"
#include "lib/exceptions.h"
#include "lib/timer.h"

int main(int argc, char **argv) {
    setup_signals();

    std::vector<const char *> args;
    args.reserve(argc);
    for (int i = 0; i < argc; ++i) {
        args.push_back(argv[i]);
    }

    int result = EXIT_SUCCESS;
    try {
        Util::ScopedTimer timer("P4Smith Main");
        result = P4Tools::P4Smith::Smith().main(args);
    } catch (const Util::CompilerBug &e) {
        std::cerr << "Internal error: " << e.what() << '\n';
        std::cerr << "Please submit a bug report with your code." << '\n';
        result = EXIT_FAILURE;
    } catch (const Util::CompilerUnimplemented &e) {
        std::cerr << e.what() << '\n';
        result = EXIT_FAILURE;
    } catch (const Util::CompilationError &e) {
        std::cerr << e.what() << '\n';
        result = EXIT_FAILURE;
    } catch (const std::exception &e) {
        std::cerr << "Internal error: " << e.what() << '\n';
        std::cerr << "Please submit a bug report with your code." << '\n';
        result = EXIT_FAILURE;
    } catch (...) {
        std::cerr << "Internal error. Please submit a bug report with your code." << '\n';
        result = EXIT_FAILURE;
    }
    P4Tools::printPerformanceReport();
    return result;
}
