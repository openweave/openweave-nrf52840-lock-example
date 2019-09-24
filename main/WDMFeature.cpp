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

#include "WDMFeature.h"

#include "nrf_log.h"
#include "nrf_error.h"

#include <Weave/DeviceLayer/WeaveDeviceLayer.h>

using namespace ::nl;
using namespace ::nl::Inet;
using namespace ::nl::Weave;
using namespace ::nl::Weave::DeviceLayer;
using namespace ::nl::Weave::DeviceLayer::Internal;
using namespace ::nl::Weave::Profiles::DataManagement_Current;

/** Defines the timeout for a response to any message initiated by the device to a WDM subscriber.
 *  This includes notifies, subscribe confirms, cancels and updates.
 */
#define INBOUND_SUBSCRIPTION_MESSAGE_RESPONSE_TIMEOUT_MS 10000

/** Defines the timeout for a message to get retransmitted when no wrm ack or
 *  response has been heard back from a WDM subscriber.
 */
#define INBOUND_SUBSCRIPTION_WRM_INITIAL_RETRANS_TIMEOUT_MS 2500

#define INBOUND_SUBSCRIPTION_WRM_ACTIVE_RETRANS_TIMEOUT_MS 2500

/** Define the maximum number of retransmissions in WRM
 */
#define INBOUND_SUBSCRIPTION_WRM_MAX_RETRANS 3

/** Define the timeout for piggybacking a WRM acknowledgment message
 */
#define INBOUND_SUBSCRIPTION_WRM_PIGGYBACK_ACK_TIMEOUT_MS 200

const nl::Weave::WRMPConfig gWRMPConfigInboundSubscription = { INBOUND_SUBSCRIPTION_WRM_INITIAL_RETRANS_TIMEOUT_MS, INBOUND_SUBSCRIPTION_WRM_ACTIVE_RETRANS_TIMEOUT_MS,
                                                   INBOUND_SUBSCRIPTION_WRM_PIGGYBACK_ACK_TIMEOUT_MS, INBOUND_SUBSCRIPTION_WRM_MAX_RETRANS };

WDMFeature WDMFeature::sWDMfeature;

SubscriptionEngine * SubscriptionEngine::GetInstance()
{
    return &(WdmFeature().mSubscriptionEngine);
}

int PublisherLock::Init()
{
    mRecursiveLock = xSemaphoreCreateRecursiveMutex();
    return ((mRecursiveLock == NULL) ? NRF_ERROR_NULL : NRF_SUCCESS);
}

WEAVE_ERROR PublisherLock::Lock()
{
    if (pdTRUE != xSemaphoreTakeRecursive((SemaphoreHandle_t) mRecursiveLock, portMAX_DELAY))
    {
        return WEAVE_ERROR_LOCKING_FAILURE;
    }

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR PublisherLock::Unlock()
{
    if (pdTRUE != xSemaphoreGiveRecursive((SemaphoreHandle_t) mRecursiveLock))
    {
        return WEAVE_ERROR_LOCKING_FAILURE;
    }

    return WEAVE_NO_ERROR;
}

WDMFeature::WDMFeature(void)
    : mTraitSourceCatalog(ResourceIdentifier(ResourceIdentifier::SELF_NODE_ID),
                          mTraitSourceCatalogStore, ArraySize(mTraitSourceCatalogStore))
{
}

void WDMFeature::AsyncProcessChanges(intptr_t arg)
{
    sWDMfeature.mSubscriptionEngine.GetNotificationEngine()->Run();
}

void WDMFeature::ProcessTraitChanges(void)
{
    PlatformMgr().ScheduleWork(AsyncProcessChanges);
}

void WDMFeature::HandleSubscriptionEngineEvent(void * appState, SubscriptionEngine::EventID eventType,
                                               const SubscriptionEngine::InEventParam & inParam,
                                               SubscriptionEngine::OutEventParam & outParam)
{
    switch (eventType)
    {
        case SubscriptionEngine::kEvent_OnIncomingSubscribeRequest:
            outParam.mIncomingSubscribeRequest.mHandlerEventCallback = HandleInboundSubscriptionEvent;
            outParam.mIncomingSubscribeRequest.mHandlerAppState      = NULL;
            outParam.mIncomingSubscribeRequest.mRejectRequest        = false;
            break;

        default:
            SubscriptionEngine::DefaultEventHandler(eventType, inParam, outParam);
            break;
    }
}

void WDMFeature::HandleInboundSubscriptionEvent(void * aAppState, SubscriptionHandler::EventID eventType,
                                                const SubscriptionHandler::InEventParam & inParam,
                                                SubscriptionHandler::OutEventParam & outParam)
{
    switch (eventType)
    {
        case SubscriptionHandler::kEvent_OnSubscribeRequestParsed:
        {
            Binding * binding = inParam.mSubscribeRequestParsed.mHandler->GetBinding();

            binding->SetDefaultResponseTimeout(INBOUND_SUBSCRIPTION_MESSAGE_RESPONSE_TIMEOUT_MS);
            binding->SetDefaultWRMPConfig(gWRMPConfigInboundSubscription);

            inParam.mSubscribeRequestParsed.mHandler->AcceptSubscribeRequest(inParam.mSubscribeRequestParsed.mTimeoutSecMin);
            break;
        }

        case SubscriptionHandler::kEvent_OnSubscriptionEstablished:
        {
            break;
        }

        case SubscriptionHandler::kEvent_OnSubscriptionTerminated:
        {
            const char * termDesc = (inParam.mSubscriptionTerminated.mReason == WEAVE_ERROR_STATUS_REPORT_RECEIVED)
                ? StatusReportStr(inParam.mSubscriptionTerminated.mStatusProfileId, inParam.mSubscriptionTerminated.mStatusCode)
                : ErrorStr(inParam.mSubscriptionTerminated.mReason);

            NRF_LOG_INFO("Inbound terminated: %s", termDesc);
            break;
        }

        default:
            SubscriptionHandler::DefaultEventHandler(eventType, inParam, outParam);
            break;
    }
}

void WDMFeature::PlatformEventHandler(const WeaveDeviceEvent * event, intptr_t arg)
{
}

WEAVE_ERROR WDMFeature::Init()
{
    WEAVE_ERROR err;
    int ret;
    Binding * binding;

    ret = mPublisherLock.Init();
    VerifyOrExit(ret != NRF_ERROR_NULL, err = WEAVE_ERROR_NO_MEMORY);

    PlatformMgr().AddEventHandler(PlatformEventHandler);

    mTraitSourceCatalog.AddAt(0, &mSimpleCommandTraitSource, kSourceHandle_SimpleCommandTrait);

    err = mSubscriptionEngine.Init(&ExchangeMgr, this, HandleSubscriptionEngineEvent);
    SuccessOrExit(err);

    err = mSubscriptionEngine.EnablePublisher(&mPublisherLock, &mTraitSourceCatalog);
    SuccessOrExit(err);

    NRF_LOG_INFO("WDMFeature Init Complete");

exit:
    return err;
}
