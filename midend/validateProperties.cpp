#include "validateProperties.h"

namespace P4 {

void ValidateTableProperties::postorder(const IR::TableProperty* property) {
    if (legalProperties.find(property->name.name) != legalProperties.end())
        return;
    ::warning("Unknown table property: %1%", property);
}

}  // namespace P4
