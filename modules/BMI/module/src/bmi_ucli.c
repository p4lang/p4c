/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <BMI/bmi_config.h>

#if BMI_CONFIG_INCLUDE_UCLI == 1

#include <uCli/ucli.h>
#include <uCli/ucli_argparse.h>
#include <uCli/ucli_handler_macros.h>

static ucli_status_t
bmi_ucli_ucli__config__(ucli_context_t* uc)
{
    UCLI_HANDLER_MACRO_MODULE_CONFIG(bmi)
}

/* <auto.ucli.handlers.start> */
/* <auto.ucli.handlers.end> */

static ucli_module_t
bmi_ucli_module__ =
    {
        "bmi_ucli",
        NULL,
        bmi_ucli_ucli_handlers__,
        NULL,
        NULL,
    };

ucli_node_t*
bmi_ucli_node_create(void)
{
    ucli_node_t* n;
    ucli_module_init(&bmi_ucli_module__);
    n = ucli_node_create("bmi", NULL, &bmi_ucli_module__);
    ucli_node_subnode_add(n, ucli_module_log_node_create("bmi"));
    return n;
}

#else
void*
bmi_ucli_node_create(void)
{
    return NULL;
}
#endif

