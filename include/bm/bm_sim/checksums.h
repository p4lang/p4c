/* Copyright 2013-present Barefoot Networks, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Antonin Bas (antonin@barefootnetworks.com)
 *
 */

#ifndef BM_BM_SIM_CHECKSUMS_H_
#define BM_BM_SIM_CHECKSUMS_H_

#include <string>
#include <memory>

#include "phv.h"
#include "named_p4object.h"
#include "calculations.h"
#include "expressions.h"

namespace bm {

class Checksum : public NamedP4Object{
 public:
  Checksum(const std::string &name, p4object_id_t id,
           header_id_t header_id, int field_offset);

  virtual ~Checksum() { }

  void update(Packet *pkt) const;

  bool verify(const Packet &pkt) const;

  void set_checksum_condition(std::unique_ptr<Expression> cksum_condition);

 private:
  virtual void update_(Packet *pkt) const = 0;
  virtual bool verify_(const Packet &pkt) const = 0;

  bool is_checksum_condition_met(const Packet &pkt) const;

 protected:
  header_id_t header_id;
  int field_offset;

 private:
  std::unique_ptr<Expression> condition{nullptr};
};

class CalcBasedChecksum : public Checksum {
 public:
  CalcBasedChecksum(const std::string &name, p4object_id_t id,
                    header_id_t header_id, int field_offset,
                    const NamedCalculation *calculation);

  CalcBasedChecksum(const CalcBasedChecksum &other) = delete;
  CalcBasedChecksum &operator=(const CalcBasedChecksum &other) = delete;

  CalcBasedChecksum(CalcBasedChecksum &&other) = default;
  CalcBasedChecksum &operator=(CalcBasedChecksum &&other) = default;

 private:
  void update_(Packet *pkt) const override;
  bool verify_(const Packet &pkt) const override;

 private:
  const NamedCalculation *calculation{nullptr};
};

class IPv4Checksum : public Checksum {
 public:
  IPv4Checksum(const std::string &name, p4object_id_t id,
               header_id_t header_id, int field_offset);

 private:
  void update_(Packet *pkt) const override;
  bool verify_(const Packet &pkt) const override;
};

}  // namespace bm

#endif  // BM_BM_SIM_CHECKSUMS_H_
