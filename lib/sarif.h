// SPDX-FileCopyrightText: 2026 The P4 Language Consortium
// SPDX-License-Identifier: Apache-2.0

#ifndef LIB_SARIF_H_
#define LIB_SARIF_H_

#include <iosfwd>
#include <string>
#include <utility>
#include <vector>

#include "lib/diagnostic.h"

namespace P4 {

/// SARIF is an output adapter over the public, serialization-independent model.
void writeSarif(std::ostream &out, const std::vector<Diagnostic> &diagnostics);

/// Keeps a complete document on disk after initialization and every diagnostic.
/// Does not own diagnostic data. Ordinary early returns need no finalization.
class SarifWriter {
    std::string file;

 public:
    explicit SarifWriter(std::string file) : file(std::move(file)) {}
    bool write(const std::vector<Diagnostic> &diagnostics) const;
    const std::string &filename() const { return file; }
};

}  // namespace P4

#endif /* LIB_SARIF_H_ */
