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

#include "AppTask.h"
#include "AppEvent.h"
#include "ButtonManager.h"
#include "DeviceDiscovery.h"
#include "WDMFeature.h"
#include "LEDWidget.h"

#include <schema/include/BoltLockTrait.h>

#include "app_config.h"
#include "app_timer.h"
#include "app_button.h"
#include "boards.h"

#include "nrf_log.h"

#include "FreeRTOS.h"

#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Support/crypto/HashAlgos.h>

using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::DeviceLayer;
using namespace ::nl::Weave::Profiles::SoftwareUpdate;

#define FACTORY_RESET_CANCEL_WINDOW_TIMEOUT 3000
#define APP_TASK_STACK_SIZE                 (4096)
#define APP_TASK_PRIORITY                   2
#define APP_EVENT_QUEUE_SIZE                10

APP_TIMER_DEF(sFunctionTimer);

static SemaphoreHandle_t sWeaveEventLock;

static TaskHandle_t sAppTaskHandle;
static QueueHandle_t sAppEventQueue;

/* Shows the system status (service provisioned,
 * service subscription establishment)
 */
static LEDWidget sStatusLED;

/* Shows the lock/unlock state:
 * Solid LED -> Locked
 * Off -> Unlocked
 * Blinking -> Actuator moving/In progress
 */
static LEDWidget sLockLED;

/* Shows the state of device subscription:
 * Off: Not subscribed to another device
 * Solid: Subscription established
 */
static LEDWidget sDeviceSubStatusLED;

static LEDWidget sUnusedLED_1;

static bool sIsThreadProvisioned              = false;
static bool sIsThreadEnabled                  = false;
static bool sIsThreadAttached                 = false;
static bool sIsPairedToAccount                = false;
static bool sIsServiceSubscriptionEstablished = false;
static bool sIsDeviceSubscriptionEstablished  = false;
static bool sHaveBLEConnections               = false;
static bool sHaveServiceConnectivity          = false;

static nl::Weave::Platform::Security::SHA256 sSHA256;

AppTask AppTask::sAppTask;

namespace nl {
namespace Weave {
namespace Profiles {
namespace DataManagement_Current {
namespace Platform {

void CriticalSectionEnter(void)
{
    xSemaphoreTake(sWeaveEventLock, 0);
}

void CriticalSectionExit(void)
{
    xSemaphoreGive(sWeaveEventLock);
}

} // namespace Platform
} // namespace DataManagement_Current
} // namespace Profiles
} // namespace Weave
} // namespace nl

int AppTask::StartAppTask()
{
    ret_code_t ret = NRF_SUCCESS;

    sAppEventQueue = xQueueCreate(APP_EVENT_QUEUE_SIZE, sizeof(AppEvent));
    if (sAppEventQueue == NULL)
    {
        NRF_LOG_INFO("Failed to allocate app event queue");
        ret = NRF_ERROR_NULL;
        APP_ERROR_HANDLER(ret);
    }

    // Start App task.
    if (xTaskCreate(AppTaskMain, "APP", APP_TASK_STACK_SIZE / sizeof(StackType_t), NULL, APP_TASK_PRIORITY, &sAppTaskHandle) !=
        pdPASS)
    {
        ret = NRF_ERROR_NULL;
    }

    return ret;
}

