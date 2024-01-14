#ifndef DEFERREDATTRIBUTEINTERPOLATIONSHADING_ARGPARSE
#define DEFERREDATTRIBUTEINTERPOLATIONSHADING_ARGPARSE

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

struct Args
{
    std::map<std::string, std::string> keyValueArgs;
    std::vector<std::string> positionalArgs;
};

Args argparse(int argc, char** argv) {
    Args args;
    for (int i = 1; i < argc; i += 2) {
        std::string arg = argv[i];
        if (arg[0] == '-') {
            if (arg[1] != '-') {
                throw std::runtime_error("Short arguments not supported");
            }
            auto& value
              = (args.keyValueArgs[std::string{arg.begin() + 2, arg.end()}]
                 = argv[++i]);
            if (value.starts_with('-')) {
                throw std::runtime_error("Argument " + arg
                                         + " requires a value");
            }
        } else {
            args.positionalArgs.push_back(arg);
        }
    }
    return args;
}

#endif /* DEFERREDATTRIBUTEINTERPOLATIONSHADING_ARGPARSE */
