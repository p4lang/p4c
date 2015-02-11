/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <behavioral_utils/behavioral_utils_config.h>

#include "behavioral_utils_log.h"

static int
datatypes_init__(void)
{
#define BEHAVIORAL_UTILS_ENUMERATION_ENTRY(_enum_name, _desc)     AIM_DATATYPE_MAP_REGISTER(_enum_name, _enum_name##_map, _desc,                               AIM_LOG_INTERNAL);
#include <behavioral_utils/behavioral_utils.x>
    return 0;
}

void __behavioral_utils_module_init__(void)
{
    AIM_LOG_STRUCT_REGISTER();
    datatypes_init__();
}