int AppTask::Init()
{
    ret_code_t ret;
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Initialize LEDs
    sStatusLED.Init(SYSTEM_STATE_LED);

    sLockLED.Init(LOCK_STATE_LED);
    sLockLED.Set(!BoltLockMgr().IsUnlocked());

    sDeviceSubStatusLED.Init(BSP_LED_2);
    sUnusedLED_1.Init(BSP_LED_3);

    // Initialize Timer for Function Selection
    ret = app_timer_init();
    if (ret != NRF_SUCCESS)
    {
        NRF_LOG_INFO("app_timer_init() failed");
        APP_ERROR_HANDLER(ret);
    }

    ret = app_timer_create(&sFunctionTimer, APP_TIMER_MODE_SINGLE_SHOT, TimerEventHandler);
    if (ret != NRF_SUCCESS)
    {
        NRF_LOG_INFO("app_timer_create() failed");
        APP_ERROR_HANDLER(ret);
    }

    ret = ButtonMgr().Init();
    if (ret != NRF_SUCCESS)
    {
        NRF_LOG_INFO("ButtonMgr().Init() failed");
        APP_ERROR_HANDLER(ret);
    }

#if SERVICELESS_DEVICE_DISCOVERY_ENABLED
    ButtonMgr().AddButtonEventHandler(ATTENTION_BUTTON, AttentionButtonHandler);
#endif

    ButtonMgr().AddButtonEventHandler(FUNCTION_BUTTON,  FunctionButtonHandler);

    ret = BoltLockMgr().Init();
    if (ret != NRF_SUCCESS)
    {
        NRF_LOG_INFO("BoltLockMgr().Init() failed");
        APP_ERROR_HANDLER(ret);
    }

    BoltLockMgr().AddEventHandler(LockActionEventHandler, 0);

    sWeaveEventLock = xSemaphoreCreateMutex();
    if (sWeaveEventLock == NULL)
    {
        NRF_LOG_INFO("xSemaphoreCreateMutex() failed");
        APP_ERROR_HANDLER(NRF_ERROR_NULL);
    }

    // Initialize WDM Feature
    ret = WdmFeature().Init();
    if (ret != WEAVE_NO_ERROR)
    {
        NRF_LOG_INFO("WdmFeature().Init() failed");
        APP_ERROR_HANDLER(ret);
    }

    SoftwareUpdateMgr().SetEventCallback(this, HandleSoftwareUpdateEvent);

    // Enable timer based Software Update Checks
    SoftwareUpdateMgr().SetQueryIntervalWindow(SWU_INTERVAl_WINDOW_MIN_MS, SWU_INTERVAl_WINDOW_MAX_MS);

    // Print the current software version
    char currentFirmwareRev[ConfigurationManager::kMaxFirmwareRevisionLength+1] = {0};
    size_t currentFirmwareRevLen;
    err = ConfigurationMgr().GetFirmwareRevision(currentFirmwareRev, sizeof(currentFirmwareRev), currentFirmwareRevLen);
    APP_ERROR_CHECK(err);

    ret = DeviceDiscoveryFeature().Init();
    if (ret != WEAVE_NO_ERROR)
    {
        NRF_LOG_INFO("DeviceDiscoveryFeature().Init() failed");
        APP_ERROR_HANDLER(ret);
    }

    NRF_LOG_INFO("Current Firmware Version: %s", currentFirmwareRev);

    return ret;
}

