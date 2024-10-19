/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#ifndef BACKENDS_TOFINO_BF_P4C_LIB_ERROR_TYPE_H_
#define BACKENDS_TOFINO_BF_P4C_LIB_ERROR_TYPE_H_

#include <lib/error_catalog.h>

namespace BFN {

using namespace P4;

/// Barefoot specific error and warning types.
class ErrorType : public P4::ErrorType {
 public:
    static const int WARN_TABLE_PLACEMENT = 1501;
    static const int WARN_PRAGMA_USE = 1502;
    static const int WARN_SUBSTITUTION = 1503;
    static const int WARN_PHV_ALLOCATION = 1504;
    static const int WARN_UNINIT_OVERLAY = 1505;

    static const int FIRST_BACKEND_WARNING = WARN_TABLE_PLACEMENT;

    /// in case we need to
    static ErrorType &getErrorTypes() {
        static ErrorType instance;
        return instance;
    }

    void printWarningsHelp(std::ostream &out);

 private:
    /// Barefoot specific error catalog.
    /// It simply adds all the supported errors and warnings in Barefoot's backend.
    ErrorType() {
        P4::ErrorCatalog::getCatalog()
            .add<ErrorMessage::MessageType::Warning, WARN_TABLE_PLACEMENT>("table-placement");
        P4::ErrorCatalog::getCatalog().add<ErrorMessage::MessageType::Warning, WARN_PRAGMA_USE>(
            "pragma-use");
        P4::ErrorCatalog::getCatalog().add<ErrorMessage::MessageType::Warning, WARN_SUBSTITUTION>(
            "substitution");
        P4::ErrorCatalog::getCatalog().add<ErrorMessage::MessageType::Warning, WARN_PHV_ALLOCATION>(
            "phv-allocation");
        P4::ErrorCatalog::getCatalog().add<ErrorMessage::MessageType::Warning, WARN_UNINIT_OVERLAY>(
            "uninitialized-overlay");
    }
};

}  // end namespace BFN

#endif /* BACKENDS_TOFINO_BF_P4C_LIB_ERROR_TYPE_H_ */
