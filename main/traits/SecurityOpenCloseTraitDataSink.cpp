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

#include "SecurityOpenCloseTraitDataSink.h"
#include "BoltLockManager.h"

#include "nrf_log.h"

#include <nest/trait/detector/OpenCloseTrait.h>
#include <nest/trait/security/SecurityOpenCloseTrait.h>

#include <Weave/DeviceLayer/WeaveDeviceLayer.h>
#include <Weave/Support/TraitEventUtils.h>

using namespace nl::Weave;
using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles::DataManagement;
using namespace Schema::Nest::Trait::Security;
using namespace Schema::Nest::Trait::Detector::OpenCloseTrait;
using namespace Schema::Nest::Trait::Security::SecurityOpenCloseTrait;

SecurityOpenCloseTraitDataSink::SecurityOpenCloseTraitDataSink()
    : TraitDataSink(&SecurityOpenCloseTrait::TraitSchema)
{
}

bool SecurityOpenCloseTraitDataSink::IsOpen(void)
{
    return mSecurityOpenCloseState == OPEN_CLOSE_STATE_CLOSED ? false : true;
}

WEAVE_ERROR SecurityOpenCloseTraitDataSink::SetLeafData(PropertyPathHandle aLeafHandle, TLVReader & aReader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    switch (aLeafHandle)
    {
        case SecurityOpenCloseTrait::kPropertyHandle_OpenCloseState:
        {
            int32_t open_close_state;

            err = aReader.Get(open_close_state);
            SuccessOrExit(err);

            mSecurityOpenCloseState = open_close_state;

            BoltLockMgr().EvaluateChange();

            NRF_LOG_INFO("SecurityOpenCloseTrait -> Open Close State: %d", open_close_state);
            break;
        }
        case SecurityOpenCloseTrait::kPropertyHandle_BypassRequested:
        {
            /* Bypass is not a supported feature in this example */
            bool bypass_requested;
            err = aReader.Get(bypass_requested);
            SuccessOrExit(err);
            NRF_LOG_INFO("SecurityOpenCloseTrait -> Bypass Requested: %d", bypass_requested);
            break;
        }

        case SecurityOpenCloseTrait::kPropertyHandle_FirstObservedAtMs:
        {
            uint64_t currentTime = 0;
            err = aReader.Get(currentTime);
            NRF_LOG_INFO("SecurityOpenCloseTrait -> First Observed At (ms): %llu", currentTime);
            SuccessOrExit(err);
            break;
        }

        default:
            NRF_LOG_INFO("SecurityOpenCloseTrait::Unexpected Leaf");
            break;
    }

exit:
    return err;
}