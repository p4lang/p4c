#include <fstream>
#include <iostream>

#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/text_format.h"
#include "ir/ir.h"
#include "p4/config/v1/p4info.pb.h"

using ::google::protobuf::TextFormat;
using ::p4::config::v1::P4Info;

const char kP4InfoFile[] = "program.p4info.txt";

int main() {
  // Demonstrate that compile-time generate p4info file can be accessed.
  std::ifstream p4info_file(kP4InfoFile);
  if (!p4info_file.is_open()) {
    std::cerr << "Unable to open p4info file: " << kP4InfoFile << std::endl;
    return 1;
  }
  google::protobuf::io::IstreamInputStream stream(&p4info_file);
  P4Info p4info;
  TextFormat::Parse(&stream, &p4info);
  std::cout << p4info.DebugString();

  // Demonstrate that IR extension is present.
  IR::MyCustomStatement statement("Hello, P4 extension world!");
  std::cout << statement << std::endl;
}
