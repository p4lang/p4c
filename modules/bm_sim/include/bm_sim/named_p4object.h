#ifndef _BM_NAMED_P4OBJECT_H_
#define _BM_NAMED_P4OBJECT_H_

#include <string>

typedef int p4object_id_t;

class NamedP4Object {
public:
  NamedP4Object(const std::string &name, p4object_id_t id)
    : name(name), id(id) {}
  virtual ~NamedP4Object() { };

  const std::string &get_name() const { return name; }

  p4object_id_t get_id() const { return id; }

protected:
  const std::string name;
  p4object_id_t id;
};

#endif
