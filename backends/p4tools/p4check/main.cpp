#include <exception>
#include <iostream>
#include <utility>

#include <boost/optional/optional.hpp>

#include "backends/p4tools/mutate/mutate.h"
#include "backends/p4tools/p4check/commands.h"
#include "backends/p4tools/smith/smith.h"
#include "lib/crash.h"
#include "lib/exceptions.h"
#include "lib/gc.h"

#include "backends/p4tools/testgen/testgen.h"
#include "extensions/testgen/common/version.h"

int main(int argc, char** argv) {
    setup_gc_logging();
    setup_signals();

    // Catch all exceptions here.
    try {
        // Process command-line options to find the command being requested and hand off to that
        // command.
        if (const auto pair = P4Tools::P4CheckCommands::get().process(argc, argv)) {
            const auto& command = pair->first;
            const auto& args = pair->second;

            switch (command) {
                case P4Tools::HELP:
                    P4Tools::P4CheckCommands::get().usage(argv[0]);
                    return 0;

                case P4Tools::VERSION:
                    printVersion(argv[0]);
                    return 0;

                case P4Tools::TESTGEN:
                    return P4Tools::P4Testgen::Testgen().main(args);

                case P4Tools::VERIFY:
                    std::cerr << "Verify is not implemented yet." << std::endl;
                    return 1;

                case P4Tools::MUTATE:
                    return P4Tools::P4Mutate::Mutate().main(args);

                case P4Tools::SMITH:
                    return P4Tools::P4Smith::Smith().main(args);

                default:
                    std::cerr << "Unknown or unimplemented command: \"" << command << "\"."
                              << std::endl;
                    return 1;
            }
        } else {
            // Processing failed.
            return 1;
        }
    } catch (const Util::CompilerBug& e) {
        std::cerr << "Internal error: " << e.what() << std::endl;
        std::cerr << "Please submit a bug report with your code." << std::endl;
        return 1;
    } catch (const Util::CompilerUnimplemented& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    } catch (const Util::CompilationError& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Internal error: " << e.what() << std::endl;
        std::cerr << "Please submit a bug report with your code." << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Internal error. Please submit a bug report with your code." << std::endl;
        return 1;
    }
}
