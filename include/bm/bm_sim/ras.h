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

#ifndef BM_BM_SIM_RAS_H_
#define BM_BM_SIM_RAS_H_

#include <algorithm>  // for std::swap

#include <cassert>

#include <Judy.h>

namespace bm {

class RandAccessUIntSet {
 public:
  typedef uintptr_t mbr_t;

 public:
  /* this code is basically copied from handle_mgr.h, maybe I should just write
     a wrapper for Judy once and for all */

  class const_iterator;

  class iterator {
    friend class const_iterator;

   public:
    iterator(RandAccessUIntSet *ras_mgr, mbr_t index)
      : ras_mgr(ras_mgr), index(index) { }

    mbr_t &operator*() {return index;}
    mbr_t *operator->() {return &index;}

    bool operator==(const iterator &other) const {
      return (ras_mgr == other.ras_mgr) && (index == other.index);
    }

    bool operator!=(const iterator &other) const {
      return !(*this == other);
    }

    iterator& operator++() {
      int Rc_int;
      Word_t jindex = index;;
      J1N(Rc_int, ras_mgr->members, jindex);
      if (!Rc_int)
        index = -1;
      else
        index = jindex;
      return *this;
    }

    iterator operator++(int) {
      // Use operator++()
      const iterator old(*this);
      ++(*this);
      return old;
    }

   private:
    RandAccessUIntSet *ras_mgr;
    mbr_t index;
  };

  class const_iterator {
   public:
    const_iterator(const RandAccessUIntSet *ras_mgr, mbr_t index)
      : ras_mgr(ras_mgr), index(index) { }

    explicit const_iterator(const iterator &other)
      : ras_mgr(other.ras_mgr), index(other.index) { }

    const mbr_t &operator*() const {return index;}
    const mbr_t *operator->() const {return &index;}

    const_iterator& operator=(const const_iterator &other) {
      ras_mgr = other.ras_mgr;
      index = other.index;
      return *this;
    }

    bool operator==(const const_iterator &other) const {
      return (ras_mgr == other.ras_mgr) && (index == other.index);
    }

    bool operator!=(const const_iterator &other) const {
      return !(*this == other);
    }

    const const_iterator& operator++() {
      int Rc_int;
      Word_t jindex = index;
      J1N(Rc_int, ras_mgr->members, jindex);
      if (!Rc_int)
        index = -1;
      else
        index = jindex;
      return *this;
    }

    const const_iterator operator++(int) {
      // Use operator++()
      const const_iterator old(*this);
      ++(*this);
      return old;
    }

   private:
    const RandAccessUIntSet *ras_mgr;
    mbr_t index;
  };


 public:
  RandAccessUIntSet()
    : members((Pvoid_t) NULL) { }

  ~RandAccessUIntSet() {
    Word_t bytes_freed;
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
#endif
    J1FA(bytes_freed, members);
#ifdef __clang__
#pragma clang diagnostic pop
#endif
  }

  RandAccessUIntSet(const RandAccessUIntSet &other) = delete;
  RandAccessUIntSet &operator=(const RandAccessUIntSet &other) = delete;

  RandAccessUIntSet(RandAccessUIntSet &&other)
    : members(other.members) {
    other.members = nullptr;
  }

  RandAccessUIntSet &operator=(RandAccessUIntSet &&other) {
    std::swap(members, other.members);
    return *this;
  }

  // returns 0 if already present (0 element added), 1 otherwise
  int add(mbr_t mbr) {
    int Rc;
    J1S(Rc, members, mbr);
    return Rc;
  }

  // returns 0 if not present (0 element removed), 1 otherwise
  int remove(mbr_t mbr) {
    int Rc;
    J1U(Rc, members, mbr);
    return Rc;
  }

  bool contains(mbr_t mbr) const {
    int Rc;
    J1T(Rc, members, mbr);
    return Rc;
  }

  size_t count() const {
    size_t nb;
    J1C(nb, members, 0, -1);
    return nb;
  }

  // n >= 0, 0 is first element in set
  mbr_t get_nth(size_t n) const {
    assert(n <= count());
    int Rc;
    Word_t mbr;
    J1BC(Rc, members, n + 1, mbr);
    assert(Rc == 1);
    return static_cast<mbr_t>(mbr);
  }

  // iterators

  iterator begin() {
    Word_t index = 0;
    int Rc_int;
    J1F(Rc_int, members, index);
    if (!Rc_int) index = -1;
    return iterator(this, index);
  }

  const_iterator begin() const {
    Word_t index = 0;
    int Rc_int;
    J1F(Rc_int, members, index);
    if (!Rc_int) index = -1;
    return const_iterator(this, index);
  }

  iterator end() {
    Word_t index = -1;
    return iterator(this, index);
  }

  const_iterator end() const {
    Word_t index = -1;
    return const_iterator(this, index);
  }

 private:
  Pvoid_t members;
};

}  // namespace bm

#endif  // BM_BM_SIM_RAS_H_
