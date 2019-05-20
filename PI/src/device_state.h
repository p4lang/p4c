/* Copyright 2019-present Barefoot Networks, Inc.
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

#ifndef SRC_DEVICE_STATE_H_
#define SRC_DEVICE_STATE_H_

#include <PI/p4info.h>

#include <memory>
#include <unordered_map>

#include "group_selection.h"

namespace pibmv2 {

struct DummyMutex {
  DummyMutex() = default;
  DummyMutex(const DummyMutex &) = delete;
  DummyMutex &operator=(const DummyMutex &) = delete;
  DummyMutex(DummyMutex &&) = delete;
  DummyMutex &operator=(DummyMutex &&) = delete;
};

template <typename T>
struct DummyLock {
  explicit DummyLock(const T &) { }
  // avoid "set but not used" warning when calling DeviceLock::unique_lock()
  ~DummyLock() { }
  DummyLock(const DummyLock &) = delete;
  DummyLock &operator=(const DummyLock &) = delete;
  DummyLock(DummyLock &&) = default;
  DummyLock &operator=(DummyLock &&) = default;
};

class DeviceLock {
 public:
  // TODO(antonin): the right thing to do would probably be to lock the device
  // properly but since this code is only really used by the P4Runtime server
  // implementation (p4lang/PI) which is thread safe (and ensures that no API
  // calls are made concurrently with pi_update_device_start), there is no real
  // need to lock the device again here.
  using Mutex = DummyMutex;
  using UniqueLock = DummyLock<Mutex>;
  using SharedLock = DummyLock<Mutex>;

  UniqueLock unique_lock() { return UniqueLock(mutex); }
  SharedLock shared_lock() { return SharedLock(mutex); }

 private:
  mutable DummyMutex mutex;
};

class DeviceState {
 public:
  explicit DeviceState(const pi_p4info_t *p4info);

  std::unordered_map<pi_p4_id_t, std::shared_ptr<GroupSelection> >
  act_prof_selection;
};

extern DeviceLock *device_lock;
extern DeviceState *device_state;

}  // namespace pibmv2

#endif  // SRC_DEVICE_STATE_H_
