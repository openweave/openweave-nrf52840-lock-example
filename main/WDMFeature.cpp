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
#include "DeviceDiscovery.h"
#include "BoltLockManager.h"

#include "nrf_log.h"
#include "nrf_error.h"

#include <Weave/DeviceLayer/WeaveDeviceLayer.h>

using namespace ::nl;
using namespace ::nl::Inet;
using namespace ::nl::Weave;
using namespace ::nl::Weave::DeviceLayer;
using namespace ::nl::Weave::DeviceLayer::Internal;
using namespace ::nl::Weave::Profiles::DataManagement_Current;

// TODO: Remove this
#define kServiceEndpoint_Data_Management 0x18B4300200000003ull ///< Core Weave data management protocol endpoint

const nl::Weave::WRMPConfig gWRMPConfigService = { SERVICE_WRM_INITIAL_RETRANS_TIMEOUT_MS, SERVICE_WRM_ACTIVE_RETRANS_TIMEOUT_MS,
                                                   SERVICE_WRM_PIGGYBACK_ACK_TIMEOUT_MS, SERVICE_WRM_MAX_RETRANS };

const nl::Weave::WRMPConfig gWRMPConfigDevice  = { DEVICE_WRM_INITIAL_RETRANS_TIMEOUT_MS, DEVICE_WRM_ACTIVE_RETRANS_TIMEOUT_MS,
                                                   DEVICE_WRM_PIGGYBACK_ACK_TIMEOUT_MS, DEVICE_WRM_MAX_RETRANS };

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
    : mServiceSinkTraitCatalog(ResourceIdentifier(ResourceIdentifier::SELF_NODE_ID), mServiceSinkCatalogStore,
                               sizeof(mServiceSinkCatalogStore) / sizeof(mServiceSinkCatalogStore[0]))
    , mDeviceSinkTraitCatalog(ResourceIdentifier(ResourceIdentifier::SELF_NODE_ID), mDeviceSinkCatalogStore,
                               sizeof(mDeviceSinkCatalogStore) / sizeof(mDeviceSinkCatalogStore[0]))
    , mServiceSourceTraitCatalog(ResourceIdentifier(ResourceIdentifier::SELF_NODE_ID), mServiceSourceCatalogStore,
                               sizeof(mServiceSourceCatalogStore) / sizeof(mServiceSourceCatalogStore[0]))
    , mServiceSubClient(NULL)
    , mDeviceSubClient(NULL)
    , mServiceCounterSubHandler(NULL)
    , mServiceSubBinding(NULL)
    , mDeviceSubBinding(NULL)
    , mIsSubToServiceEstablished(false)
    , mIsServiceCounterSubEstablished(false)
    , mIsSubToServiceActivated(false)
    , mIsDeviceSubEstablished(false)
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

bool WDMFeature::AreServiceSubscriptionsEstablished(void)
{
    return (mIsSubToServiceEstablished && mIsServiceCounterSubEstablished);
}

bool WDMFeature::IsDeviceSubscriptionEstablished(void)
{
    return mIsDeviceSubEstablished;
}

void WDMFeature::InitiateSubscriptionToService(void)
{
    NRF_LOG_INFO("Initiating Subscription To Service");

    mServiceSubClient->EnableResubscribe(NULL);
    mServiceSubClient->InitiateSubscription();
}

void WDMFeature::InitiateSubscriptionToDevice(intptr_t arg)
{
    NRF_LOG_INFO("Initiating Subscription To Device");

    WdmFeature().TearDownDeviceSubscription();
    WdmFeature().mDeviceSubClient->EnableResubscribe(NULL);
    WdmFeature().mDeviceSubClient->InitiateSubscription();
}

void WDMFeature::TearDownDeviceSubscription(void)
{
    if (mDeviceSubClient)
    {
        mDeviceSubClient->AbortSubscription();
    }
}

void WDMFeature::TearDownServiceSubscriptions(void)
{
    if (mServiceSubClient)
    {
        mServiceSubClient->AbortSubscription();
        if (mServiceCounterSubHandler != NULL)
        {
            mServiceCounterSubHandler->AbortSubscription();
            mServiceCounterSubHandler = NULL;
        }
    }
}

