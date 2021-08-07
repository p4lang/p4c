/* Copyright 2013-present Barefoot Networks, Inc.
 * Copyright 2021 VMware, Inc.
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
 * Antonin Bas
 *
 */

#ifndef BM_BM_SIM_HANDLE_MGR_H_
#define BM_BM_SIM_HANDLE_MGR_H_

#include <bm/config.h>

#include <algorithm>  // for swap
#include <type_traits>
#include <utility>

#include "dynamic_bitset.h"

namespace bm {

using handle_t = uintptr_t;

class HandleMgr {
 public:
  using bitset = DynamicBitset;
  using size_type = bitset::size_type;
  static constexpr size_type npos = bitset::npos;

  // iterators: since they are very simple, I did not mind duplicating the
  // code. Maybe it would be a good idea to condition the const'ness of the
  // iterator with a boolean template, to unify the 2 iterators.

  class const_iterator;

  class iterator {
    friend class const_iterator;

   public:
    iterator(HandleMgr *handle_mgr, handle_t index)
      : handle_mgr(handle_mgr), index(index) {}

    handle_t &operator*() {return index;}
    handle_t *operator->() {return &index;}

    bool operator==(const iterator &other) const {
      return (handle_mgr == other.handle_mgr) && (index == other.index);
    }

    bool operator!=(const iterator &other) const {
      return !(*this == other);
    }

    iterator& operator++() {
      auto pos = static_cast<size_type>(index);
      pos = handle_mgr->handles.find_next(pos);
      if (pos == npos)
        index = -1;
      else
        index = static_cast<handle_t>(pos);
      return *this;
    }

    iterator operator++(int) {
      // Use operator++()
      const iterator old(*this);
      ++(*this);
      return old;
    }

   private:
    HandleMgr *handle_mgr;
    handle_t index;
  };

  class const_iterator {
   public:
    const_iterator(const HandleMgr *handle_mgr, handle_t index)
      : handle_mgr(handle_mgr), index(index) {}

    const_iterator(const iterator &other)  // NOLINT(runtime/explicit)
      : handle_mgr(other.handle_mgr), index(other.index) {}

    const handle_t &operator*() const {return index;}
    const handle_t *operator->() const {return &index;}

    const_iterator& operator=(const const_iterator &other) {
      handle_mgr = other.handle_mgr;
      index = other.index;
      return *this;
    }

    bool operator==(const const_iterator &other) const {
      return (handle_mgr == other.handle_mgr) && (index == other.index);
    }

    bool operator!=(const const_iterator &other) const {
      return !(*this == other);
    }

    const const_iterator& operator++() {
      auto pos = static_cast<size_type>(index);
      pos = handle_mgr->handles.find_next(pos);
      if (pos == npos)
        index = -1;
      else
        index = static_cast<handle_t>(pos);
      return *this;
    }

    const const_iterator operator++(int) {
      // Use operator++()
      const const_iterator old(*this);
      ++(*this);
      return old;
    }

   private:
    const HandleMgr *handle_mgr;
    handle_t index;
  };


 public:
  bool operator==(const HandleMgr &other) const {
    return (handles == other.handles);
  }

  bool operator!=(const HandleMgr &other) const {
    return !(*this == other);
  }

  /* Return 0 on success, -1 on failure */

  int get_handle(handle_t *handle) {
    auto pos = first_unset;
    *handle = static_cast<handle_t>(pos);
    auto s = handles.size();
    if (pos < s) {
      handles.set(pos);
      first_unset = handles.find_unset_next(first_unset);
      first_unset = (first_unset == DynamicBitset::npos) ? s : first_unset;
    } else {
      assert(pos == s);
      first_unset++;
      handles.push_back(true);
    }
    return 0;
  }

  int release_handle(handle_t handle) {
    auto pos = static_cast<size_type>(handle);
    auto s = handles.size();
    if (pos >= s || !handles.reset(pos)) return -1;
    if (first_unset > pos) first_unset = pos;
    return 0;
  }

  int set_handle(handle_t handle) {
    auto pos = static_cast<size_type>(handle);
    auto s = handles.size();
    if (pos >= s)
      handles.resize(pos + 1);
    if (!handles.set(pos))
      return -1;
    if (first_unset == pos) {
      first_unset = handles.find_unset_next(pos);
      first_unset = (first_unset == DynamicBitset::npos) ? s : first_unset;
    }
    return 0;
  }

  size_t size() const {
    return static_cast<size_t>(handles.count());
  }

  bool valid_handle(handle_t handle) const {
    auto pos = static_cast<size_type>(handle);
    auto s = handles.size();
    if (pos >= s) return false;
    return handles.test(pos);
  }

  void clear() {
    handles.clear();
    first_unset = 0;
  }

  // iterators

  iterator begin() {
    auto pos = handles.find_first();
    if (pos == npos)
      return iterator(this, -1);
    else
      return iterator(this, static_cast<handle_t>(pos));
  }

  const_iterator begin() const {
    auto pos = handles.find_first();
    if (pos == npos)
      return const_iterator(this, -1);
    else
      return const_iterator(this, static_cast<handle_t>(pos));
  }

  iterator end() {
    return iterator(this, -1);
  }

  const_iterator end() const {
    return const_iterator(this, -1);
  }

 private:
  bitset handles;
  size_type first_unset{0};
};

}  // namespace bm

#endif  // BM_BM_SIM_HANDLE_MGR_H_
