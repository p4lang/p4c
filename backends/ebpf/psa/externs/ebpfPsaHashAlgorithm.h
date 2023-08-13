/*
Copyright 2022-present Orange
Copyright 2022-present Open Networking Foundation

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
#ifndef BACKENDS_EBPF_PSA_EXTERNS_EBPFPSAHASHALGORITHM_H_
#define BACKENDS_EBPF_PSA_EXTERNS_EBPFPSAHASHALGORITHM_H_

#include <vector>

#include "backends/ebpf/codeGen.h"
#include "backends/ebpf/ebpfObject.h"
#include "ir/ir.h"
#include "ir/visitor.h"
#include "lib/cstring.h"

namespace EBPF {

class EBPFProgram;

class EBPFHashAlgorithmPSA : public EBPFObject {
 public:
    typedef std::vector<const IR::Expression *> ArgumentsList;

 protected:
    cstring baseName;
    const EBPFProgram *program;
    Visitor *visitor;

    ArgumentsList unpackArguments(const IR::MethodCallExpression *expr, int dataPos);

 public:
    // keep this enum in sync with psa.p4 file
    enum HashAlgorithm {
        IDENTITY,
        CRC32,
        CRC32_CUSTOM,
        CRC16,
        CRC16_CUSTOM,
        ONES_COMPLEMENT16,  // aka InternetChecksum
        TARGET_DEFAULT
    };

    EBPFHashAlgorithmPSA(const EBPFProgram *program, cstring name)
        : baseName(name), program(program), visitor(nullptr) {}

    void setVisitor(Visitor *instance) { this->visitor = instance; }

    virtual unsigned getOutputWidth() const { return 0; }

    // decl might be a null pointer
    virtual void emitVariables(CodeBuilder *builder, const IR::Declaration_Instance *decl) = 0;

    virtual void emitClear(CodeBuilder *builder) = 0;
    virtual void emitAddData(CodeBuilder *builder, int dataPos,
                             const IR::MethodCallExpression *expr);
    virtual void emitAddData(CodeBuilder *builder, const ArgumentsList &arguments) = 0;
    virtual void emitGet(CodeBuilder *builder) = 0;

    virtual void emitSubtractData(CodeBuilder *builder, int dataPos,
                                  const IR::MethodCallExpression *expr);
    virtual void emitSubtractData(CodeBuilder *builder, const ArgumentsList &arguments) = 0;

    virtual void emitGetInternalState(CodeBuilder *builder) = 0;
    virtual void emitSetInternalState(CodeBuilder *builder,
                                      const IR::MethodCallExpression *expr) = 0;
};

class CRCChecksumAlgorithm : public EBPFHashAlgorithmPSA {
 protected:
    cstring registerVar;
    cstring initialValue;
    cstring updateMethod;
    cstring finalizeMethod;
    cstring polynomial;
    const int crcWidth;

 public:
    CRCChecksumAlgorithm(const EBPFProgram *program, cstring name, int width)
        : EBPFHashAlgorithmPSA(program, name), crcWidth(width) {}

    unsigned getOutputWidth() const override { return crcWidth; }

    static void emitUpdateMethod(CodeBuilder *builder, int crcWidth);

    void emitVariables(CodeBuilder *builder, const IR::Declaration_Instance *decl) override;

    void emitClear(CodeBuilder *builder) override;
    void emitAddData(CodeBuilder *builder, const ArgumentsList &arguments) override;
    void emitGet(CodeBuilder *builder) override;

    void emitSubtractData(CodeBuilder *builder, const ArgumentsList &arguments) override;

    void emitGetInternalState(CodeBuilder *builder) override;
    void emitSetInternalState(CodeBuilder *builder, const IR::MethodCallExpression *expr) override;
};

/**
 * For CRC16 calculation we use a polynomial 0x8005.
 * - updateMethod adds a data to the checksum
 * and performs a CRC16 calculation
 * - finalizeMethod returns the CRC16 result
 *
 * Above C functions are emitted via emitGlobals.
 */
