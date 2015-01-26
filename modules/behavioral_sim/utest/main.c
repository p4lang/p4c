/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <behavioral_sim/behavioral_sim_config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <AIM/aim.h>

int aim_main(int argc, char* argv[])
{
    printf("behavioral_sim Utest Is Empty\n");
    behavioral_sim_config_show(&aim_pvs_stdout);
    return 0;
}

