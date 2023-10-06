
#include "backends/p4tools/common/core/target.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <optional>
#include <string>
#include <utility>

#include "backends/p4tools/common/lib/variables.h"
#include "ir/irutils.h"

namespace P4Tools {

Target::Spec::Spec(std::string deviceName, std::string archName)
    : deviceName(std::move(deviceName)), archName(std::move(archName)) {
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
std::map<Target::Spec, std::map<std::string, const Target *>> Target::registry = {};
std::map<std::string, std::string> Target::defaultArchByDevice = {};
std::map<std::string, std::string> Target::defaultDeviceByArch = {};

bool Target::init(std::string deviceName, std::string archName) {
    Spec spec(std::move(deviceName), std::move(archName));

    if (registry.count(spec) != 0U) {
        curTarget = spec;
        return true;
    }

    return false;
}

bool Target::setDevice(std::string deviceName) {
    std::transform(deviceName.begin(), deviceName.end(), deviceName.begin(), ::tolower);
    if (defaultArchByDevice.count(deviceName) == 0U) {
        return false;
    }

    auto archName = curTarget ? curTarget->archName : defaultArchByDevice.at(deviceName);
    return init(deviceName, archName);
}

bool Target::setArch(std::string archName) {
    std::transform(archName.begin(), archName.end(), archName.begin(), ::tolower);
    if (defaultDeviceByArch.count(archName) == 0U) {
        return false;
    }

    auto deviceName = curTarget ? curTarget->deviceName : defaultDeviceByArch.at(archName);
    return init(deviceName, archName);
}

const IR::Expression *Target::createTargetUninitialized(const IR::Type *type,
                                                        bool forceTaint) const {
    if (forceTaint) {
        return ToolsVariables::getTaintExpression(type);
    }
    return IR::getDefaultValue(type);
}

Target::Target(std::string toolName, std::string deviceName, std::string archName)
    : toolName(toolName), spec(deviceName, archName) {
    // Register this instance.
    BUG_CHECK(!registry[spec].count(toolName), "Already registered %1%/%2% instance for %3%",
              deviceName, archName, toolName);
    registry[spec][toolName] = this;

    // Register default device and architecture, if needed.
    if (defaultDeviceByArch.count(spec.archName) == 0U) {
        defaultDeviceByArch[spec.archName] = spec.deviceName;
    }
    if (defaultArchByDevice.count(spec.deviceName) == 0U) {
        defaultArchByDevice[spec.deviceName] = spec.archName;
    }
}

}  // namespace P4Tools