void WDMFeature::HandleBindingEvent(void * appState, ::nl::Weave::Binding::EventType eventType,
                                           const ::nl::Weave::Binding::InEventParam & inParam,
                                           ::nl::Weave::Binding::OutEventParam & outParam)
{
    Binding * binding = inParam.Source;

    switch (eventType)
    {
        case Binding::kEvent_PrepareRequested:
        {
            uint64_t target_node = kServiceEndpoint_Data_Management;

            if (binding == sWDMfeature.mDeviceSubBinding)
            {
                if (!DeviceDiscoveryFeature().GetDiscoveredDeviceNodeId(target_node))
                {
                    outParam.PrepareRequested.PrepareError = WEAVE_ERROR_INCORRECT_STATE;
                    break;
                }

                outParam.PrepareRequested.PrepareError = binding->BeginConfiguration()
                                                         .Target_NodeId(target_node)
                                                         .TargetAddress_WeaveFabric(::nl::Weave::kWeaveSubnetId_ThreadMesh)
                                                         .Transport_UDP_WRM()
                                                         .Transport_DefaultWRMPConfig(gWRMPConfigDevice)
                                                         .Exchange_ResponseTimeoutMsec(DEVICE_MESSAGE_RESPONSE_TIMEOUT_MS)
                                                         .Security_CASESession()
                                                         .PrepareBinding();
            }
            else if (binding == sWDMfeature.mServiceSubBinding)
            {
                outParam.PrepareRequested.PrepareError = binding->BeginConfiguration()
                                                         .Target_ServiceEndpoint(kServiceEndpoint_Data_Management)
                                                         .Transport_UDP_WRM()
                                                         .Transport_DefaultWRMPConfig(gWRMPConfigService)
                                                         .Exchange_ResponseTimeoutMsec(SERVICE_MESSAGE_RESPONSE_TIMEOUT_MS)
                                                         .Security_SharedCASESession()
                                                         .PrepareBinding();
            }
            break;
        }

        case Binding::kEvent_PrepareFailed:
            NRF_LOG_INFO("Failed to prepare %s subscription binding: %s",
                                                                        (binding == sWDMfeature.mServiceSubBinding) ? "service" : "device",
                                                                        ErrorStr(inParam.PrepareFailed.Reason));
            break;

        case Binding::kEvent_BindingFailed:
            NRF_LOG_INFO("%s subscription binding failed: %s",
                                                            (binding == sWDMfeature.mServiceSubBinding) ? "service" : "device",
                                                            ErrorStr(inParam.BindingFailed.Reason));
            break;

        case Binding::kEvent_BindingReady:
            NRF_LOG_INFO("%s subscription binding ready",(binding == sWDMfeature.mServiceSubBinding) ? "service" : "device");
            break;

        default:
            nl::Weave::Binding::DefaultEventHandler(appState, eventType, inParam, outParam);
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

            if (inParam.mSubscribeRequestParsed.mIsSubscriptionIdValid &&
                inParam.mSubscribeRequestParsed.mMsgInfo->SourceNodeId == kServiceEndpoint_Data_Management)
            {
                NRF_LOG_INFO("Inbound service counter-subscription request received (sub id %016" PRIX64 ", path count %" PRId16 ")",
                             inParam.mSubscribeRequestParsed.mSubscriptionId, inParam.mSubscribeRequestParsed.mNumTraitInstances);

                sWDMfeature.mServiceCounterSubHandler = inParam.mSubscribeRequestParsed.mHandler;

                binding->SetDefaultResponseTimeout(SERVICE_MESSAGE_RESPONSE_TIMEOUT_MS);
                binding->SetDefaultWRMPConfig(gWRMPConfigService);
            }
            else
            {
                NRF_LOG_INFO("Inbound device subscription request received (sub id %016" PRIX64 ", path count %" PRId16 ")",
                             inParam.mSubscribeRequestParsed.mSubscriptionId, inParam.mSubscribeRequestParsed.mNumTraitInstances);
            }

            inParam.mSubscribeRequestParsed.mHandler->AcceptSubscribeRequest(inParam.mSubscribeRequestParsed.mTimeoutSecMin);
            break;
        }

        case SubscriptionHandler::kEvent_OnSubscriptionEstablished:
        {
            if (inParam.mSubscriptionEstablished.mHandler == sWDMfeature.mServiceCounterSubHandler)
            {
                NRF_LOG_INFO("Inbound service counter-subscription established");

                sWDMfeature.mIsServiceCounterSubEstablished = true;
            }
            else
            {
                 NRF_LOG_INFO("Inbound device subscription established");
            }
            break;
        }

        case SubscriptionHandler::kEvent_OnSubscriptionTerminated:
        {
            const char * termDesc = (inParam.mSubscriptionTerminated.mReason == WEAVE_ERROR_STATUS_REPORT_RECEIVED)
                ? StatusReportStr(inParam.mSubscriptionTerminated.mStatusProfileId, inParam.mSubscriptionTerminated.mStatusCode)
                : ErrorStr(inParam.mSubscriptionTerminated.mReason);

            if (inParam.mSubscriptionTerminated.mHandler == sWDMfeature.mServiceCounterSubHandler)
            {
                NRF_LOG_INFO("Inbound service counter-subscription terminated: %s", termDesc);

                sWDMfeature.mServiceCounterSubHandler       = NULL;
                sWDMfeature.mIsServiceCounterSubEstablished = false;
            }
            else
            {
                NRF_LOG_INFO("Inbound device subscription terminated: %s", termDesc);
            }
            break;
        }

        default:
            SubscriptionHandler::DefaultEventHandler(eventType, inParam, outParam);
            break;
    }
}

