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

#endif  /* BACKENDS_DPDK_CONSTANTS_H_ */
