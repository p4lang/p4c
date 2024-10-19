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

#ifndef BF_P4C_PHV_V2_TX_SCORE_H_
#define BF_P4C_PHV_V2_TX_SCORE_H_

#include "bf-p4c/phv/utils/utils.h"

namespace PHV {
namespace v2 {

/// TxScore is the interface of an allocation score.
class TxScore {
 public:
    virtual bool better_than(const TxScore *other) const = 0;
    virtual std::string str() const = 0;
};

/// TxScoreMaker is the interface of the factory class of TxScore.
class TxScoreMaker {
 public:
    virtual TxScore *make(const Transaction &tx) const = 0;
};

/// NilTxScore is an empty score implementation. Use it when you do not care about
/// score of transaction.
class NilTxScore : public TxScore {
 public:
    bool better_than(const TxScore *) const override { return false; }
    std::string str() const override { return "nil"; };
};

/// NilTxScoreMaker is a factory class to make nil score.
class NilTxScoreMaker : public TxScoreMaker {
 public:
    TxScore *make(const Transaction &) const override { return new NilTxScore(); }
};

/// MinPackTxScore prefers minimal packing.
class MinPackTxScore : public TxScore {
 public:
    // number of container that is used for packing.
    int n_packed = 0;
    bool use_mocha_or_dark = false;
    MinPackTxScore(int n_packed, bool use_mocha_or_dark)
        : n_packed(n_packed), use_mocha_or_dark(use_mocha_or_dark) {}
    bool better_than(const TxScore *other) const override;
    std::string str() const override;
};

/// factory class for MinPackTxScore.
class MinPackTxScoreMaker : public TxScoreMaker {
 public:
    TxScore *make(const Transaction &tx) const override;
};

}  // namespace v2
}  // namespace PHV

#endif /* BF_P4C_PHV_V2_TX_SCORE_H_ */
