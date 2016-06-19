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

#ifndef BM_PDFIXED_INT_PD_CONN_MGR_H_
#define BM_PDFIXED_INT_PD_CONN_MGR_H_

#ifdef P4THRIFT
#include <p4thrift/protocol/TBinaryProtocol.h>
#include <p4thrift/transport/TSocket.h>
#include <p4thrift/transport/TTransportUtils.h>
#include <p4thrift/protocol/TMultiplexedProtocol.h>

namespace thrift_provider = p4::thrift;
#else
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/protocol/TMultiplexedProtocol.h>

namespace thrift_provider = apache::thrift;
#endif

#include <mutex>
#include <iostream>
#include <array>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <string>

#define NUM_DEVICES 256

template <typename A>
struct Client {
  A *c;
  std::unique_lock<std::mutex> _lock;
};

namespace detail {

struct TypeMapper {
  template <typename A>
  void add() {
    map.insert({std::type_index(typeid(A)), idx++});
  }

  template <typename A>
  size_t get() const {
    return map.at(std::type_index(typeid(A)));
  }

  size_t idx{0};
  std::unordered_map<std::type_index, size_t> map{};
};

struct ClientState {
  std::mutex mutex{};
  std::vector<void *> ptrs{};
  int dev_id{};
  boost::shared_ptr<::thrift_provider::protocol::TTransport> transport{nullptr};
};

}  // namespace detail

struct PdConnMgr {
  virtual ~PdConnMgr() { }

  template <typename A>
  Client<A> get(int dev_id) {
    static size_t idx = mapper.get<A>();
    auto &cstate = clients[dev_id];
    return {static_cast<A *>(cstate.ptrs[idx]),
          std::unique_lock<std::mutex>(cstate.mutex)};
  }

  virtual int client_init(int dev_id, int thrift_port_num) = 0;
  virtual int client_close(int dev_id) = 0;

 protected:
  std::array<detail::ClientState, NUM_DEVICES> clients;
  detail::TypeMapper mapper{};
};

namespace detail {

template <typename... Args> struct Connector;

template <>
struct Connector<> {
  static int connect(ClientState *cstate, int thrift_port_num,
                     std::vector<std::string>::iterator it) {
    (void) cstate; (void) thrift_port_num; (void) it;
    return 0;
  }
};

template <typename A, typename... Args>
struct Connector<A, Args...> {
  static int connect(ClientState *cstate, int thrift_port_num,
                      std::vector<std::string>::iterator it) {
    using namespace ::thrift_provider;  // NOLINT(build/namespaces)
    using namespace ::thrift_provider::protocol;  // NOLINT(build/namespaces)
    using namespace ::thrift_provider::transport;  // NOLINT(build/namespaces)

    boost::shared_ptr<TTransport> socket(
        new TSocket("localhost", thrift_port_num));
    boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
    boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

    boost::shared_ptr<TMultiplexedProtocol> sub_protocol(
        new TMultiplexedProtocol(protocol, *it));

    cstate->transport = transport;
    cstate->ptrs.push_back(static_cast<void *>(new A(sub_protocol)));

    try {
      transport->open();
    }
    catch (TException& tx) {
      std::cout << "Could not connect to port " << thrift_port_num
                << "(device " << cstate->dev_id << ")" << std::endl;
      return 1;
    }

    return Connector<Args...>::connect(cstate, thrift_port_num, ++it);
  }
};

template <typename... Args> struct Cleaner;

template <>
struct Cleaner<> {
  static void clean(ClientState *cstate) {
    assert(cstate->ptrs.empty());
  }
};

template <typename A, typename... Args>
struct Cleaner<A, Args...> {
  static void clean(ClientState *cstate) {
    delete static_cast<A *>(cstate->ptrs.back());
    cstate->ptrs.pop_back();
    Cleaner<Args...>::clean(cstate);
  }
};

template <typename... Args> struct TypeMapping;

template <>
struct TypeMapping<> {
  static void map(TypeMapper *mapper) {
    (void) mapper;
  }
};

template <typename A, typename... Args>
struct TypeMapping<A, Args...> {
  static void map(TypeMapper *mapper) {
    mapper->add<A>();
    TypeMapping<Args...>::map(mapper);
  }
};

template<typename... Args>
struct PdConnMgr_ : public PdConnMgr {
  template <typename ...Names>
  explicit PdConnMgr_(Names&&... names) {
    static_assert(sizeof...(Args) == sizeof...(Names),
                  "You need to provide as many names as client types");
    names_ = {names...};
    TypeMapping<Args...>::map(&mapper);
  }

  int client_init(int dev_id, int thrift_port_num) override {
    auto &cstate = clients[dev_id];
    cstate.dev_id = dev_id;
    assert(cstate.ptrs.empty());
    return Connector<Args...>::connect(&cstate, thrift_port_num,
                                       names_.begin());
  }

  int client_close(int dev_id) override {
    auto &cstate = clients[dev_id];
    assert(cstate.ptrs.size() == sizeof...(Args));
    cstate.transport->close();
    cstate.transport = nullptr;
    Cleaner<Args...>::clean(&cstate);
    assert(cstate.ptrs.empty());
    return 0;
  }

 private:
  std::vector<std::string> names_{};
};

}  // namespace detail

#undef NUM_DEVICES

#endif  // BM_PDFIXED_INT_PD_CONN_MGR_H_
