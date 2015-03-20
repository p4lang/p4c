#ifndef _BM_CHECKSUMS_H_
#define _BM_CHECKSUMS_H_

#include "bm_sim/phv.h"
#include "bm_sim/named_p4object.h"

class Checksum : public NamedP4Object{
public:
  Checksum(const string &name, p4object_id_t id,
	   header_id_t header_id, int field_offset);
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
  virtual bool verify(const PHV &phv) const override;
};

#endif
