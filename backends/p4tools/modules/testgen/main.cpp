#include <exception>
#include <iostream>
#include <vector>

#include "lib/crash.h"

#include "backends/p4tools/modules/testgen/testgen.h"

std::string updateErrorMsg(std::string errorMsg) {
    for (const std::string_view toReplace : {"Compiler", "compiler"}) {
        if (const auto pos = errorMsg.find(toReplace); pos != std::string::npos) {
            errorMsg.replace(pos, toReplace.size(), "P4Testgen");
            break;
        }
    }

    return errorMsg;
}

int main(int argc, char **argv) {
    setup_signals();

    std::vector<const char *> args;
    args.reserve(argc);
    for (int i = 0; i < argc; ++i) {
        args.push_back(argv[i]);
    }

    try {
        return P4Tools::P4Testgen::Testgen().main(args);
    } catch (const Util::CompilerBug &e) {
        std::cerr << "Internal error: " << updateErrorMsg(e.what()) << "\n";
        std::cerr << "Please submit a bug report with your code."
                  << "\n";
        return EXIT_FAILURE;
    } catch (const Util::CompilerUnimplemented &e) {
        std::cerr << updateErrorMsg(e.what()) << "\n";
        return EXIT_FAILURE;
    } catch (const Util::CompilationError &e) {
        std::cerr << updateErrorMsg(e.what()) << "\n";
        return EXIT_FAILURE;
    } catch (const std::exception &e) {
        std::cerr << "Internal error: " << updateErrorMsg(e.what()) << "\n";
        std::cerr << "Please submit a bug report with your code."
                  << "\n";
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "Internal error. Please submit a bug report with your code."
                  << "\n";
        return EXIT_FAILURE;
    }
}