void WDMFeature::HandleOutboundServiceSubscriptionEvent(void * appState, SubscriptionClient::EventID eventType,
                                                        const SubscriptionClient::InEventParam & inParam,
                                                        SubscriptionClient::OutEventParam & outParam)
{
    switch (eventType)
    {
        case SubscriptionClient::kEvent_OnSubscribeRequestPrepareNeeded:
        {
            if (inParam.mSubscribeRequestPrepareNeeded.mClient == sWDMfeature.mServiceSubClient)
            {
                outParam.mSubscribeRequestPrepareNeeded.mPathList                  = &(sWDMfeature.mServiceSinkTraitPaths[0]);
                outParam.mSubscribeRequestPrepareNeeded.mPathListSize              = kSinkHandle_Service_Max;
                outParam.mSubscribeRequestPrepareNeeded.mTimeoutSecMin             = SERVICE_LIVENESS_TIMEOUT_SEC;
                outParam.mSubscribeRequestPrepareNeeded.mTimeoutSecMax             = SERVICE_LIVENESS_TIMEOUT_SEC;
            }
            else if (inParam.mSubscribeRequestPrepareNeeded.mClient == sWDMfeature.mDeviceSubClient)
            {
                outParam.mSubscribeRequestPrepareNeeded.mPathList                  = &(sWDMfeature.mDeviceSinkTraitPaths[0]);
                outParam.mSubscribeRequestPrepareNeeded.mPathListSize              = kSinkHandle_Device_Max;
                outParam.mSubscribeRequestPrepareNeeded.mTimeoutSecMin             = DEVICE_LIVENESS_TIMEOUT_SEC;
                outParam.mSubscribeRequestPrepareNeeded.mTimeoutSecMax             = DEVICE_LIVENESS_TIMEOUT_SEC;
            }

            outParam.mSubscribeRequestPrepareNeeded.mVersionedPathList         = NULL;
            outParam.mSubscribeRequestPrepareNeeded.mNeedAllEvents             = false;
            outParam.mSubscribeRequestPrepareNeeded.mLastObservedEventList     = NULL;
            outParam.mSubscribeRequestPrepareNeeded.mLastObservedEventListSize = 0;

            NRF_LOG_INFO("Sending outbound service subscribe request");

            break;
        }
        case SubscriptionClient::kEvent_OnSubscriptionEstablished:
            if (inParam.mSubscriptionEstablished.mClient == sWDMfeature.mServiceSubClient)
            {
                NRF_LOG_INFO("Outbound service subscription established (sub id %016" PRIX64 ")",
                             inParam.mSubscriptionEstablished.mSubscriptionId);

                sWDMfeature.mIsSubToServiceEstablished = true;
            }
            else if (inParam.mSubscriptionEstablished.mClient == sWDMfeature.mDeviceSubClient)
            {
                NRF_LOG_INFO("Outbound device subscription established (sub id %016" PRIX64 ")",
                             inParam.mSubscriptionEstablished.mSubscriptionId);

                sWDMfeature.mIsDeviceSubEstablished = true;

                BoltLockMgr().EvaluateChange();
            }
            break;

        case SubscriptionClient::kEvent_OnSubscriptionTerminated:
        {
            if (inParam.mSubscriptionTerminated.mClient == sWDMfeature.mServiceSubClient)
            {
                NRF_LOG_INFO(
                    "Outbound service subscription terminated: %s",
                    (inParam.mSubscriptionTerminated.mIsStatusCodeValid)
                        ? StatusReportStr(inParam.mSubscriptionTerminated.mStatusProfileId, inParam.mSubscriptionTerminated.mStatusCode)
                        : ErrorStr(inParam.mSubscriptionTerminated.mReason));


                sWDMfeature.mIsSubToServiceEstablished = false;
            }
            else if (inParam.mSubscriptionTerminated.mClient == sWDMfeature.mDeviceSubClient)
            {
                NRF_LOG_INFO(
                    "Outbound device subscription terminated: %s",
                    (inParam.mSubscriptionTerminated.mIsStatusCodeValid)
                        ? StatusReportStr(inParam.mSubscriptionTerminated.mStatusProfileId, inParam.mSubscriptionTerminated.mStatusCode)
                        : ErrorStr(inParam.mSubscriptionTerminated.mReason));

                sWDMfeature.mIsDeviceSubEstablished = false;

                BoltLockMgr().EvaluateChange();
            }
            break;
        }

        default:
            SubscriptionClient::DefaultEventHandler(eventType, inParam, outParam);
            break;
    }
}

