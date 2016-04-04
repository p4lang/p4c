#ifndef _LIB_OPTIONS_H_
#define _LIB_OPTIONS_H_

#include <functional>
#include <stdexcept>
#include <string>
#include <vector>
#include <ostream>

#include "cstring.h"
#include "map.h"
#include "error.h"

namespace Util {

// Command-line options processing
class Options {
 public:
    // return true if processing is successful
    typedef std::function<bool(const char* optarg)> OptionProcessor;

 protected:
    struct Option {
        cstring option;
        const char* argName;  // nullptr if argument is not required
        const char* description;
        OptionProcessor processor;
    };
    const char* binaryName;
    cstring message;
    std::ostream* outStream = &std::cerr;

    std::map<cstring, const Option*> options;
    std::vector<cstring> optionOrder;
    std::vector<const char*> additionalUsage;
    std::vector<const char*> remainingOptions;  // produced as output
    // if true unknown options are collected in remainingOptions
    bool collectUnknownOptions = false;

    void setOutStream(std::ostream* out) { outStream = out; }
    void registerUsage(const char* msg) { additionalUsage.push_back(msg); }
    void registerOption(const char* option,   // option to register, e.g., -c or --version
                        const char* argName,  // name of option argument;
                                              // nullptr if no argument expected
                        OptionProcessor processor,  // function to execute when option matches
                        const char* description) {  // option help message
        if (option == nullptr || processor == nullptr || description == nullptr)
            throw std::logic_error("Null argument to registerOption");
        if (strlen(option) <= 1)
            throw std::logic_error(std::string("Option too short: ") + option);
        if (option[0] != '-')
            throw std::logic_error(std::string("Expected option to start with -: ") + option);
        auto o = new Option();
        o->option = option;
        o->argName = argName;
        o->processor = processor;
        o->description = description;
        auto opt = get(options, option);
        if (opt != nullptr)
            throw std::logic_error(std::string("Option already registered: ") + option);
        options.emplace(option, o);
        optionOrder.push_back(option);
    }

    explicit Options(cstring message) : message(message) {}

 public:
    // Process options; return list of remaining options.
    // Returns 'nullptr' if an error is signalled
    std::vector<const char*>* process(int argc, char* const argv[]) {
        if (argc == 0 || argv == nullptr)
            throw std::logic_error("No arguments to process");
        binaryName = argv[0];
        for (int i=1; i < argc; i++) {
            cstring opt = argv[i];
            const char* arg = nullptr;
            const Option* option = nullptr;

            if (opt.startsWith("--")) {
                option = get(options, opt);
                if (option == nullptr) {
                    ::error("Unknown option %1%", opt);
                    usage();
                    return nullptr;
                }
            } else if (opt.startsWith("-")) {
                if (opt.size() > 2) {
                    arg = opt.substr(2);
                    opt = opt.substr(0, 2);
                }
                option = get(options, opt);
                if (option == nullptr) {
                    ::error("Unknown option %1%", opt);
                    usage();
                    return nullptr;
                }
            }

            if (option == nullptr) {
                remainingOptions.push_back(opt);
            } else {
                if (option->argName != nullptr && arg == nullptr) {
                    if (i == argc - 1)
                        throw std::logic_error(std::string("Option ") + opt +
                                               " is missing required argument " + option->argName);
                    arg = argv[++i];
                }
                bool success = option->processor(arg);
                if (!success)
                    break;
            }
        }
        return &remainingOptions;
    }

    void usage() {
        *outStream << binaryName << ": " << message << std::endl;

        size_t labelLen = 0;
        for (auto o : optionOrder) {
            size_t len = o.size();
            auto option = get(options, o);
            if (option->argName != nullptr)
                len += 1 + strlen(option->argName);
            if (labelLen < len)
                labelLen = len;
        }

        labelLen += 3;
        for (auto o : optionOrder) {
            auto option = get(options, o);
            size_t len = strlen(o);
            *outStream << option->option;
            if (option->argName != nullptr) {
                *outStream << " " << option->argName;
                len += 1 + strlen(option->argName);
            }
            std::string line;
            std::stringstream ss(option->description);
            while (std::getline(ss, line)) {
                *outStream << std::string(labelLen - len, ' ');
                *outStream << line << std::endl;
                len = 0;
            }
        }

        for (auto m : additionalUsage)
            *outStream << m << std::endl;
    }
};

}  // namespace Util

#endif /* _LIB_OPTIONS_H_ */
