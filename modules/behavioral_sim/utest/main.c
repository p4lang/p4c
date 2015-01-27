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

// implemented in utests.cc
int run_all_gtests(int argc, char **argv);

int aim_main(int argc, char* argv[])
{
    printf("config dump\n");
    behavioral_sim_config_show(&aim_pvs_stdout);

    run_all_gtests(argc, argv);

    return 0;
}

