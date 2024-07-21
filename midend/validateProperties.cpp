#include "validateProperties.h"

namespace p4c::P4 {

void ValidateTableProperties::postorder(const IR::Property *property) {
    if (legalProperties.find(property->name.name) != legalProperties.end()) return;
    warn(ErrorType::WARN_IGNORE_PROPERTY, "Unknown table property: %1%", property);
}

}  // namespace p4c::P4