void AppTask::AppTaskMain(void * pvParameter)
{
    ret_code_t ret;
    AppEvent event;

    ret = sAppTask.Init();
    if (ret != NRF_SUCCESS)
    {
        NRF_LOG_INFO("AppTask.Init() failed");
        APP_ERROR_HANDLER(ret);
    }

    while (true)
    {
        BaseType_t eventReceived = xQueueReceive(sAppEventQueue, &event, pdMS_TO_TICKS(10));
        while (eventReceived == pdTRUE)
        {
            sAppTask.DispatchEvent(&event);
            eventReceived = xQueueReceive(sAppEventQueue, &event, 0);
        }

        // Collect connectivity and configuration state from the Weave stack.  Because the
        // Weave event loop is being run in a separate task, the stack must be locked
        // while these values are queried.  However we use a non-blocking lock request
        // (TryLockWeaveStack()) to avoid blocking other UI activities when the Weave
        // task is busy (e.g. with a long crypto operation).
        if (PlatformMgr().TryLockWeaveStack())
        {
            sIsThreadProvisioned              = ConnectivityMgr().IsThreadProvisioned();
            sIsThreadEnabled                  = ConnectivityMgr().IsThreadEnabled();
            sIsThreadAttached                 = ConnectivityMgr().IsThreadAttached();
            sHaveBLEConnections               = (ConnectivityMgr().NumBLEConnections() != 0);
            sIsPairedToAccount                = ConfigurationMgr().IsPairedToAccount();
            sHaveServiceConnectivity          = ConnectivityMgr().HaveServiceConnectivity();
            sIsServiceSubscriptionEstablished = WdmFeature().AreServiceSubscriptionsEstablished();
            sIsDeviceSubscriptionEstablished  = WdmFeature().IsDeviceSubscriptionEstablished();
            PlatformMgr().UnlockWeaveStack();
        }

        // Consider the system to be "fully connected" if it has service
        // connectivity and it is able to interact with the service on a regular basis.
        bool isFullyConnected = (sHaveServiceConnectivity && sIsServiceSubscriptionEstablished);

        // Update the status LED if factory reset has not been initiated.
        //
        // If system has "full connectivity", keep the LED On constantly.
        //
        // If thread and service provisioned, but not attached to the thread network yet OR no
        // connectivity to the service OR subscriptions are not fully established
        // THEN blink the LED Off for a short period of time.
        //
        // If the system has ble connection(s) uptill the stage above, THEN blink the LEDs at an even
        // rate of 100ms.
        //
        // Otherwise, blink the LED ON for a very short time.
        if (!sAppTask.mFactoryResetTimerActive)
        {
            if (isFullyConnected)
            {
                sStatusLED.Set(true);
            }
            else if (sIsThreadProvisioned && sIsThreadEnabled && sIsPairedToAccount && (!sIsThreadAttached || !isFullyConnected))
            {
                sStatusLED.Blink(950, 50);
            }
            else if (sHaveBLEConnections)
            {
                sStatusLED.Blink(100, 100);
            }
            else
            {
                sStatusLED.Blink(50, 950);
            }

            // Show the status of device subscription.
            sDeviceSubStatusLED.Set(sIsDeviceSubscriptionEstablished);
        }

        sStatusLED.Animate();
        sLockLED.Animate();
        sDeviceSubStatusLED.Animate();
        sUnusedLED_1.Animate();
        ButtonMgr().Poll();
    }
}

void AppTask::TimerEventHandler(void * p_context)
{
    if (sAppTask.mFactoryResetTimerActive)
    {
        AppEvent event;
        event.Type               = AppEvent::kEventType_Timer;
        event.TimerEvent.Context = p_context;
        event.Handler            = FactoryResetTriggerTimerExpired;
        sAppTask.PostEvent(&event);
    }
}

void AppTask::FactoryResetTriggerTimerExpired(const AppEvent * aEvent)
{
    sAppTask.mFactoryResetTimerActive = false;

    // Actually trigger Factory Reset
    nl::Weave::DeviceLayer::ConfigurationMgr().InitiateFactoryReset();
}

void AppTask::FunctionButtonHandler(const AppEvent * aEvent)
{
    // To trigger software update: press the FUNCTION_BUTTON button briefly (< LONG_PRESS_TIMEOUT_MS)
    // To initiate factory reset: press the FUNCTION_BUTTON for LONG_PRESS_TIMEOUT_MS + FACTORY_RESET_CANCEL_WINDOW_TIMEOUT
    // All LEDs start blinking after LONG_PRESS_TIMEOUT_MS to signal factory reset has been initiated.
    // To cancel factory reset: release the FUNCTION_BUTTON once all LEDs start blinking within the
    // FACTORY_RESET_CANCEL_WINDOW_TIMEOUT

    if (aEvent->ButtonEvent.Action == ButtonManager::kButtonAction_Release)
    {
        if (SoftwareUpdateMgr().IsInProgress())
        {
            NRF_LOG_INFO("Canceling In Progress Software Update");
            SoftwareUpdateMgr().Abort();
        }
        else
        {
            NRF_LOG_INFO("Manual Software Update Triggered");
            SoftwareUpdateMgr().CheckNow();
        }
    }
    else if (aEvent->ButtonEvent.Action == ButtonManager::kButtonAction_LongPress)
    {
        NRF_LOG_INFO("Factory Reset Triggered. Release button within %ums to cancel.", FACTORY_RESET_CANCEL_WINDOW_TIMEOUT);

        // Start timer for FACTORY_RESET_CANCEL_WINDOW_TIMEOUT to allow user to cancel, if required.
        sAppTask.StartTimer(FACTORY_RESET_CANCEL_WINDOW_TIMEOUT);

        // Turn off all LEDs before starting blink to make sure blink is co-ordinated.
        sStatusLED.Set(false);
        sLockLED.Set(false);
        sUnusedLED_1.Set(false);
        sDeviceSubStatusLED.Set(false);

        sStatusLED.Blink(500);
        sLockLED.Blink(500);
        sDeviceSubStatusLED.Blink(500);
        sUnusedLED_1.Blink(500);
    }
    else if (aEvent->ButtonEvent.Action == ButtonManager::kButtonAction_LongPress_Release && sAppTask.mFactoryResetTimerActive)
    {
        sDeviceSubStatusLED.Set(false);
        sUnusedLED_1.Set(false);

        // Set lock status LED back to show state of lock.
        sLockLED.Set(!BoltLockMgr().IsUnlocked());

        sAppTask.CancelTimer();

        NRF_LOG_INFO("Factory Reset has been Canceled");
    }
}