class CRC16ChecksumAlgorithm : public CRCChecksumAlgorithm {
 public:
    CRC16ChecksumAlgorithm(const EBPFProgram *program, cstring name)
        : CRCChecksumAlgorithm(program, name, 16) {
        initialValue = "0";
        // We use a 0x8005 polynomial.
        // 0xA001 comes from 0x8005 value bits reflection.
        polynomial = "0xA001";
        updateMethod = "crc16_update";
        finalizeMethod = "crc16_finalize";
    }

    static void emitGlobals(CodeBuilder *builder);
};

/**
 * For CRC32 calculation we use a polynomial 0x04C11DB7.
 * - updateMethod adds a data to the checksum
 * and performs a CRC32 calculation
 * - finalizeMethod finalizes a CRC32 calculation
 * and returns the CRC32 result
 *
 * Above C functions are emitted via emitGlobals.
 */
class CRC32ChecksumAlgorithm : public CRCChecksumAlgorithm {
 public:
    CRC32ChecksumAlgorithm(const EBPFProgram *program, cstring name)
        : CRCChecksumAlgorithm(program, name, 32) {
        initialValue = "0xffffffff";
        // We use a 0x04C11DB7 polynomial.
        // 0xEDB88320 comes from 0x04C11DB7 value bits reflection.
        polynomial = "0xEDB88320";
        updateMethod = "crc32_update";
        finalizeMethod = "crc32_finalize";
    }

    static void emitGlobals(CodeBuilder *builder);
};

class InternetChecksumAlgorithm : public EBPFHashAlgorithmPSA {
 protected:
    cstring stateVar;

    void updateChecksum(CodeBuilder *builder, const ArgumentsList &arguments, bool addData);

 public:
    InternetChecksumAlgorithm(const EBPFProgram *program, cstring name)
        : EBPFHashAlgorithmPSA(program, name) {}

    unsigned getOutputWidth() const override { return 16; }

    static void emitGlobals(CodeBuilder *builder);

    void emitVariables(CodeBuilder *builder, const IR::Declaration_Instance *decl) override;

    void emitClear(CodeBuilder *builder) override;
    void emitAddData(CodeBuilder *builder, const ArgumentsList &arguments) override;
    void emitGet(CodeBuilder *builder) override;

    void emitSubtractData(CodeBuilder *builder, const ArgumentsList &arguments) override;

    void emitGetInternalState(CodeBuilder *builder) override;
    void emitSetInternalState(CodeBuilder *builder, const IR::MethodCallExpression *expr) override;
};

class EBPFHashAlgorithmTypeFactoryPSA {
 public:
    static EBPFHashAlgorithmTypeFactoryPSA *instance() {
        static EBPFHashAlgorithmTypeFactoryPSA factory;
        return &factory;
    }

    EBPFHashAlgorithmPSA *create(int type, const EBPFProgram *program, cstring name) {
        if (type == EBPFHashAlgorithmPSA::HashAlgorithm::CRC32)
            return new CRC32ChecksumAlgorithm(program, name);
        else if (type == EBPFHashAlgorithmPSA::HashAlgorithm::CRC16)
            return new CRC16ChecksumAlgorithm(program, name);
        else if (type == EBPFHashAlgorithmPSA::HashAlgorithm::ONES_COMPLEMENT16 ||
                 type == EBPFHashAlgorithmPSA::HashAlgorithm::TARGET_DEFAULT)
            return new InternetChecksumAlgorithm(program, name);

        return nullptr;
    }

    void emitGlobals(CodeBuilder *builder) {
        CRC16ChecksumAlgorithm::emitGlobals(builder);
        CRC32ChecksumAlgorithm::emitGlobals(builder);
        InternetChecksumAlgorithm::emitGlobals(builder);
    }
};

}  // namespace EBPF

#endif /* BACKENDS_EBPF_PSA_EXTERNS_EBPFPSAHASHALGORITHM_H_ */
