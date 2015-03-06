/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <BMI/bmi_config.h>

#include <BMI/bmi_port.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <AIM/aim.h>

#include <unistd.h>

int aim_main(int argc, char* argv[])
{
    printf("BMI Utest\n");
    bmi_config_show(&aim_pvs_stdout);

    bmi_port_mgr_t *port_mgr;
    bmi_port_create_mgr(&port_mgr);

    bmi_port_interface_add(port_mgr, "lo", 1, NULL);

    sleep(100);

    bmi_port_interface_remove(port_mgr, 1);

    bmi_port_destroy_mgr(port_mgr);

    return 0;
}

