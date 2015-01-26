/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <behavioral_sim/behavioral_sim_config.h>

#if BEHAVIORAL_SIM_CONFIG_INCLUDE_UCLI == 1

#include <uCli/ucli.h>
#include <uCli/ucli_argparse.h>
#include <uCli/ucli_handler_macros.h>

static ucli_status_t
behavioral_sim_ucli_ucli__config__(ucli_context_t* uc)
{
    UCLI_HANDLER_MACRO_MODULE_CONFIG(behavioral_sim)
}

/* <auto.ucli.handlers.start> */
/* <auto.ucli.handlers.end> */

static ucli_module_t
behavioral_sim_ucli_module__ =
    {
        "behavioral_sim_ucli",
        NULL,
        behavioral_sim_ucli_ucli_handlers__,
        NULL,
        NULL,
    };

ucli_node_t*
behavioral_sim_ucli_node_create(void)
{
    ucli_node_t* n;
    ucli_module_init(&behavioral_sim_ucli_module__);
    n = ucli_node_create("behavioral_sim", NULL, &behavioral_sim_ucli_module__);
    ucli_node_subnode_add(n, ucli_module_log_node_create("behavioral_sim"));
    return n;
}

#else
void*
behavioral_sim_ucli_node_create(void)
{
    return NULL;
}
#endif

