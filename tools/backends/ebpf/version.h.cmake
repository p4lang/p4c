/*
Copyright 2018-present Barefoot Networks, Inc.

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

#ifndef _BACKENDS_EBPF_VERSION_H
#define _BACKENDS_EBPF_VERSION_H

/**
  Set the compiler version at build time.
  The build system defines P4C_VERSION as a full string as well as the
  following components: P4C_VERSION_MAJOR, P4C_VERSION_MINOR,
  P4C_VERSION_PATCH, P4C_VERSION_RC, and P4C_GIT_SHA.

  They can be used to construct a version string as follows:
  #define VERSION_STRING "@P4C_VERSION@"
  or
  #define VERSION_STRING "@P4C_VERSION_MAJOR@.@P4C_VERSION_MINOR@.@P4C_VERSION_PATCH@@P4C_VERSION_RC@"

  Or, since this is backend specific, feel free to define other numbering
  scheme.

  */

#define P4C_EBPF_VERSION_STRING "@P4C_VERSION@"

#endif  // _BACKENDS_EBPF_VERSION_H
