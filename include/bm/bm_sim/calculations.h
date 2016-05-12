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

//! @file calculations.h
//!
//! Each hash algorithm supported by the target needs to be declared as a
//! functor. This is how you declare one:
//! @code
//! struct hash_ex {
//!   uint32_t operator()(const char *buf, size_t s) const {
//!     const int p = 16777619;
//!     int hash = 2166136261;

//!     for (size_t i = 0; i < s; i++)
//!       hash = (hash ^ buf[i]) * p;

//!     hash += hash << 13;
//!     hash ^= hash >> 7;
//!     hash += hash << 3;
//!     hash ^= hash >> 17;
//!     hash += hash << 5;
//!     return static_cast<uint32_t>(hash);
//!   }
//! };
//! @endcode
//! Note that the signature of the functor must exactly match the one of the
//! example: `uint32_t(const char *buf, size_t s) const`. Otherwise, you will
//! get a compilation error.
//!
//! You can then register your hash function like this:
//! @code
//! REGISTER_HASH(hash_ex)
//! @endcode
//! In P4 v1.0.2, hash algorithms are used by `field_list_calculation` objects


#ifndef BM_BM_SIM_CALCULATIONS_H_
#define BM_BM_SIM_CALCULATIONS_H_

#include <memory>
#include <type_traits>
#include <string>
#include <unordered_map>
#include <tuple>
#include <vector>
#include <algorithm>   // for std::copy
#include <ostream>

#include "boost/variant.hpp"

#include "phv.h"
#include "packet.h"

namespace bm {

/* Used to determine whether a class overloads the '()' operator */
template <typename T>
struct defines_functor_operator {
  typedef char (& yes)[1];
  typedef char (& no)[2];

  // we need a template here to enable SFINAE
  template <typename U>
  static yes deduce(char (*)[sizeof(&U::operator())]);
  // fallback
  template <typename> static no deduce(...);

  static bool constexpr value = sizeof(deduce<T>(0)) == sizeof(yes);
};

namespace detail {

template <class ReturnType, class... Args>
struct callable_traits_base {
  using return_type = ReturnType;
  using argument_type = std::tuple<Args...>;

  template<std::size_t I>
  using arg = typename std::tuple_element<I, argument_type>::type;
};

}  // namespace detail

/* Used to determine the return type and argument type of the '()' operator */
template <class T>
struct callable_traits : callable_traits<decltype(&T::operator())> { };

/* lambda / functor */
template <class ClassType, class ReturnType, class... Args>
struct callable_traits<ReturnType(ClassType::*)(Args...) const>
  : detail::callable_traits_base<ReturnType, Args...> { };

/* non-const case */
template <class ClassType, class ReturnType, class... Args>
struct callable_traits<ReturnType(ClassType::*)(Args...)>
  : detail::callable_traits_base<ReturnType, Args...> { };

template <bool functor, typename H>
struct HashChecker {
 protected:
  static bool constexpr valid_hash = false;

  ~HashChecker() { }
};

/* specific to hash algorithms
   checks that the signature of the '()' operator is what we want */
template<typename T, typename Return>
struct check_functor_signature {
  template<typename U, Return (U::*)(const char *, size_t) const>
  struct SFINAE {};

  template<typename U> static char Test(SFINAE<U, &U::operator()>*);
  template<typename U> static int Test(...);
  static const bool value = sizeof(Test<T>(0)) == sizeof(char);
};

/* provides "readable" error messages if an hash functor was not define
   correctly */
template <typename H>
struct HashChecker<true, H> {
 private:
  typedef typename callable_traits<H>::return_type return_type;
  typedef typename callable_traits<H>::argument_type argument_type;

  static bool constexpr v1 = std::is_unsigned<return_type>::value;
  static bool constexpr v2 =
    std::is_same<argument_type, std::tuple<const char *, size_t>>::value;

  static_assert(v1, "Invalid return type for HashFn");
  static_assert(v2, "Invalid parameters for HashFn");

  static bool constexpr v3 = check_functor_signature<H, return_type>::value;
  static_assert(!(v1 && v2) || v3, "HashFn is not const");

 protected:
  static bool constexpr valid_hash = v1 && v2 && v3;

  ~HashChecker() { }
};


template <typename U,
          typename std::enable_if<std::is_unsigned<U>::value, int>::type = 0>
class RawCalculationIface {
 public:
  U output(const char *buffer, size_t s) const {
    return output_(buffer, s);
  }

