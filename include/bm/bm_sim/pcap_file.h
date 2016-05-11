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

#ifndef BM_BM_SIM_PCAP_FILE_H_
#define BM_BM_SIM_PCAP_FILE_H_

#include <pcap.h>

#include <stdexcept>
#include <mutex>
#include <cassert>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

#include "packet_handler.h"

namespace bm {

// A PcapPacket is a packet that has been read from a Pcap file.
// These packets can only be created by the PcapFileIn class.
class PcapPacket {
 private:
  unsigned port;  // port index where packet originated
  const u_char *data;  // type defined in pcap.h
  // The packet is only valid as long as the cursor
  // in the PcapFileIn is not changed, since the pcap_header*
  // points to a buffer returned by the pcap library.
  const pcap_pkthdr *pcap_header;

  PcapPacket(unsigned port, const u_char *data, const pcap_pkthdr *pcap_header)
    : port(port),
      data(data),
      pcap_header(pcap_header) { }

  // Only class that can call constructor
  friend class PcapFileIn;

 public:
  static std::string timevalToString(const struct timeval *tv);

  const char *getData() const { return reinterpret_cast<const char *>(data); }
  unsigned getLength() const { return static_cast<unsigned>(pcap_header->len); }
  const struct timeval *getTime() const { return &pcap_header->ts; }
  unsigned getPort() const { return port; }
};

class PcapFileBase {
 protected:
  unsigned port;  // port number represented by this file
  std::string filename;

  PcapFileBase(unsigned port, std::string filename);
};

// C++ wrapper around a pcap file that is used for reading:
// A stream-like abstraction of a PCAP (Packet capture) file.
// Assumes that all packets in a file are sorted on time.
class PcapFileIn : public PcapFileBase {
 public:
  PcapFileIn(unsigned port, std::string filename);
  virtual ~PcapFileIn();

  // Returns pointer to current packet
  std::unique_ptr<PcapPacket> current() const;
  // Advances to next packet.  Should be called before current().
  // Returns 'false' on end of file.
  bool moveNext();
  // restart from the beginning
  void reset();
  // True if we have reached end of file.
  bool atEOF() const;

 private:
  pcap_t *pcap;
  pcap_pkthdr *current_header;
  const u_char *current_data;

  enum class State {
    Uninitialized,
    Opened,
    Reading,
    AtEnd
  };

  State state;

 private:
  void open();
  bool advance();
  void deallocate();

  PcapFileIn(PcapFileIn const& ) = delete;
  PcapFileIn& operator=(PcapFileIn const&) = delete;
};


class PcapFileOut :
    public PcapFileBase {
 public:
  // port is not really used
  PcapFileOut(unsigned port, std::string filename);
  virtual ~PcapFileOut();
  void writePacket(const char *data, unsigned length);

 private:
  pcap_t *pcap;
  pcap_dumper_t *dumper;

  PcapFileOut(PcapFileOut const& ) = delete;
  PcapFileOut& operator=(PcapFileOut const&) = delete;
};


// Reads data from a set of Pcap files; returns packets in order
// of their timestamps.
class PcapFilesReader :
    public PacketDispatcherIface {
 public:
  // Read packets from a set of files in timestamp order.  Each file is
  // associated to a port number corresponding to its index in the files vector.
  // If 'respectTiming' is true, the PcapFilesReader only emits packets at the
  // proper times, adjusted relative to the time of the 'start' call.  In this
  // case the 'start' method should probably be invoked by the caller on a
  // separate thread.  'wait_time_in_seconds' is the time that the reader should
  // wait before starting to process packets.
  PcapFilesReader(bool respectTiming, unsigned wait_time_in_seconds);
  // Add a file corresponding to the specified port.
  void addFile(unsigned port, std::string file);
  void start();  // start processing the pcap files

  // Invoked every time a packet is read
  PacketDispatcherIface::ReturnCode set_packet_handler(
      const PacketHandler &handler, void *cookie);

 private:
  std::vector<std::unique_ptr<PcapFileIn>> files;
  unsigned nonEmptyFiles;
  unsigned wait_time_in_seconds;
  bool     respectTiming;
  // Time when observable starts to read packets
  struct timeval startTime;
  // Time of first packet across all files
  struct timeval firstPacketTime;
  // constant zero (could be static, but it's easier to initialize it this way)
  struct timeval zero;

  // index of next file to read packet from
  unsigned scheduledIndex;
  bool started;

  void scan();
  void schedulePacket(unsigned index, const struct timeval *delay);
  void timerFired();  // send a scheduled packet

  PcapFilesReader(PcapFilesReader const& ) = delete;
  PcapFilesReader& operator=(PcapFilesReader const&) = delete;

  // Function to call when a packet is read
  PacketHandler handler;
  void *cookie;
};


// Writes data to a set of Pcap files.
class PcapFilesWriter : public PacketReceiverIface {
 public:
  PcapFilesWriter();
  // Add a file corresponding to the specified port.
  void addFile(unsigned port, std::string file);
  void send_packet(int port_num, const char *buffer, int len);

 private:
  std::unordered_map<unsigned, std::unique_ptr<PcapFileOut>> files;

  PcapFilesWriter(PcapFilesWriter const& ) = delete;
  PcapFilesWriter& operator=(PcapFilesWriter const&) = delete;
};

}  // namespace bm

#endif  // BM_BM_SIM_PCAP_FILE_H_
