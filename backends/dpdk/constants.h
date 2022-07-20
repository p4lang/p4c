/*
Copyright 2021 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef BACKENDS_DPDK_CONSTANTS_H_
#define BACKENDS_DPDK_CONSTANTS_H_

/* Unique handle for action and table */
const unsigned table_handle_prefix = 0x00010000;
const unsigned action_handle_prefix = 0x00020000;

// Default values
const unsigned dpdk_default_table_size = 65536;
#define DPDK_MAX_SHIFT_AMOUNT 64

// Maximum number of configurable timeout values
const unsigned dpdk_learner_max_configurable_timeout_values = 8;
const unsigned default_learner_table_size = 0x10000;
const unsigned default_learner_table_timeout = 120;
// Maximum values
const int dpdk_max_field_width = 64;

// JSON schema versions
const cstring bfrtSchemaVersion = "1.0.0";
const cstring tdiSchemaVersion = "0.1";

// HASH Values
#define JHASH0 0
#define CRC1 1
#define CRC2 2
#define CRC3 3
#define CRC4 4
#define JHASH5 5

#endif  /* BACKENDS_DPDK_CONSTANTS_H_ */
