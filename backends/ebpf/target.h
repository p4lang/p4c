/*
Copyright 2013-present Barefoot Networks, Inc.

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

#ifndef _BACKENDS_EBPF_TARGET_H_
#define _BACKENDS_EBPF_TARGET_H_

#include "lib/cstring.h"
#include "lib/sourceCodeBuilder.h"
#include "lib/exceptions.h"

// We are prepared to support code generation using multiple styles
// (e.g., using BCC or using CLANG).

namespace EBPF {

class Target {
 protected:
    cstring name;
    explicit Target(cstring name) : name(name) {}
    Target() = delete;
    virtual ~Target() {}

 public:
    virtual void emitLicense(Util::SourceCodeBuilder* builder, cstring license) const = 0;
    virtual void emitCodeSection(Util::SourceCodeBuilder* builder, cstring sectionName) const = 0;
    virtual void emitIncludes(Util::SourceCodeBuilder* builder) const = 0;
    virtual void emitTableLookup(Util::SourceCodeBuilder* builder, cstring tblName,
                                 cstring key, cstring value) const = 0;
    virtual void emitTableUpdate(Util::SourceCodeBuilder* builder, cstring tblName,
                                 cstring key, cstring value) const = 0;
    virtual void emitTableDecl(Util::SourceCodeBuilder* builder,
                               cstring tblName, bool isHash,
                               cstring keyType, cstring valueType, unsigned size) const = 0;
};

// Represents a target that is compiled within the kernel
// source tree samples folder and which attaches to a socket
class KernelSamplesTarget : public Target {
 public:
    KernelSamplesTarget() : Target("Linux kernel") {}
    void emitLicense(Util::SourceCodeBuilder* builder, cstring license) const override;
    void emitCodeSection(Util::SourceCodeBuilder* builder, cstring sectionName) const override;
    void emitIncludes(Util::SourceCodeBuilder* builder) const override;
    void emitTableLookup(Util::SourceCodeBuilder* builder, cstring tblName,
                         cstring key, cstring value) const override;
    void emitTableUpdate(Util::SourceCodeBuilder* builder, cstring tblName,
                         cstring key, cstring value) const override;
    void emitTableDecl(Util::SourceCodeBuilder* builder,
                       cstring tblName, bool isHash,
                       cstring keyType, cstring valueType, unsigned size) const override;
};

// Represents a target compiled by bcc that uses the TC
class BccTarget : public Target {
 public:
    BccTarget() : Target("BCC") {}
    void emitLicense(Util::SourceCodeBuilder* builder, cstring license) const override;
    void emitCodeSection(Util::SourceCodeBuilder*, cstring) const override {}
    void emitIncludes(Util::SourceCodeBuilder* builder) const override;
    void emitTableLookup(Util::SourceCodeBuilder* builder, cstring tblName,
                         cstring key, cstring value) const override;
    void emitTableUpdate(Util::SourceCodeBuilder* builder, cstring tblName,
                         cstring key, cstring value) const override;
    void emitTableDecl(Util::SourceCodeBuilder* builder,
                       cstring tblName, bool isHash,
                       cstring keyType, cstring valueType, unsigned size) const override;
};

}  // namespace EBPF

#endif /* _BACKENDS_EBPF_TARGET_H_ */
