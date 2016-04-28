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

#include <gtest/gtest.h>

#include <boost/filesystem.hpp>

#include <bm/bm_sim/pcap_file.h>
#include <stdio.h>

using namespace bm;

namespace fs = boost::filesystem;

#ifndef TESTDATADIR
#define TESTDATADIR "testdata"
#endif

// Google Test fixture for pcap tests
class PcapTest : public ::testing::Test {
protected:
  PcapTest()
    : testDataFolder(TESTDATADIR), testfile1("en0.pcap"), testfile2("lo0.pcap"),
      tmpfile("tmp.pcap"), received(0), receiver(nullptr) {}

  virtual void SetUp()  {}

  virtual void TearDown() {
    std::string tmpfile = getTmpFile();
    remove(tmpfile.c_str());
  }

  std::string getFile1() {
    fs::path path = fs::path(testDataFolder) / fs::path(testfile1);
    return path.string();
  }

  std::string getFile2() {
    fs::path path = fs::path(testDataFolder) / fs::path(testfile2);
    return path.string();
  }

  std::string getTmpFile() {
    fs::path path = fs::path(tmpfile);
    return path.string();
  }

  void setReceiver(PacketReceiverIface* recv) {
    receiver = recv;
  }
  
public:
  int receive(int port_num, const char *buffer, int len) {
    received++;
    if (receiver != nullptr)
      receiver->send_packet(port_num, buffer, len);
    return received;
  }

protected:
  PacketReceiverIface* receiver;
  int received;
  std::string testDataFolder;
  std::string testfile1;
  std::string testfile2;
  std::string tmpfile;
};

// exit codes are based on the cmp file comparison utility
// #define OK 0
// #define DIFFERENT 1
// #define EXCEPTION 2

class PcapFileComparator
{
public:
  enum class Status { OK, DIFFERENT, EXCEPTION };

private:
  bool verbose;
  unsigned packetIndex;

  Status reportDifference(std::string message) {
    if (this->verbose)
      std::cerr << message << std::endl;
    return Status::DIFFERENT;
  }

  Status comparePackets(std::unique_ptr<PcapPacket> p1,
                        std::unique_ptr<PcapPacket> p2) {
    unsigned p1len = p1->getLength();
    unsigned p2len = p2->getLength();

    if (p1len != p2len)
      return this->reportDifference(
          std::string("Packet with index ") +
          std::to_string(this->packetIndex) + " differs in length");
    const char* p1d = p1->getData();
    const char* p2d = p2->getData();

    int cmp = memcmp(p1d, p2d, p1len);
    if (cmp != 0) {
      return this->reportDifference(
          std::string("Packet with index ") +
          std::to_string(this->packetIndex) + " differs ");
    }

    return Status::OK;
  }
    
public:
  PcapFileComparator(bool verbose)
      : verbose(verbose), packetIndex(0) {}

  Status compare(std::string file1, std::string file2) {
    PcapFileIn pf1(0, file1);
    PcapFileIn pf2(0, file2);

    while (true) {
      bool f1 = pf1.moveNext();
      bool f2 = pf2.moveNext();

      if (f1 != f2) {
        if (f1 == false)
          return this->reportDifference(file1 + " is shorter");
        if (f2 == false)
          return this->reportDifference(file2 + " is shorter");
      }

      if (!f1)
        break;

      std::unique_ptr<PcapPacket> p1 = pf1.current();
      std::unique_ptr<PcapPacket> p2 = pf2.current();

      Status cp = this->comparePackets(std::move(p1), std::move(p2));
      if (cp != Status::OK)
        return cp;

      this->packetIndex++;
    }
        
    return Status::OK;
  }
};

TEST_F(PcapTest, ReadOneFile) {
  std::unique_ptr<PcapFileIn> file =
      std::unique_ptr<PcapFileIn>(new PcapFileIn(0, getFile1()));

  unsigned packetsRead = 0;
  unsigned packetsSize = 0;
  while (file->moveNext()) {
    std::unique_ptr<PcapPacket> packet = file->current();
    packetsRead++;
    packetsSize += packet->getLength();
  }
    
  file->reset();
  while (file->moveNext()) {
    std::unique_ptr<PcapPacket> packet = file->current();
    packetsRead--;
    packetsSize -= packet->getLength();
  }
    
  ASSERT_EQ(packetsRead, 0);
  ASSERT_EQ(packetsSize, 0);
}


static void
packet_handler(int port_num, const char *buffer, int len, void *cookie) {
  ((PcapTest*) cookie)->receive(port_num, buffer, len);
}


TEST_F(PcapTest, MergeFiles) {
  PcapFilesReader reader(false, 0);
  reader.addFile(0, getFile1());
  reader.addFile(1, getFile2());
  reader.set_packet_handler(packet_handler, (void*)this);
  reader.start();
  int totalPackets = received;
  received = 0;
    
  PcapFilesReader reader1(false, 0);
  reader1.addFile(0, getFile1());
  reader1.set_packet_handler(packet_handler, (void*)this);
  reader1.start();
  int file1Packets = received;
  received = 0;

  PcapFilesReader reader2(false, 0);
  reader2.addFile(0, getFile2());
  reader2.set_packet_handler(packet_handler, (void*)this);
  reader2.start();
  int file2Packets = received;

  ASSERT_EQ(totalPackets, file1Packets + file2Packets);
}

TEST_F(PcapTest, Write) {
  typedef PcapFileComparator::Status Status;

  PcapFilesReader reader(false, 0);
  reader.addFile(0, getFile1());
  reader.addFile(1, getFile2());
  reader.set_packet_handler(packet_handler, (void*)this);

  PcapFilesWriter writer;
  setReceiver(&writer);
  // only write packets for port 0, drop packets for port 1
  writer.addFile(0, getTmpFile());

  reader.start();
  setReceiver(nullptr);

  PcapFileComparator comparator(false);
  Status comparison = comparator.compare(getFile1(), getTmpFile());
  ASSERT_EQ(Status::OK, comparison);
}
