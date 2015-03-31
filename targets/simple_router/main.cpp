/****************************************************************
 * 
 * Test module test target
 *
 ***************************************************************/

#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <vector>
#include <list>

// #include <cstring>
#include <cassert>

#include <unistd.h>

#include <boost/program_options.hpp>

#define PROFILER

#ifdef PROFILER
#include <gperftools/profiler.h>
#endif

extern "C" {
#include "BMI/bmi_port.h"
}

#include "simple_router.h"
#include "simplelog.h"

#define CHECK(x) assert(!x)

// #define PCAP_DUMP

static bmi_port_mgr_t *port_mgr;

static void packet_handler(int port_num, const char *buffer, int len) {
  SIMPLELOG << "Received packet of length " << len
	    << " on port " << port_num << std::endl;
  packet_accept(port_num, buffer, len);
}

static void transmit_fn(int port_num, const char *buffer, int len) {
  bmi_port_send(port_mgr, port_num, buffer, len);
}

class InterfaceList {
public:
  typedef std::list<std::string>::iterator iterator;
  typedef std::list<std::string>::const_iterator const_iterator;
public:
  void append(const std::string &iface) { ifaces.push_back(iface); }
  bool empty() { return ifaces.empty(); }
  // iterators
  iterator begin() { return ifaces.begin(); }
  const_iterator begin() const { return ifaces.begin(); }
  iterator end() { return ifaces.end(); }
  const_iterator end() const { return ifaces.end(); }
private:
  std::list<std::string> ifaces;
public:
  static InterfaceList *get_instance() {
    static InterfaceList list;
    return &list;
  }
};

static void parse_options(int argc, char* argv[]) {
  namespace po = boost::program_options;

  po::options_description description("Simple switch model");

  description.add_options()
    ("help,h", "Display this help message")
    ("interface,i", po::value<std::vector<std::string> >()->composing(),
     "Attach this network interface at startup\n");

  po::variables_map vm;

  try {
    po::store(
      po::command_line_parser(argc, argv).options(description).run(), 
      vm
    ); // throws on error
  }
  catch(...) {
    std::cout << "Error while parsing command line arguments" << std::endl;
    std::cout << description;
    exit(1);
  }

  if(vm.count("help")) {
    std::cout << description;
    exit(0);
  }

  if(vm.count("interface")) {
    for (const auto &iface : vm["interface"].as<std::vector<std::string> >()) {
      InterfaceList::get_instance()->append(iface);
    }
  }
}

int 
main(int argc, char* argv[])
{
  parse_options(argc, argv);

  std::cout << "Initializing BMI port manager\n";
  
  CHECK(bmi_port_create_mgr(&port_mgr));

  InterfaceList *ifaces = InterfaceList::get_instance();
  if(ifaces->empty()) {
    std::cout << "Adding all 4 ports\n";  
#ifdef PCAP_DUMP
    CHECK(bmi_port_interface_add(port_mgr, "veth1", 1, "port1.pcap"));
    CHECK(bmi_port_interface_add(port_mgr, "veth3", 2, "port2.pcap"));
    CHECK(bmi_port_interface_add(port_mgr, "veth5", 3, "port3.pcap"));
    CHECK(bmi_port_interface_add(port_mgr, "veth7", 4, "port4.pcap"));
#else
    CHECK(bmi_port_interface_add(port_mgr, "veth1", 1, NULL));
    CHECK(bmi_port_interface_add(port_mgr, "veth3", 2, NULL));
    CHECK(bmi_port_interface_add(port_mgr, "veth5", 3, NULL));
    CHECK(bmi_port_interface_add(port_mgr, "veth7", 4, NULL));
#endif
  }
  else {
    int port_num = 1;
    for(const auto &iface : *ifaces) {
      std::cout << "Adding interface " << iface
		<< " as port " << port_num << std::endl;
#ifdef PCAP_DUMP
      const std::string pcap = iface + ".pcap";
      CHECK(bmi_port_interface_add(port_mgr, iface.c_str(), port_num++, pcap.c_str()));
#else
      CHECK(bmi_port_interface_add(port_mgr, iface.c_str(), port_num++, NULL));
#endif
    }
  }
  
  start_processing(transmit_fn);
  
  CHECK(bmi_set_packet_handler(port_mgr, packet_handler));
  
  while(1) {
    std::this_thread::sleep_for(std::chrono::seconds(5));
#ifdef PROFILER
    ProfilerFlush();
#endif
  }
  
  bmi_port_destroy_mgr(port_mgr);
  
  /* printf("testmod target: no-op\n"); */
  
  return 0; 
}

