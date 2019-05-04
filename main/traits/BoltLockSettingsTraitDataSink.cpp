/*
 *
 *    Copyright (c) 2016 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    This document is the property of Nest. It is considered
 *    confidential and proprietary information.
 *
 *    This document may not be reproduced or transmitted in any form,
 *    in whole or in part, without the express written permission of
 *    Nest.
 *
 *
 */

#include <traits/include/BoltLockSettingsTraitDataSink.h>
#include <schema/include/BoltLockSettingsTrait.h>

#include "nrf_log.h"

using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles::DataManagement;

using namespace Schema::Weave::Trait::Security;
using namespace Schema::Weave::Trait::Security::BoltLockSettingsTrait;

BoltLockSettingsTraitDataSink::BoltLockSettingsTraitDataSink()
    : TraitDataSink(&BoltLockSettingsTrait::TraitSchema)
{
    mAutoRelockOn = false;
    mAutoLockDurationSecs = 0;
}

WEAVE_ERROR
BoltLockSettingsTraitDataSink::SetLeafData(PropertyPathHandle aLeafHandle, TLVReader &aReader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    switch (aLeafHandle)
    {
        case BoltLockSettingsTrait::kPropertyHandle_AutoRelockOn:
            err = aReader.Get(mAutoRelockOn);
            nlREQUIRE_SUCCESS(err, exit);

            NRF_LOG_INFO("Auto Relock On Setting Value: %d", mAutoRelockOn);

            // Print Auto Relock State
            break;

        case BoltLockSettingsTrait::kPropertyHandle_AutoRelockDuration:
            err = aReader.Get(mAutoLockDurationSecs);
            nlREQUIRE_SUCCESS(err, exit);

            // Print Auto Lock Duration
            NRF_LOG_INFO("Auto Relock On Duration Setting Value: %d", mAutoLockDurationSecs);
            break;

        default:
            break;
    }

exit:
    return err;
}
