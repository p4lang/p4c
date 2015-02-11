/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <behavioral_utils/behavioral_utils_config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <AIM/aim.h>

int aim_main(int argc, char* argv[])
{
    printf("behavioral_utils Utest Is Empty\n");
    behavioral_utils_config_show(&aim_pvs_stdout);
    return 0;
}

