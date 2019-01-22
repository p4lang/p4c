/* Copyright 2018 Boon Thau Loo
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

#include <bm/bm_sim/periodic_task.h>

#include <chrono>
#include <functional>

using namespace bm;

class PeriodicIncrementor {
 public:
  PeriodicIncrementor(const std::chrono::milliseconds &interval, int *i)
      : i(i), increment_task("increment",
                             std::bind(&PeriodicIncrementor::increment, this),
                             interval) {}

  void increment() {
    (*i)++;
  }
  int *i;

 private:
  PeriodicTask increment_task;
};

static constexpr int kSleepTimeMs = 500;
static constexpr int kPeriods = 5;

static constexpr std::chrono::milliseconds kInterval{kSleepTimeMs / kPeriods};
static constexpr std::chrono::milliseconds kSleepTime{kSleepTimeMs};

// Tasks registered before the start of the thread execute on start
TEST(PeriodicExtern, PreStartRegistration) {
  int i = 0;
  PeriodicIncrementor incrementor(kInterval, &i);
  std::this_thread::sleep_for(kSleepTime);

  EXPECT_EQ(i, 0);

  PeriodicTaskList::get_instance().start();
  std::this_thread::sleep_for(kSleepTime);
  PeriodicTaskList::get_instance().join();

  // One more or one less execution is o.k.
  EXPECT_GE(i, kPeriods - 1);
  EXPECT_LE(i, kPeriods + 1);
}

// Tasks registered after the start of the thread still execute
TEST(PeriodicExtern, PostStartRegistration) {
  int i = 0;
  PeriodicTaskList::get_instance().start();

  PeriodicIncrementor incrementor(kInterval, &i);

  std::this_thread::sleep_for(kSleepTime);
  PeriodicTaskList::get_instance().join();

  EXPECT_GE(i, kPeriods - 1);
  EXPECT_LE(i, kPeriods + 1);
}

// Tasks stop executing when they go out of scope
TEST(PeriodicExtern, Unregistration) {
  int i = 0;
  PeriodicTaskList::get_instance().start();
  {
    PeriodicIncrementor incrementor(kInterval, &i);
    std::this_thread::sleep_for(kSleepTime);
  }
  std::this_thread::sleep_for(kSleepTime);

  EXPECT_GE(i, kPeriods - 1);
  EXPECT_LE(i, kPeriods + 1);
}

// Two tasks will be scheduled correctly
TEST(PeriodicExtern, MultipleTasks) {
  int i = 0;
  int j = 0;
  std::chrono::milliseconds interval_2{kSleepTimeMs / (kPeriods * 2)};
  PeriodicTaskList::get_instance().start();
  {
      PeriodicIncrementor incrementor_1(kInterval, &i);
      PeriodicIncrementor incrementor_2(interval_2, &j);
      std::this_thread::sleep_for(kSleepTime);
  }

  EXPECT_GE(i, kPeriods - 1);
  EXPECT_LE(i, kPeriods + 1);
  EXPECT_GE(j, (kPeriods * 2) - 1);
  EXPECT_LE(j, (kPeriods * 2) + 1);
}
