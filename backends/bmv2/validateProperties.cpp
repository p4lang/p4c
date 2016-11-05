#include "validateProperties.h"

namespace BMV2 {

void ValidateTableProperties::postorder(const IR::TableProperty* property) {
    static cstring knownProperties[] = {
        "actions",
        "default_action",
        "key",
        "implementation",
        "size"
    };
    for (size_t i=0; i < ELEMENTS(knownProperties); i++)
        if (property->name.name == knownProperties[i])
            return;
    ::warning("Unknown table property: %1%", property);
}

}  // namespace BMV2