void AppTask::CancelTimer()
{
    ret_code_t ret;

    ret = app_timer_stop(sFunctionTimer);
    if (ret != NRF_SUCCESS)
    {
        NRF_LOG_INFO("app_timer_stop() failed");
        APP_ERROR_HANDLER(ret);
    }

    mFactoryResetTimerActive = false;
}

void AppTask::StartTimer(uint32_t aTimeoutInMs)
{
    ret_code_t ret;

    ret = app_timer_start(sFunctionTimer, pdMS_TO_TICKS(aTimeoutInMs), this);
    if (ret != NRF_SUCCESS)
    {
        NRF_LOG_INFO("app_timer_start() failed")
;        APP_ERROR_HANDLER(ret);
    }

    mFactoryResetTimerActive = true;
}

void AppTask::LockActionEventHandler(const AppEvent * aEvent, intptr_t Arg)
{
    if (aEvent->Type == AppEvent::kEventType_LockAction_Initiated)
    {
        if (aEvent->LockAction_Initiated.Action == BoltLockManager::LOCK_ACTION)
        {
            NRF_LOG_INFO("Lock Action has been initiated")
        }
        else if (aEvent->LockAction_Initiated.Action == BoltLockManager::UNLOCK_ACTION)
        {
            NRF_LOG_INFO("Unlock Action has been initiated")
        }

        sLockLED.Blink(50, 50);
    }
    else if (aEvent->Type == AppEvent::kEventType_LockAction_Completed)
    {
        if (aEvent->LockAction_Completed.Action == BoltLockManager::LOCK_ACTION)
        {
            NRF_LOG_INFO("Lock Action has been completed")

            sLockLED.Set(true);
        }
        else if (aEvent->LockAction_Completed.Action == BoltLockManager::UNLOCK_ACTION)
        {
            NRF_LOG_INFO("Unlock Action has been completed")

            sLockLED.Set(false);
        }
    }
}

void AppTask::PostEvent(const AppEvent * aEvent)
{
    if (sAppEventQueue != NULL)
    {
        if (!xQueueSend(sAppEventQueue, aEvent, 1))
        {
            NRF_LOG_INFO("Failed to post event to app task event queue");
        }
    }
}

void AppTask::DispatchEvent(AppEvent * aEvent)
{
    if (aEvent->Handler)
    {
        aEvent->Handler(aEvent);
    }
    else
    {
        if (aEvent->Type == AppEvent::kEventType_LocalDeviceDiscovered)
        {
            NRF_LOG_INFO("Local Device Discovered Event received. Attempting Device Subscription.");
            PlatformMgr().ScheduleWork(WdmFeature().InitiateSubscriptionToDevice);
        }
        else
        {
            NRF_LOG_INFO("Event received with no handler. Dropping event.");
        }
    }
}

void AppTask::InstallEventHandler(const AppEvent * aEvent)
{
    SoftwareUpdateMgr().ImageInstallComplete(WEAVE_NO_ERROR);
}

