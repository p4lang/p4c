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

#ifndef _BM_CHECKSUMS_H_
#define _BM_CHECKSUMS_H_

#include "phv.h"
#include "named_p4object.h"
#include "calculations.h"
#include "logger.h"

class Checksum : public NamedP4Object{
public:
  Checksum(const std::string &name, p4object_id_t id,
	   header_id_t header_id, int field_offset);
  virtual ~Checksum() { }

public:
  void update(Packet *pkt) const {
    BMLOG_DEBUG_PKT(*pkt, "Updating checksum '{}'", get_name());
    update_(pkt);
  }

  bool verify(const Packet &pkt) const {
    bool valid = verify_(pkt);
    BMLOG_DEBUG_PKT(pkt, "Verifying checksum '{}': {}", get_name(), valid);
    return valid;
  }

private:
  virtual void update_(Packet *pkt) const = 0;
  virtual bool verify_(const Packet &pkt) const = 0;

protected:
  header_id_t header_id;
  int field_offset;
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

#endif
