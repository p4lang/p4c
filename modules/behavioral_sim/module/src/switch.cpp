#include "behavioral_sim/switch.h"

Switch::Switch() {
  p4objects = std::unique_ptr<P4Objects>(new P4Objects());
}

void Switch::init_objects(const std::string &json_path) {
  std::fstream fs(json_path);
  p4objects->init_objects(fs);
}
