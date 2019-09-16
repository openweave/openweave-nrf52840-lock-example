/*
 *
 *    Copyright (c) 2019 Google LLC.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef DEVICE_DISCOVERY_H
#define DEVICE_DISCOVERY_H

#include <stdint.h>
#include <stdbool.h>

#include "nrf_log.h"

#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/device-description/DeviceDescription.h>

class DeviceDiscovery
{
public:

    WEAVE_ERROR Init(void);

    void EnableUserSelectedMode(bool aEnable, uint16_t aTimeOutMs = 0);
    void SetDeviceDiscoveredNodeId(uint64_t aNodeId);

    bool IsPeerDeviceDiscovered(void);
    bool GetDiscoveredDeviceNodeId(uint64_t & aNodeId);

    WEAVE_ERROR SearchForUserSelectedModeDevices(uint16_t aVendorId, uint16_t aProductId);

private:

    friend DeviceDiscovery & DeviceDiscoveryFeature(void);
    uint64_t mDiscoveredNodeId;

    static void HandleIdentifyResponseReceived(void *apAppState,
                                               uint64_t aNodeId,
                                               const nl::Inet::IPAddress& aNodeAddr,
                                               const nl::Weave::Profiles::DeviceDescription::IdentifyResponseMessage& aRespMsg);


    static DeviceDiscovery sDeviceDiscoveryFeature;

};

inline DeviceDiscovery & DeviceDiscoveryFeature(void)
{
    return DeviceDiscovery::sDeviceDiscoveryFeature;
}

#endif // DEVICE_DISCOVERY_H