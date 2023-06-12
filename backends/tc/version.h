/*
Copyright (C) 2023 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions
and limitations under the License.
*/

#ifndef BACKENDS_TC_VERSION_H_
#define BACKENDS_TC_VERSION_H_

#include <string>

/* Start version defines
** The build process will define these macros
** so we can have the same version number as the build process */

#ifndef MAJORVERSION
#define MAJORVERSION 1
#endif

#ifndef MINORVERSION
#define MINORVERSION 0
#endif

#ifndef BUILDVERSION
#define BUILDVERSION 0
#endif

#ifndef FIXVERSION
#define FIXVERSION 0
#endif
/* end version defines*/

std::string version_string();
unsigned int version_num();

#endif /* BACKENDS_TC_VERSION_H_ */
