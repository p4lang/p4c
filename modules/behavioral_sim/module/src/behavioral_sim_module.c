/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <behavioral_sim/behavioral_sim_config.h>

#include "behavioral_sim_log.h"

static int
datatypes_init__(void)
{
#define BEHAVIORAL_SIM_ENUMERATION_ENTRY(_enum_name, _desc)     AIM_DATATYPE_MAP_REGISTER(_enum_name, _enum_name##_map, _desc,                               AIM_LOG_INTERNAL);
#include <behavioral_sim/behavioral_sim.x>
    return 0;
}

void __behavioral_sim_module_init__(void)
{
    AIM_LOG_STRUCT_REGISTER();
    datatypes_init__();
}