void WDMFeature::PlatformEventHandler(const WeaveDeviceEvent * event, intptr_t arg)
{
    bool serviceSubShouldBeActivated = (ConnectivityMgr().HaveServiceConnectivity() && ConfigurationMgr().IsPairedToAccount());

    // If we should be activated and we are not, initiate subscription
    if (serviceSubShouldBeActivated == true && sWDMfeature.mIsSubToServiceActivated == false)
    {
        sWDMfeature.InitiateSubscriptionToService();
        sWDMfeature.mIsSubToServiceActivated = true;
    }
    // If we should be activated and we already are, but not established and not in progress
    else if (serviceSubShouldBeActivated && sWDMfeature.mIsSubToServiceActivated && !sWDMfeature.mIsSubToServiceEstablished &&
             !sWDMfeature.mServiceSubClient->IsInProgressOrEstablished())
    {
        // Connectivity might have just come back again. Reset Resubscribe Interval
        sWDMfeature.mServiceSubClient->ResetResubscribe();
    }
}

WEAVE_ERROR WDMFeature::Init()
{
    WEAVE_ERROR err;
    int ret;

    ret = mPublisherLock.Init();
    VerifyOrExit(ret != NRF_ERROR_NULL, err = WEAVE_ERROR_NO_MEMORY);

    PlatformMgr().AddEventHandler(PlatformEventHandler);

    mBoltLockTraitSource.Init();

    mServiceSourceTraitCatalog.AddAt(0, &mBoltLockTraitSource, kSourceHandle_BoltLockTrait);
    mServiceSourceTraitCatalog.AddAt(0, &mDeviceIdentityTraitSource, kSourceHandle_DeviceIdentityTrait);

    mServiceSinkTraitCatalog.AddAt(0, &mBoltLockSettingsTraitSink, kSinkHandle_Service_BoltLockSettingsTrait);

    mDeviceSinkTraitCatalog.AddAt(0, &mSecurityOpenCloseTraitSink, kSinkHandle_Device_SecurityOpenCloseTrait);

    for (uint8_t handle = 0; handle < kSinkHandle_Service_Max; handle++)
    {
        mServiceSinkTraitPaths[handle].mTraitDataHandle    = handle;
        mServiceSinkTraitPaths[handle].mPropertyPathHandle = kRootPropertyPathHandle;
    }

    for (uint8_t handle = 0; handle < kSinkHandle_Device_Max; handle++)
    {
        mDeviceSinkTraitPaths[handle].mTraitDataHandle    = handle;
        mDeviceSinkTraitPaths[handle].mPropertyPathHandle = kRootPropertyPathHandle;
    }

    err = mSubscriptionEngine.Init(&ExchangeMgr, this, HandleSubscriptionEngineEvent);
    SuccessOrExit(err);

    err = mSubscriptionEngine.EnablePublisher(&mPublisherLock, &mServiceSourceTraitCatalog);
    SuccessOrExit(err);

    mServiceSubBinding = ExchangeMgr.NewBinding(HandleBindingEvent, this);
    VerifyOrExit(NULL != mServiceSubBinding, err = WEAVE_ERROR_NO_MEMORY);

    mDeviceSubBinding = ExchangeMgr.NewBinding(HandleBindingEvent, this);
    VerifyOrExit(NULL != mDeviceSubBinding, err = WEAVE_ERROR_NO_MEMORY);

    err = mSubscriptionEngine.NewClient(&(mServiceSubClient), mServiceSubBinding, this, HandleOutboundServiceSubscriptionEvent,
                                        &mServiceSinkTraitCatalog, SUBSCRIPTION_RESPONSE_TIMEOUT_MS);
    SuccessOrExit(err);

    err = mSubscriptionEngine.NewClient(&(mDeviceSubClient), mDeviceSubBinding, this, HandleOutboundServiceSubscriptionEvent,
                                        &mDeviceSinkTraitCatalog, SUBSCRIPTION_RESPONSE_TIMEOUT_MS);
    SuccessOrExit(err);


    NRF_LOG_INFO("WDMFeature Init Complete");

exit:
    return err;
}
