/*
 * Copyright (C) 2017 The Android Open Source Project
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
#pragma once
#include <linux/if_ether.h>

#include "common/vsoc/shm/base.h"
#include "common/vsoc/shm/circqueue.h"
#include "common/vsoc/shm/lock.h"
#include "common/vsoc/shm/version.h"

// Memory layout for wifi packet exchange region.
namespace vsoc {
namespace layout {
namespace wifi {

struct WifiExchangeLayout : public RegionLayout {
  // Traffic originating from host that proceeds towards guest.
  CircularPacketQueue<16, 8192> guest_ingress;
  // Traffic originating from guest that proceeds towards host.
  CircularPacketQueue<16, 8192> guest_egress;

  // config_lock_ manages access to configuration section
  SpinLock config_lock_;
  // config_ready_ indicates whether config section is ready to be accessed.
  bool config_ready_;
  // Desired MAC address for guest device.
  uint8_t mac_address[ETH_ALEN];

  static const char* region_name;
};

ASSERT_SHM_COMPATIBLE(WifiExchangeLayout, wifi);

}  // namespace wifi
}  // namespace layout
}  // namespace vsoc
