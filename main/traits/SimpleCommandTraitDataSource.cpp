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

#include <traits/include/SimpleCommandTraitDataSource.h>
#include <schema/include/SimpleCommandTrait.h>
#include "nrf_log.h"
#include <WDMFeature.h>
#include <AppTask.h>

#include <Weave/DeviceLayer/WeaveDeviceLayer.h>
#include <Weave/Support/TraitEventUtils.h>

using namespace nl::Weave;
using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles::DataManagement;
using namespace Schema::Nest::Test::Trait;

SimpleCommandTraitDataSource::SimpleCommandTraitDataSource() : TraitDataSource(&SimpleCommandTrait::TraitSchema)
{
}

WEAVE_ERROR SimpleCommandTraitDataSource::GetLeafData(PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, TLVWriter & aWriter)
{
    return WEAVE_ERROR_INVALID_ARGUMENT;
}

void SimpleCommandTraitDataSource::OnCustomCommand(nl::Weave::Profiles::DataManagement::Command * aCommand,
                                              const nl::Weave::WeaveMessageInfo * aMsgInfo, nl::Weave::PacketBuffer * aPayload,
                                              const uint64_t & aCommandType, const bool aIsExpiryTimeValid,
                                              const int64_t & aExpiryTimeMicroSecond, const bool aIsMustBeVersionValid,
                                              const uint64_t & aMustBeVersion, nl::Weave::TLV::TLVReader & aArgumentReader)
{
    WEAVE_ERROR err           = WEAVE_NO_ERROR;
    uint32_t reportProfileId  = nl::Weave::Profiles::kWeaveProfile_Common;
    uint16_t reportStatusCode = nl::Weave::Profiles::Common::kStatus_BadRequest;

    if (aIsMustBeVersionValid)
    {
        if (aMustBeVersion != GetVersion())
        {
            NRF_LOG_INFO("Actual version is 0x%X, while must-be version is: 0x%" PRIx64, GetVersion(), aMustBeVersion);
            reportProfileId  = nl::Weave::Profiles::kWeaveProfile_WDM;
            reportStatusCode = kStatus_VersionMismatch;
            goto exit;
        }
    }

    if (aIsExpiryTimeValid)
    {
        reportProfileId = nl::Weave::Profiles::kWeaveProfile_WDM;
#if WEAVE_DEVICE_CONFIG_ENABLE_WEAVE_TIME_SERVICE_TIME_SYNC
        uint64_t currentTime;
        err = System::Platform::Layer::GetClock_RealTimeMS(currentTime);
        if (err == WEAVE_SYSTEM_ERROR_REAL_TIME_NOT_SYNCED)
        {
            NRF_LOG_INFO("SimpleCommandRequest failed!");
            reportStatusCode = kStatus_NotTimeSyncedYet;
            goto exit;
        }

        // If we have already passed the commands expiration time,
        // error out
        if (aExpiryTimeMicroSecond < static_cast<int64_t>(currentTime))
        {
            NRF_LOG_INFO("SimpleCommandRequest Expired!");
            reportStatusCode = kStatus_RequestExpiredInTime;
            goto exit;
        }

#else
        reportStatusCode = kStatus_ExpiryTimeNotSupported;
        goto exit;
#endif
    }

    VerifyOrExit(aCommandType == SimpleCommandTrait::kSimpleCommandRequestId, err = WEAVE_ERROR_NOT_IMPLEMENTED);

    NRF_LOG_INFO("SimpleCommandRequest received");

    PacketBuffer::Free(aPayload);
    aPayload = NULL;

    // Generate a success response right here.
    if (err == WEAVE_NO_ERROR)
    {
        PacketBuffer * msgBuf = PacketBuffer::New();
        if (NULL == msgBuf)
        {
            reportProfileId  = nl::Weave::Profiles::kWeaveProfile_Common;
            reportStatusCode = nl::Weave::Profiles::Common::kStatus_OutOfMemory;
            ExitNow(err = WEAVE_ERROR_NO_MEMORY);
        }

        NRF_LOG_INFO("Sending Success Response to SimpleCommandRequest");
        aCommand->SendResponse(GetVersion(), msgBuf);
        aCommand = NULL;
        msgBuf   = NULL;
    }
    else
    {
        NRF_LOG_INFO("SimpleCommandRequest Command Error : %d", err);
    }

exit:
    if (NULL != aCommand)
    {
        aCommand->SendError(reportProfileId, reportStatusCode, err);
        aCommand = NULL;
    }

    if (aPayload)
    {
        PacketBuffer::Free(aPayload);
        aPayload = NULL;
    }
}
