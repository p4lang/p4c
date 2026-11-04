/*
 * SPDX-FileCopyrightText: 2026 Mojatatu Networks
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <assert.h>
#include <string.h>
#include "case_parser.h"
#include "register_defaults.h"

static struct p4tc_ext_bpf_val values[REGISTER_INSTANCES + 1];
static int initialized;

static void initialize(void)
{
    if (initialized)
        return;
    for (unsigned i = 1; i <= REGISTER_INSTANCES; i++)
        memcpy(values[i].out_params, register_defaults[i], sizeof(register_defaults[i]));
    initialized = 1;
}

struct p4tc_ext_bpf_val *bpf_p4tc_extern_md_read(struct __sk_buff *skb,
                                              struct p4tc_ext_bpf_params *params,
                                              const u32 size)
{
    assert(size == sizeof(*params));
    assert(params->index == 1);
    assert(params->inst_id > 0 && params->inst_id <= REGISTER_INSTANCES);
    initialize();
    return &values[params->inst_id];
}

int bpf_p4tc_extern_md_write(struct __sk_buff *skb, struct p4tc_ext_bpf_params *params,
                           const u32 size, struct p4tc_ext_bpf_val *value,
                           const u32 value_size)
{
    assert(size == sizeof(*params));
    assert(value_size == sizeof(*value));
    assert(params->index == 1);
    assert(params->inst_id > 0 && params->inst_id <= REGISTER_INSTANCES);
    initialize();
    values[params->inst_id] = *value;
    return 0;
}
