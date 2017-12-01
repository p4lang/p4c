/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "options.h"

void Util::Options::registerOption(const char* option, const char* argName,
                                   OptionProcessor processor, const char* description,
                                   OptionFlags flags /* = OptionFlags::Default */) {
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
    o->flags = flags;
    auto opt = get(options, option);
    if (opt != nullptr)
        throw std::logic_error(std::string("Option already registered: ") + option);
    options.emplace(option, o);
    optionOrder.push_back(option);
}

// Process options; return list of remaining options.
// Returns 'nullptr' if an error is signalled
std::vector<const char*>* Util::Options::process(int argc, char* const argv[]) {
    if (argc == 0 || argv == nullptr)
        throw std::logic_error("No arguments to process");
    binaryName = argv[0];
    for (int i=1; i < argc; i++) {
        cstring opt = argv[i];
        const char* arg = nullptr;
        const Option* option = nullptr;

        if (opt.startsWith("--")) {
            option = get(options, opt);
            if (!option && (arg = opt.find('=')))
                option = get(options, opt.before(arg++));
            if (option == nullptr) {
                ::error("Unknown option %1%", opt);
                usage();
                return nullptr; }
        } else if (opt.startsWith("-")) {
            if (opt.size() > 2) {
                arg = opt.substr(2);
                opt = opt.substr(0, 2); }
            option = get(options, opt);
            if (option == nullptr) {
                ::error("Unknown option %1%", opt);
                usage();
                return nullptr; }
            if ((option->flags & OptionFlags::OptionalArgument) &&
                    (!arg || strlen(arg) == 0))
                arg = nullptr; }

        if (option == nullptr) {
            remainingOptions.push_back(opt);
        } else {
            if (option->argName != nullptr && arg == nullptr &&
                    !(option->flags & OptionFlags::OptionalArgument)) {
                if (i == argc - 1) {
                    ::error("Option %1% is missing required argument %2%",
                            opt, option->argName);
                    usage();
                    return nullptr; }
                arg = argv[++i]; }
            bool success = option->processor(arg);
            if (!success) {
                usage();
                return nullptr; } } }

    return &remainingOptions;
}

void Util::Options::usage() {
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
        if (option->flags & OptionFlags::Hide)
            continue;
        *outStream << option->option;
        if (option->argName != nullptr) {
            if (option->flags & OptionFlags::OptionalArgument) {
                *outStream << "[=" << option->argName << "]";
                len += 3 + strlen(option->argName);
            } else {
                *outStream << " " << option->argName;
                len += 1 + strlen(option->argName);
            }
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
