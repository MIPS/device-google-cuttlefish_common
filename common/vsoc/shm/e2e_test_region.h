#pragma once
/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstdint>
#include "common/vsoc/shm/base.h"
#include "common/vsoc/shm/version.h"

// Memory layout for a region that supports end-to-end (E2E) testing of
// shared memory regions. This verifies that all sorts of things work along the
// the path:
//
//   host libraries <-> ivshmem server <-> kernel <-> guest libraries
//
// This is intentionally not a unit test. The primary source of errors along
// this path is a misunderstanding and/or inconsistency in one of the
// interfaces. Introducing mocks would allow these errors to go undetected.
// Another way of looking at it is that the mocks would end up being a
// a copy-and-paste job, making a series of change-detector tests.
//
// These tests are actually run on every device boot to verify that things are
// ok.

namespace vsoc {
namespace layout {
namespace e2e_test {

/**
 * Flags that are used to indicate test status. Some of the latter testing
 * stages rely on initializion that must be done on the peer.
 */
enum E2ETestStage {
  // No tests have passed
  E2E_STAGE_NONE = 0,
  // This side has finished writing its pattern to the region
  E2E_MEMORY_FILLED = 1,
  // This side has confirmed that it can see its peer's writes to the region
  E2E_PEER_MEMORY_READ = 2,
};

/**
 * Structure that grants permission to write in the region to either the guest
 * or the host. This size of these fields is arbitrary.
 */
struct E2EMemoryFill {
  static const std::size_t kOwnedFieldSize = 32;

  // The compiler must not attempt to optimize away reads and writes to the
  // shared memory window. This is pretty typical when dealing with devices
  // doing memory mapped I/O.
  volatile char host_writable[kOwnedFieldSize];
  volatile char guest_writable[kOwnedFieldSize];
};
ASSERT_SHM_COMPATIBLE(E2EMemoryFill, e2e_test);


/**
 * Structure that grants permission to write in the region to either the guest
 * or the host. This size of these fields is arbitrary.
 */
class E2ETestStageRegister {
 public:
  E2ETestStage value() const {
    E2ETestStage rval = static_cast<E2ETestStage>(value_);
    return rval;
  }

  void set_value(E2ETestStage new_value) {
    value_ = new_value;
  }

 protected:
  // The compiler must not attempt to optimize away reads and writes to the
  // shared memory window. This is pretty typical when dealing with devices
  // doing memory mapped I/O.
  volatile uint32_t value_;
};
ASSERT_SHM_COMPATIBLE(E2ETestStageRegister, e2e_test);

/**
 * Describes the layout of the regions used for the end-to-end test. There
 * are multiple regions: primary and secondary, so some details like the region
 * name must wait until later.
 */
struct E2ETestRegionBase {
  /**
   * Computes how many E2EMemoryFill records we need to cover the region.
   * Covering the entire region during the test ensures that everything is
   * mapped and coherent between guest and host.
   */
  static std::size_t NumFillRecords(std::size_t region_size) {
    if (region_size < sizeof(E2ETestRegionBase)) {
      return 0;
    }
    // 1 + ... An array of size 1 is allocated in the E2ETestRegion.
    // TODO(ghartman): AddressSanitizer may find this sort of thing to be
    // alarming.
    return 1 + (region_size - sizeof(E2ETestRegionBase)) /
        sizeof(E2EMemoryFill);
  }

  // The number of test stages that have completed on the guest
  // Later host tests will wait on this
  E2ETestStageRegister guest_status;
  // The number of test stages that have completed on the host
  // Later guest tests will wait on this
  E2ETestStageRegister host_status;
  // There rest of the region will be filled by guest_host_strings.
  // We actually use more than one of these, but we can't know how many
  // until we examine the region.
  E2EMemoryFill data[1];
};
ASSERT_SHM_COMPATIBLE(E2ETestRegionBase, e2e_test);

struct E2EPrimaryTestRegion : public E2ETestRegionBase {
  static const char* region_name;
  static const char guest_pattern[E2EMemoryFill::kOwnedFieldSize];
  static const char host_pattern[E2EMemoryFill::kOwnedFieldSize];
};
ASSERT_SHM_COMPATIBLE(E2EPrimaryTestRegion, e2e_test);

struct E2ESecondaryTestRegion : public E2ETestRegionBase {
  static const char* region_name;
  static const char guest_pattern[E2EMemoryFill::kOwnedFieldSize];
  static const char host_pattern[E2EMemoryFill::kOwnedFieldSize];
};
ASSERT_SHM_COMPATIBLE(E2ESecondaryTestRegion, e2e_test);

}  // e2e_test
}  // layout
}  // vsoc