  std::unique_ptr<RawCalculationIface<U> > clone() const {
    return std::unique_ptr<RawCalculationIface<U> > (clone_());
  }

  virtual ~RawCalculationIface() { }

 private:
  virtual U output_(const char *buffer, size_t s) const = 0;

  virtual RawCalculationIface<U> *clone_() const = 0;
};


template <typename T, typename HashFn,
          typename std::enable_if<std::is_unsigned<T>::value, int>::type = 0>
class RawCalculation
  : HashChecker<defines_functor_operator<HashFn>::value, HashFn>,
    public RawCalculationIface<T> {
 public:
  // explicit RawCalculation(const HashFn &hash) : hash(hash) { }

  std::unique_ptr<RawCalculation<T, HashFn> > clone() const {
    return std::unique_ptr<RawCalculation<T, HashFn> > (clone_());
  }

  HashFn &get_hash_fn() { return hash; }

 private:
  T output_(const char *buffer, size_t s) const override {
    return output__(buffer, s);
  }

  RawCalculation<T, HashFn> *clone_() const override {
    RawCalculation<T, HashFn> *ptr = new RawCalculation<T, HashFn>();
    return ptr;
  }

  typedef HashChecker<defines_functor_operator<HashFn>::value, HashFn> HC;

  static_assert(defines_functor_operator<HashFn>::value,
                "HashFn needs to overload '()' operator");

  template <typename U = T,
            typename std::enable_if<std::is_unsigned<U>::value, int>::type = 0>
  typename std::enable_if<HC::valid_hash, U>::type
  output__(const char *buffer, size_t s) const {
    return static_cast<U>(hash(buffer, s));
  }

  template <typename U = T,
            typename std::enable_if<std::is_unsigned<U>::value, int>::type = 0>
  typename std::enable_if<!HC::valid_hash, U>::type
  output__(const char *buffer, size_t s) const {
    (void) buffer; (void) s;
    return static_cast<U>(0u);
  }

  HashFn hash;
};


struct BufBuilder {
  struct field_t {
    header_id_t header;
    int field_offset;
  };

  struct constant_t {
    ByteContainer v;
    size_t nbits;
  };

  struct header_t {
    header_id_t header;
  };

  std::vector<boost::variant<field_t, constant_t, header_t> > entries{};
  bool with_payload{false};

  void push_back_field(header_id_t header, int field_offset) {
    field_t f = {header, field_offset};
    entries.emplace_back(f);
  }

  void push_back_constant(const ByteContainer &v, size_t nbits) {
    // TODO(antonin): general case
    assert(nbits % 8 == 0);
    constant_t c = {v, nbits};
    entries.emplace_back(c);
  }

  void push_back_header(header_id_t header) {
    header_t h = {header};
    entries.emplace_back(h);
  }

  void append_payload() {
    with_payload = true;
  }

  struct Deparse : public boost::static_visitor<> {
    Deparse(const PHV &phv, ByteContainer *buf)
      : phv(phv), buf(buf) { }

    char *extend(int more_bits) {
      int nbits_ = nbits + more_bits;
      buf->resize((nbits_ + 7) / 8, '\x00');
      char *ptr = buf->data() + (nbits / 8);
      nbits = nbits_;
      // needed ?
      // if (new_bytes > 0) buf->back() = '\x00';
      return ptr;
    }

    int get_offset() const {
      return nbits % 8;
    }

    void operator()(const field_t &f) {
      const Header &header = phv.get_header(f.header);
      if (!header.is_valid()) return;
      const Field &field = header.get_field(f.field_offset);
      // taken from headers.cpp::deparse
      const int offset = get_offset();
      field.deparse(extend(field.get_nbits()), offset);
    }

    void operator()(const constant_t &c) {
      assert(get_offset() == 0);
      std::copy(c.v.begin(), c.v.end(), extend(c.nbits));
    }

    void operator()(const header_t &h) {
      assert(get_offset() == 0);
      const Header &header = phv.get_header(h.header);
      if (header.is_valid()) {
        header.deparse(extend(header.get_nbytes_packet() * 8));
      }
    }

    Deparse(const Deparse &other) = delete;
    Deparse &operator=(const Deparse &other) = delete;

    Deparse(Deparse &&other) = delete;
    Deparse &operator=(Deparse &&other) = delete;

    const PHV &phv;
    ByteContainer *buf;
    int nbits{0};
  };

