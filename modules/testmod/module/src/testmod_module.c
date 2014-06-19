/****************************************************************
 * 
 ***************************************************************/

#include <testmod/testmod_config.h>
#include "testmod_log.h"

int
__testmod_module_init__(void)
{
    AIM_LOG_STRUCT_REGISTER(); 
    return 0; 
}

