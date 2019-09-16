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

#include <DeviceDiscovery.h>

#include "AppEvent.h"
#include "AppTask.h"

#include "nrf_log.h"

#include <Weave/DeviceLayer/WeaveDeviceLayer.h>

using namespace ::nl::Weave;
using namespace ::nl::Weave::DeviceLayer;
using namespace ::nl::Weave::Profiles::DeviceDescription;

static DeviceDescriptionClient sDeviceDescriptionClient;
DeviceDiscovery DeviceDiscovery::sDeviceDiscoveryFeature;

WEAVE_ERROR DeviceDiscovery::Init(void)
{
	WEAVE_ERROR err = WEAVE_NO_ERROR;

    mDiscoveredNodeId = 0;

#if SERVICELESS_DEVICE_DISCOVERY_ENABLED
    err = sDeviceDescriptionClient.Init(&ExchangeMgr);
    SuccessOrExit(err);

    sDeviceDescriptionClient.OnIdentifyResponseReceived = HandleIdentifyResponseReceived;

exit:
#endif
    return err;
}

bool DeviceDiscovery::IsPeerDeviceDiscovered(void)
{
	return mDiscoveredNodeId != 0 ? true:false;
}

void DeviceDiscovery::SetDeviceDiscoveredNodeId(uint64_t aNodeId)
{
	mDiscoveredNodeId = aNodeId;
}

bool DeviceDiscovery::GetDiscoveredDeviceNodeId(uint64_t & aNodeId)
{
	bool device_discovered = false;

	if (IsPeerDeviceDiscovered())
	{
		aNodeId = mDiscoveredNodeId;
		device_discovered = true;
	}

	return device_discovered;
}

#if SERVICELESS_DEVICE_DISCOVERY_ENABLED
void DeviceDiscovery::EnableUserSelectedMode(bool aEnable, uint16_t aTimeOutMs)
{
    if (aTimeOutMs != 0)
    {
        ConnectivityMgr().SetUserSelectedModeTimeout(aTimeOutMs);
    }

    ConnectivityMgr().SetUserSelectedMode(aEnable);
}

WEAVE_ERROR DeviceDiscovery::SearchForUserSelectedModeDevices(uint16_t aVendorId, uint16_t aProductId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    IdentifyRequestMessage identifyReqMsg;
    nl::Inet::IPAddress ip_addr;

    if (!ConfigurationMgr().IsMemberOfFabric())
    {
        NRF_LOG_INFO("DeviceDiscovery err: Device not fabric provisioned");
    	err = WEAVE_ERROR_INCORRECT_STATE;
    }
    SuccessOrExit(err);

    ip_addr = nl::Inet::IPAddress::MakeIPv6WellKnownMulticast(nl::Inet::kIPv6MulticastScope_Link,
                                                              nl::Inet::kIPV6MulticastGroup_AllNodes);

    identifyReqMsg.TargetFabricId   = ::nl::Weave::DeviceLayer::FabricState.FabricId;
    identifyReqMsg.TargetModes      = kTargetDeviceMode_UserSelectedMode;
    identifyReqMsg.TargetVendorId   = aVendorId;
    identifyReqMsg.TargetProductId  = aProductId;
    identifyReqMsg.TargetDeviceId   = nl::Weave::kAnyNodeId;

    err = sDeviceDescriptionClient.SendIdentifyRequest(ip_addr, identifyReqMsg);
    SuccessOrExit(err);

exit:
    return err;
}

void DeviceDiscovery::HandleIdentifyResponseReceived(void *apAppState,
                                             		 uint64_t aNodeId,
                                             		 const nl::Inet::IPAddress& aNodeAddr,
                                             		 const IdentifyResponseMessage& aRespMsg)
{
    WeaveDeviceDescriptor deviceDesc = aRespMsg.DeviceDesc;

    NRF_LOG_INFO("IdentifyResponse received from node 0x%016" PRIX64, aNodeId);
    NRF_LOG_INFO("  Source Fabric Id: %016" PRIX64, deviceDesc.FabricId);
    NRF_LOG_INFO("  Source Vendor Id: %04X", (unsigned)deviceDesc.VendorId);
    NRF_LOG_INFO("  Source Product Id: %04X", (unsigned)deviceDesc.ProductId);
    NRF_LOG_INFO("  Source Product Revision: %04X", (unsigned)deviceDesc.ProductRevision);

    NRF_LOG_INFO("Device Description Operation Completed");

    sDeviceDescriptionClient.CancelExchange();

    sDeviceDiscoveryFeature.mDiscoveredNodeId = aNodeId;

    AppEvent event;
    event.Type      = AppEvent::kEventType_LocalDeviceDiscovered;
    event.Handler   = NULL;
    GetAppTask().PostEvent(&event);
}
#endif // SERVICELESS_DEVICE_DISCOVERY_ENABLED