void AppTask::HandleSoftwareUpdateEvent(void *apAppState,
                                        SoftwareUpdateManager::EventType aEvent,
                                        const SoftwareUpdateManager::InEventParam& aInParam,
                                        SoftwareUpdateManager::OutEventParam& aOutParam)
{
    static uint64_t numBytesDownloaded = 0;

    switch(aEvent)
    {
        case SoftwareUpdateManager::kEvent_PrepareQuery:
        {
            aOutParam.PrepareQuery.PackageSpecification = NULL;
            aOutParam.PrepareQuery.DesiredLocale = NULL;
            break;
        }

        case SoftwareUpdateManager::kEvent_PrepareQuery_Metadata:
        {
            WEAVE_ERROR err;
            bool haveSufficientBattery = true;
            uint32_t certBodyId = 0;

            TLVWriter *writer = aInParam.PrepareQuery_Metadata.MetaDataWriter;

            if (writer)
            {
                // Providing an installed Locale as MetaData is optional. The commented section below provides an example
                // of how one can be added to metadata.

                // TLVType arrayContainerType;
                // err = writer->StartContainer(ProfileTag(::nl::Weave::Profiles::kWeaveProfile_SWU, kTag_InstalledLocales), kTLVType_Array, arrayContainerType);
                // SuccessOrExit(err);
                // err = writer->PutString(AnonymousTag, installedLocale);
                // SuccessOrExit(err);
                // err = writer->EndContainer(arrayContainerType);

                err = writer->Put(ProfileTag(::nl::Weave::Profiles::kWeaveProfile_SWU, kTag_CertBodyId), certBodyId);
                APP_ERROR_CHECK(err);

                err = writer->PutBoolean(ProfileTag(::nl::Weave::Profiles::kWeaveProfile_SWU, kTag_SufficientBatterySWU), haveSufficientBattery);
                APP_ERROR_CHECK(err);
            }
            else
            {
                aOutParam.PrepareQuery_Metadata.Error = WEAVE_ERROR_INVALID_ARGUMENT;
                NRF_LOG_INFO("ERROR ! aOutParam.PrepareQuery_Metadata.MetaDataWriter is NULL");
            }
            break;
        }

        case SoftwareUpdateManager::kEvent_QueryPrepareFailed:
        {
            NRF_LOG_INFO("Software Update failed during prepare %d (%s) statusReport = %s", aInParam.QueryPrepareFailed.Error,
                                                                nl::ErrorStr(aInParam.QueryPrepareFailed.Error),
                                                                (aInParam.QueryPrepareFailed.StatusReport != NULL) ?
                                                                 nl::StatusReportStr(aInParam.QueryPrepareFailed.StatusReport->mProfileId,
                                                                                    aInParam.QueryPrepareFailed.StatusReport->mStatusCode) :
                                                                 "None");
            break;
        }

        case SoftwareUpdateManager::kEvent_SoftwareUpdateAvailable:
        {
            WEAVE_ERROR err;

            char currentFirmwareRev[ConfigurationManager::kMaxFirmwareRevisionLength+1] = {0};
            size_t currentFirmwareRevLen;
            err = ConfigurationMgr().GetFirmwareRevision(currentFirmwareRev, sizeof(currentFirmwareRev), currentFirmwareRevLen);
            APP_ERROR_CHECK(err);

            NRF_LOG_INFO("Current Firmware Version: %s", currentFirmwareRev);

            NRF_LOG_INFO("Software Update Available - Priority: %d Condition: %d Version: %s IntegrityType: %d URI: %s",
                                                                                aInParam.SoftwareUpdateAvailable.Priority,
                                                                                aInParam.SoftwareUpdateAvailable.Condition,
                                                                                aInParam.SoftwareUpdateAvailable.Version,
                                                                                aInParam.SoftwareUpdateAvailable.IntegrityType,
                                                                                aInParam.SoftwareUpdateAvailable.URI);

            break;
        }

        case SoftwareUpdateManager::kEvent_FetchPartialImageInfo:
        {
            aOutParam.FetchPartialImageInfo.PartialImageLen = 0;
            break;
        }

        case SoftwareUpdateManager::kEvent_ClearImageFromStorage:
        {
            NRF_LOG_INFO("Clearing Image Storage");
            numBytesDownloaded = 0;
            break;
        }

        case SoftwareUpdateManager::kEvent_StartImageDownload:
        {
            sSHA256.Begin();
            NRF_LOG_INFO("Starting Image Download");
            break;
        }
        case SoftwareUpdateManager::kEvent_StoreImageBlock:
        {
            /* This example does not store image blocks in persistent storage and merely discards them after
             * computing the SHA over it. As a result, integrity has to be computed over image blocks rather than
             * the entire image. This pattern is NOT recommended since the computed integrity
             * will be lost if the device rebooted during download (unless also stored in persistent storage).
             */
            sSHA256.AddData(aInParam.StoreImageBlock.DataBlock, aInParam.StoreImageBlock.DataBlockLen);
            numBytesDownloaded += aInParam.StoreImageBlock.DataBlockLen;
            break;
        }

        case SoftwareUpdateManager::kEvent_ComputeImageIntegrity:
        {
            NRF_LOG_INFO("Num. bytes downloaded: %d", numBytesDownloaded);

            // Make sure that the buffer provided in the parameter is large enough.
            if (aInParam.ComputeImageIntegrity.IntegrityValueBufLen < sSHA256.kHashLength)
            {
                aOutParam.ComputeImageIntegrity.Error = WEAVE_ERROR_BUFFER_TOO_SMALL;
            }
            else
            {
                sSHA256.Finish(aInParam.ComputeImageIntegrity.IntegrityValueBuf);
            }
            break;
        }

        case SoftwareUpdateManager::kEvent_ReadyToInstall:
        {
            NRF_LOG_INFO("Image is ready to be installed");
            break;
        }

        case SoftwareUpdateManager::kEvent_StartInstallImage:
        {
            AppTask *_this = static_cast<AppTask*>(apAppState);

            NRF_LOG_INFO("Image Install is not supported in this example application");

            AppEvent event;
            event.Type             = AppEvent::kEventType_Install;
            event.Handler          = InstallEventHandler;
            _this->PostEvent(&event);

            break;
        }

        case SoftwareUpdateManager::kEvent_Finished:
        {
            if (aInParam.Finished.Error == WEAVE_ERROR_NO_SW_UPDATE_AVAILABLE)
            {
                NRF_LOG_INFO("No Software Update Available");
            }
            else if (aInParam.Finished.Error == WEAVE_DEVICE_ERROR_SOFTWARE_UPDATE_IGNORED)
            {
                NRF_LOG_INFO("Software Update Ignored by Application");
            }
            else if (aInParam.Finished.Error == WEAVE_DEVICE_ERROR_SOFTWARE_UPDATE_ABORTED)
            {
                NRF_LOG_INFO("Software Update Aborted by Application");
            }
            else if (aInParam.Finished.Error != WEAVE_NO_ERROR || aInParam.Finished.StatusReport != NULL)
            {
                NRF_LOG_INFO("Software Update failed %s statusReport = %s", nl::ErrorStr(aInParam.Finished.Error),
                                                                (aInParam.Finished.StatusReport != NULL) ?
                                                                 nl::StatusReportStr(aInParam.Finished.StatusReport->mProfileId,
                                                                                    aInParam.Finished.StatusReport->mStatusCode) :
                                                                 "None");
            }
            else
            {
                NRF_LOG_INFO("Software Update Completed");
            }
            break;
        }

        default:
            nl::Weave::DeviceLayer::SoftwareUpdateManager::DefaultEventHandler(apAppState, aEvent, aInParam, aOutParam);
            break;
    }
}

#if SERVICELESS_DEVICE_DISCOVERY_ENABLED

void AppTask::AttentionButtonHandler(const AppEvent * aEvent)
{
    if (aEvent->ButtonEvent.Action == ButtonManager::kButtonAction_Release)
    {
        ConnectivityMgr().SetUserSelectedMode(true);
    }
    else if (aEvent->ButtonEvent.Action == ButtonManager::kButtonAction_LongPress)
    {
        PlatformMgr().ScheduleWork(sAppTask.SeachForLocalSDKOpenCloseSensors);
    }
}

void AppTask::SeachForLocalSDKOpenCloseSensors(intptr_t arg)
{
    /* We are only interested in other devices that have user selected mode enabled and are of the type
     * SDK Open Close Example.
     */
    DeviceDiscoveryFeature().SearchForUserSelectedModeDevices(WEAVE_DEVICE_CONFIG_DEVICE_VENDOR_ID, 0xFE02);
}

#endif // SERVICELESS_DEVICE_DISCOVERY_ENABLED