  void operator()(const Packet &pkt, ByteContainer *buf) const {
    buf->clear();
    const PHV *phv = pkt.get_phv();
    Deparse visitor(*phv, buf);
    std::for_each(entries.begin(), entries.end(),
                  boost::apply_visitor(visitor));
    if (with_payload) {
      size_t curr = buf->size();
      size_t psize = pkt.get_data_size();
      buf->resize(curr + psize);
      std::copy(pkt.data(), pkt.data() + psize, buf->begin() + curr);
    }
  }
};


template <typename T,
          typename std::enable_if<std::is_unsigned<T>::value, int>::type = 0>
class Calculation_ {
 public:
  Calculation_(const BufBuilder &builder,
               std::unique_ptr<RawCalculationIface<T> > c)
    : builder(builder), c(std::move(c)) { }

  T output(const Packet &pkt) const {
    static thread_local ByteContainer key;
    builder(pkt, &key);
    return c->output(key.data(), key.size());
  }

  RawCalculationIface<T> *get_raw_calculation() { return c.get(); }

 protected:
  ~Calculation_() { }

 private:
  BufBuilder builder;
  std::unique_ptr<RawCalculationIface<T> > c;
};


class CalculationsMap {
 public:
  typedef RawCalculationIface<uint64_t> MyC;

  static CalculationsMap *get_instance();
  bool register_one(const char *name, std::unique_ptr<MyC> c);
  std::unique_ptr<MyC> get_copy(const std::string &name);

 private:
  std::unordered_map<std::string, std::unique_ptr<MyC> > map_{};
};


class Calculation : public Calculation_<uint64_t> {
 public:
  Calculation(const BufBuilder &builder,
              std::unique_ptr<RawCalculationIface<uint64_t> > c)
    : Calculation_(builder, std::move(c)) { }

  Calculation(const BufBuilder &builder, const std::string &hash_name)
    : Calculation_(
        builder, CalculationsMap::get_instance()->get_copy(hash_name)
      ) { }
};


class NamedCalculation : public NamedP4Object, public Calculation_<uint64_t> {
 public:
  NamedCalculation(const std::string &name, p4object_id_t id,
                   const BufBuilder &builder,
                   std::unique_ptr<RawCalculationIface<uint64_t> > c)
    : NamedP4Object(name, id),
      Calculation_(builder, std::move(c)) { }

  NamedCalculation(const std::string &name, p4object_id_t id,
                   const BufBuilder &builder, const std::string &hash_name)
    : NamedP4Object(name, id),
      Calculation_(
        builder, CalculationsMap::get_instance()->get_copy(hash_name)
      ) { }
};


//! When implementing an hash operation for a target, this macro needs to be
//! called to make this module aware of the hash existence.
//! When calling this macro from an anonymous namespace, some compiler may give
//! you a warning.
#define REGISTER_HASH(hash_name)                                        \
  bool hash_name##_create_ =                                            \
      bm::CalculationsMap::get_instance()->register_one(                \
          #hash_name,                                                   \
          std::unique_ptr<bm::CalculationsMap::MyC>(                    \
              new bm::RawCalculation<uint64_t, hash_name>()));


namespace hash {

uint64_t xxh64(const char *buffer, size_t s);

}  // namespace hash


enum class CustomCrcErrorCode {
  SUCCESS = 0,
  INVALID_CALCULATION_NAME,
  WRONG_TYPE_CALCULATION,
  INVALID_CONFIG,
};

// can be used for crc16, with T == uint16_t
// can be used for crc32, with T uint32_t
template <typename T>
class CustomCrcMgr {
 public:
  struct crc_config_t {
    T polynomial;
    T initial_remainder;
    T final_xor_value;
    bool data_reflected;
    bool remainder_reflected;

    friend std::ostream &operator<<(std::ostream &out, const crc_config_t &c) {
      out << "polynomial: " << c.polynomial << ", initial_remainder: "
          << c.initial_remainder << ", final_xor_value: " << c.final_xor_value
          << ", data_reflected: " << c.data_reflected
          << ", remainder_reflected: " << c.remainder_reflected;
      return out;
    }
  };

  static CustomCrcErrorCode update_config(NamedCalculation *calculation,
                                          const crc_config_t &config);

  static CustomCrcErrorCode update_config(RawCalculationIface<uint64_t> *c,
                                          const crc_config_t &config);
};

}  // namespace bm

#endif  // BM_BM_SIM_CALCULATIONS_H_
