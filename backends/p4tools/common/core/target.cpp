
#include "backends/p4tools/common/core/target.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <optional>
#include <string>

#include "backends/p4tools/common/lib/variables.h"
#include "ir/irutils.h"

namespace P4::P4Tools {

Target::Spec::Spec(std::string_view deviceName, std::string_view archName)
    : deviceName(deviceName), archName(archName) {
    // Convert names to lower case.
    std::transform(this->archName.begin(), this->archName.end(), this->archName.begin(), ::tolower);
    std::transform(this->deviceName.begin(), this->deviceName.end(), this->deviceName.begin(),
                   ::tolower);
}

bool Target::Spec::operator<(const Spec &other) const {
    if (deviceName != other.deviceName) {
        return deviceName < other.deviceName;
    }
    return archName < other.archName;
}

/* =============================================================================================
 *  Target implementation
 * ============================================================================================= */

std::optional<Target::Spec> Target::curTarget = std::nullopt;
std::map<Target::Spec, std::map<std::string, const Target *, std::less<>>> Target::registry = {};
std::map<std::string, std::string, std::less<>> Target::defaultArchByDevice = {};
std::map<std::string, std::string, std::less<>> Target::defaultDeviceByArch = {};

bool Target::init(std::string_view deviceName, std::string_view archName) {
    Spec spec(deviceName, archName);

    if (registry.count(spec) != 0U) {
        curTarget = spec;
        return true;
    }

    return false;
}

std::optional<ICompileContext *> Target::initializeTarget(std::string_view toolName,
                                                          std::string_view target,
                                                          std::string_view arch) {
    // Establish a dummy compilation context so that we can use ::error to report errors while
    // processing target and arch.
    class DummyCompileContext : public BaseCompileContext {
    } dummyContext;
    AutoCompileContext autoDummyContext(&dummyContext);
    if (!P4Tools::Target::setDevice(target)) {
        ::P4::error("Unsupported device: %s", target);
        return std::nullopt;
    }
    if (!P4Tools::Target::setArch(arch)) {
        ::P4::error("Unsupported architecture: %s", arch);
        return std::nullopt;
    }
    const auto &instances = registry.at(*curTarget);
    auto instance = instances.find(toolName);
    BUG_CHECK(instance != instances.end(), "Architecture %1% on device %2% not supported for %3%",
              curTarget->archName, curTarget->deviceName, toolName);

    return instance->second->makeContext();
}

std::optional<ICompileContext *> Target::initializeTarget(std::string_view toolName,
                                                          const std::vector<const char *> &args) {
    // Establish a dummy compilation context so that we can use ::error to report errors while
    // processing target and arch.
    class DummyCompileContext : public BaseCompileContext {
    } dummyContext;
    AutoCompileContext autoDummyContext(&dummyContext);
    if (args.size() < 3) {
        ::P4::error("Missing --target and --arch arguments");
        return std::nullopt;
    }
    std::optional<std::string> target;
    std::optional<std::string> arch;
    // Loop through arguments (skip the program name)
    for (size_t i = 1; i < args.size(); ++i) {
        const std::string &arg = args[i];
        if (arg == "--arch") {
            if (i + 1 < args.size()) {
                arch = args[i + 1];
            } else {
                ::P4::error("Missing architecture name after --arch");
                return std::nullopt;
            }
        }
        if (arg == "--target") {
            if (i + 1 < args.size()) {
                target = args[i + 1];
            } else {
                ::P4::error("Missing device name after --target");
                return std::nullopt;
            }
        }
    }
    if (!target) {
        ::P4::error("Missing --target argument");
        return std::nullopt;
    }
    if (!arch) {
        ::P4::error("Missing --arch argument");
        return std::nullopt;
    }
    return initializeTarget(toolName, target.value(), arch.value());
}

bool Target::setDevice(std::string_view deviceName) {
    std::string lowerCaseDeviceName(deviceName);
    std::transform(lowerCaseDeviceName.begin(), lowerCaseDeviceName.end(),
                   lowerCaseDeviceName.begin(), ::tolower);
    if (defaultArchByDevice.count(lowerCaseDeviceName) == 0U) {
        return false;
    }

    auto archName = curTarget ? curTarget->archName : defaultArchByDevice.at(deviceName.data());
    return init(deviceName, archName);
}

bool Target::setArch(std::string_view archName) {
    std::string lowerCaseArchName(archName);
    std::transform(lowerCaseArchName.begin(), lowerCaseArchName.end(), lowerCaseArchName.begin(),
                   ::tolower);
    if (defaultDeviceByArch.count(lowerCaseArchName) == 0U) {
        return false;
    }

    auto deviceName = curTarget ? curTarget->deviceName : defaultDeviceByArch.at(archName.data());
    return init(deviceName, archName);
}

const IR::Expression *Target::createTargetUninitialized(const IR::Type *type,
                                                        bool forceTaint) const {
    if (forceTaint) {
        return ToolsVariables::getTaintExpression(type);
    }
    return IR::getDefaultValue(type);
}

Target::Target(std::string_view toolName, const std::string &deviceName,
               const std::string &archName)
    : toolName(toolName), spec(deviceName, archName) {
    // Register this instance.
    BUG_CHECK(!registry[spec].count(toolName), "Already registered %1%/%2% instance for %3%",
              deviceName, archName, toolName);
    registry[spec][toolName.data()] = this;

    // Register default device and architecture, if needed.
    if (defaultDeviceByArch.count(spec.archName) == 0U) {
        defaultDeviceByArch[spec.archName] = spec.deviceName;
    }
    if (defaultArchByDevice.count(spec.deviceName) == 0U) {
        defaultArchByDevice[spec.deviceName] = spec.archName;
    }
}

}  // namespace P4::P4Tools
