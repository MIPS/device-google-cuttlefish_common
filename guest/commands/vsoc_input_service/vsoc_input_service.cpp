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

#include "vsoc_input_service.h"

#include <linux/input.h>
#include <linux/uinput.h>

#include <thread>

#include "log/log.h"

#include "common/vsoc/lib/fb_bcast_region_view.h"
#include "common/vsoc/lib/input_events_region_view.h"

using vsoc::framebuffer::FBBroadcastRegionView;
using vsoc::input_events::InputEvent;
using vsoc::input_events::InputEventsRegionView;
using vsoc_input_service::VirtualDeviceBase;
using vsoc_input_service::VirtualKeyboard;
using vsoc_input_service::VirtualPowerButton;
using vsoc_input_service::VirtualTouchScreen;
using vsoc_input_service::VSoCInputService;

namespace {

template <class RegionView>
RegionView* GetRegionView() {
  static RegionView instance;
  return &instance;
}

void EventLoop(std::shared_ptr<VirtualDeviceBase> device,
               std::function<int(InputEvent*, int)> next_events) {
  while (1) {
    InputEvent events[InputEventsRegionView::kMaxEventsPerPacket];
    int count = next_events(events, InputEventsRegionView::kMaxEventsPerPacket);
    if (count <= 0) {
      SLOGE("Error getting events from the queue: Maybe check packet size");
      continue;
    }
    for (int i = 0; i < count; ++i) {
      device->EmitEvent(events[i].type, events[i].code, events[i].value);
    }
  }
}

}  // namespace

bool VSoCInputService::SetUpDevices() {
  virtual_power_button_.reset(new VirtualPowerButton());
  if (!virtual_power_button_->SetUp()) {
    return false;
  }
  virtual_keyboard_.reset(new VirtualKeyboard());
  if (!virtual_keyboard_->SetUp()) {
    return false;
  }

  FBBroadcastRegionView* fb_broadcast = GetRegionView<FBBroadcastRegionView>();
  if (!fb_broadcast->Open()) {
    SLOGE("Failed to open framebuffer broadcast region");
    return false;
  }

  virtual_touchscreen_.reset(
      new VirtualTouchScreen(fb_broadcast->x_res(), fb_broadcast->y_res()));
  if (!virtual_touchscreen_->SetUp()) {
    return false;
  }

  return true;
}

bool VSoCInputService::ProcessEvents() {
  InputEventsRegionView* input_events_rv =
      GetRegionView<InputEventsRegionView>();
  input_events_rv->Open();
  // TODO(jemoreira): Post available devices to region
  input_events_rv->StartWorker();

  // Start device threads
  std::thread screen_thread([this]() {
    EventLoop(
        virtual_touchscreen_, [](InputEvent* event_buffer, int max_events) {
          return GetRegionView<InputEventsRegionView>()->GetScreenEventsOrWait(
              event_buffer, max_events);
        });
  });
  std::thread keyboard_thread([this]() {
    EventLoop(virtual_keyboard_, [](InputEvent* event_buffer, int max_events) {
      return GetRegionView<InputEventsRegionView>()->GetKeyboardEventsOrWait(
          event_buffer, max_events);
    });
  });
  std::thread button_thread([this]() {
    EventLoop(virtual_power_button_, [](InputEvent* event_buffer,
                                        int max_events) {
      return GetRegionView<InputEventsRegionView>()->GetPowerButtonEventsOrWait(
          event_buffer, max_events);
    });
  });

  screen_thread.join();
  keyboard_thread.join();
  button_thread.join();

  // Should never return
  return false;
}
