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

#include "bm_sim/phv.h"
#include "bm_sim/named_p4object.h"

class Checksum : public NamedP4Object{
public:
  Checksum(const string &name, p4object_id_t id,
	   header_id_t header_id, int field_offset);
  virtual ~Checksum() { }

  virtual void update(PHV *phv) const = 0;
  virtual bool verify(const PHV &phv) const = 0;

protected:
  header_id_t header_id;
  int field_offset;
};

class IPv4Checksum : public Checksum {
public:
  IPv4Checksum(const string &name, p4object_id_t id,
	       header_id_t header_id, int field_offset);
  void update(PHV *phv) const override;
  bool verify(const PHV &phv) const override;
};

#endif
