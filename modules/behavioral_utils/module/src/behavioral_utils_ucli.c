/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <behavioral_utils/behavioral_utils_config.h>

#if BEHAVIORAL_UTILS_CONFIG_INCLUDE_UCLI == 1

#include <uCli/ucli.h>
#include <uCli/ucli_argparse.h>
#include <uCli/ucli_handler_macros.h>

static ucli_status_t
behavioral_utils_ucli_ucli__config__(ucli_context_t* uc)
{
    UCLI_HANDLER_MACRO_MODULE_CONFIG(behavioral_utils)
}

/* <auto.ucli.handlers.start> */
/* <auto.ucli.handlers.end> */

static ucli_module_t
behavioral_utils_ucli_module__ =
    {
        "behavioral_utils_ucli",
        NULL,
        behavioral_utils_ucli_ucli_handlers__,
        NULL,
        NULL,
    };

ucli_node_t*
behavioral_utils_ucli_node_create(void)
{
    ucli_node_t* n;
    ucli_module_init(&behavioral_utils_ucli_module__);
    n = ucli_node_create("behavioral_utils", NULL, &behavioral_utils_ucli_module__);
    ucli_node_subnode_add(n, ucli_module_log_node_create("behavioral_utils"));
    return n;
}

#else
void*
behavioral_utils_ucli_node_create(void)
{
    return NULL;
}
#endif

