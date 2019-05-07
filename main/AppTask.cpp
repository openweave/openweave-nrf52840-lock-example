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
#include "WDMFeature.h"
#include "LEDWidget.h"

#include <schema/include/BoltLockTrait.h>

#include "app_config.h"
#include "app_timer.h"
#include "app_button.h"
#include "boards.h"

#include "nrf_log.h"

#include "FreeRTOS.h"

using namespace ::nl::Weave::DeviceLayer;

#define FACTORY_RESET_TRIGGER_TIMEOUT           3000
#define FACTORY_RESET_CANCEL_WINDOW_TIMEOUT     3000
#define APP_TASK_STACK_SIZE                     (4096)
#define APP_TASK_PRIORITY                       2

#define APP_EVENT_QUEUE_SIZE                    10

APP_TIMER_DEF(sFunctionTimer);

static TaskHandle_t sAppTaskHandle;
static QueueHandle_t sAppEventQueue;

AppTask AppTask::sAppTask;

static LEDWidget statusLED;
static LEDWidget lockLED;
static LEDWidget unusedLED;
static LEDWidget unusedLED_1;

static LockWidget sLock;

static bool isThreadProvisioned = false;
static bool isThreadEnabled = false;
static bool isThreadAttached = false;
static bool isPairedToAccount = false;
static bool isServiceSubscriptionEstablished = false;
static bool haveBLEConnections = false;
static bool haveServiceConnectivity = false;

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
    if (xTaskCreate(AppTaskMain, "APP", APP_TASK_STACK_SIZE / sizeof(StackType_t), NULL, APP_TASK_PRIORITY, &sAppTaskHandle) != pdPASS)
    {
        ret = NRF_ERROR_NULL;
    }

    return ret;
}

int AppTask::Init()
{
    ret_code_t ret;

    // Initialize LEDs
    statusLED.Init(SYSTEM_STATE_LED);

    lockLED.Init(LOCK_STATE_LED);
    lockLED.Set(!sLock.IsUnlocked());

    unusedLED.Init(BSP_LED_2);
    unusedLED_1.Init(BSP_LED_3);

    // Initialize buttons
    static app_button_cfg_t sButtons[] =
    {
        { LOCK_BUTTON, APP_BUTTON_ACTIVE_LOW, BUTTON_PULL, ButtonEventHandler },
        { FUNCTION_BUTTON, APP_BUTTON_ACTIVE_LOW, BUTTON_PULL, ButtonEventHandler },
    };

    ret = app_button_init(sButtons, ARRAY_SIZE(sButtons), pdMS_TO_TICKS(FUNCTION_BUTTON_DEBOUNCE_PERIOD_MS));
    if (ret != NRF_SUCCESS)
    {
        NRF_LOG_INFO("app_button_init() failed");
        APP_ERROR_HANDLER(ret);
    }

    ret = app_button_enable();
    if (ret != NRF_SUCCESS)
    {
        NRF_LOG_INFO("app_button_enable() failed");
        APP_ERROR_HANDLER(ret);
    }

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

    ret = sLock.Init();
    if (ret != NRF_SUCCESS)
    {
        NRF_LOG_INFO("sLock.Init() failed");
        APP_ERROR_HANDLER(ret);
    }

    sLock.SetCallbacks(ActionInitiated, ActionCompleted);

    ret = WdmFeature().Init();
    if (ret != WEAVE_NO_ERROR)
    {
        NRF_LOG_INFO("WdmFeature().Init() failed");
        APP_ERROR_HANDLER(ret);
    }

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
            isThreadProvisioned = ConnectivityMgr().IsThreadProvisioned();
            isThreadEnabled = ConnectivityMgr().IsThreadEnabled();
            isThreadAttached = ConnectivityMgr().IsThreadAttached();
            haveBLEConnections = (ConnectivityMgr().NumBLEConnections() != 0);
            isPairedToAccount = ConfigurationMgr().IsPairedToAccount();
            haveServiceConnectivity = ConnectivityMgr().HaveServiceConnectivity();
            isServiceSubscriptionEstablished = WdmFeature().AreServiceSubscriptionsEstablished();
            PlatformMgr().UnlockWeaveStack();
        }

        // Consider the system to be "fully connected" if it has service
        // connectivity and it is able to interact with the service on a regular basis.
        bool isFullyConnected = (haveServiceConnectivity && isServiceSubscriptionEstablished);

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
        if (sAppTask.mFunction != kFunction_FactoryReset)
        {
            if (isFullyConnected)
            {
                statusLED.Set(true);
            }
            else if (isThreadProvisioned && isThreadEnabled && isPairedToAccount && (!isThreadAttached ||
                !isFullyConnected))
            {
                statusLED.Blink(950,50);
            }
            else if(haveBLEConnections)
            {
                statusLED.Blink(100, 100);
            }
            else
            {
                statusLED.Blink(50,950);
            }
        }

        statusLED.Animate();
        lockLED.Animate();
        unusedLED.Animate();
        unusedLED_1.Animate();
    }
}

void AppTask::LockActionEventHandler(AppEvent *aEvent)
{
    bool initiated = false;
    LockWidget::Action_t action;
    int32_t actor;
    ret_code_t ret = NRF_SUCCESS;

    if (aEvent->Type == AppEvent::kEventType_Lock)
    {
        action = static_cast<LockWidget::Action_t>(aEvent->LockEvent.Action);
        actor = aEvent->LockEvent.Actor;
    }
    else if (aEvent->Type == AppEvent::kEventType_Button)
    {
        if (sLock.IsUnlocked())
        {
            action = LockWidget::LOCK_ACTION;
        }
        else
        {
            action = LockWidget::UNLOCK_ACTION;
        }

        actor = Schema::Weave::Trait::Security::BoltLockTrait::BOLT_LOCK_ACTOR_METHOD_PHYSICAL;
    }
    else
    {
        ret = NRF_ERROR_NULL;
    }

    if (ret == NRF_SUCCESS)
    {
        initiated = sLock.InitiateAction(actor, action);

        if (!initiated)
        {
            NRF_LOG_INFO("Action is already in progress or active.");
        }
    }
}

void AppTask::ButtonEventHandler(uint8_t pin_no, uint8_t button_action)
{
    if (pin_no != LOCK_BUTTON && pin_no != FUNCTION_BUTTON)
    {
        return;
    }

    AppEvent button_event;
    button_event.Type = AppEvent::kEventType_Button;
    button_event.ButtonEvent.PinNo = pin_no;
    button_event.ButtonEvent.Action = button_action;

    if (pin_no == LOCK_BUTTON && button_action == APP_BUTTON_PUSH)
    {
        button_event.Handler = LockActionEventHandler;
    }
    else if (pin_no == FUNCTION_BUTTON)
    {
        button_event.Handler = FunctionHandler;
    }

    sAppTask.PostEvent(&button_event);
}

void AppTask::TimerEventHandler(void * p_context)
{
    AppEvent event;
    event.Type = AppEvent::kEventType_Timer;
    event.TimerEvent.Context = p_context;
    event.Handler = FunctionTimerEventHandler;
    sAppTask.PostEvent(&event);
}

void AppTask::FunctionTimerEventHandler(AppEvent *aEvent)
{
    if (aEvent->Type != AppEvent::kEventType_Timer)
        return;

    // If we reached here, the button was held past FACTORY_RESET_TRIGGER_TIMEOUT, initiate factory reset
    if (sAppTask.mFunctionTimerActive && sAppTask.mFunction == kFunction_SoftwareUpdate)
    {
        NRF_LOG_INFO("Factory Reset Triggered. Press button again to cancel");

        // Start timer for FACTORY_RESET_CANCEL_WINDOW_TIMEOUT to allow user to cancel, if required.
        sAppTask.StartTimer(FACTORY_RESET_CANCEL_WINDOW_TIMEOUT);

        sAppTask.mFunction = kFunction_FactoryReset;

        // Turn off all LEDs before starting blink to make sure blink is co-ordinated.
        statusLED.Set(false);
        lockLED.Set(false);
        unusedLED_1.Set(false);
        unusedLED.Set(false);

        statusLED.Blink(500);
        lockLED.Blink(500);
        unusedLED.Blink(500);
        unusedLED_1.Blink(500);
    }
    else if (sAppTask.mFunctionTimerActive && sAppTask.mFunction == kFunction_FactoryReset)
    {
        // Actually trigger Factory Reset
        nl::Weave::DeviceLayer::ConfigurationMgr().InitiateFactoryReset();
    }
}

void AppTask::FunctionHandler(AppEvent* aEvent)
{
    if (aEvent->ButtonEvent.PinNo != FUNCTION_BUTTON)
        return;

    // To trigger software update: press the FUNCTION_BUTTON button briefly (< FACTORY_RESET_TRIGGER_TIMEOUT)
    // To initiate factory reset: press the FUNCTION_BUTTON for FACTORY_RESET_TRIGGER_TIMEOUT + FACTORY_RESET_CANCEL_WINDOW_TIMEOUT
    // All LEDs start blinking after FACTORY_RESET_TRIGGER_TIMEOUT to signal factory reset has been initiated.
    // To cancel factory reset: release the FUNCTION_BUTTON once all LEDs start blinking within the FACTORY_RESET_CANCEL_WINDOW_TIMEOUT
    if (aEvent->ButtonEvent.Action == APP_BUTTON_PUSH)
    {
        if (!sAppTask.mFunctionTimerActive && sAppTask.mFunction == kFunction_NoneSelected)
        {
            sAppTask.StartTimer(FACTORY_RESET_TRIGGER_TIMEOUT);

            sAppTask.mFunction = kFunction_SoftwareUpdate;
        }
    }
    else
    {
        // If the button was released before factory reset got initiated, trigger a software udpate.
        if (sAppTask.mFunctionTimerActive && sAppTask.mFunction == kFunction_SoftwareUpdate)
        {
            sAppTask.CancelTimer();

            NRF_LOG_INFO("Software Update NOT IMPLEMENTED.");
        }
        else if (sAppTask.mFunctionTimerActive && sAppTask.mFunction == kFunction_FactoryReset)
        {
            unusedLED.Set(false);
            unusedLED_1.Set(false);

            // Set lock status LED back to show state of lock.
            lockLED.Set(!sLock.IsUnlocked());

            sAppTask.CancelTimer();

            // Change the function to none selected since factory reset has been cancelled.
            sAppTask.mFunction = kFunction_NoneSelected;

            NRF_LOG_INFO("Factory Reset has been Cancelled");
        }
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
    else
    {
        NRF_LOG_INFO("Cancelling Function Timer");
    }

    mFunctionTimerActive = false;
}

void AppTask::StartTimer(uint32_t aTimeoutInMs)
{
    ret_code_t ret;

    ret = app_timer_start(sFunctionTimer, pdMS_TO_TICKS(aTimeoutInMs), this);
    if (ret != NRF_SUCCESS)
    {
        NRF_LOG_INFO("app_timer_start() failed");
        APP_ERROR_HANDLER(ret);
    }

    mFunctionTimerActive = true;
}

void AppTask::ActionInitiated(LockWidget::Action_t aAction, int32_t aActor)
{
    // If the action has been initiated by the lock, update the bolt lock trait
    // and start flashing the LEDs rapidly to indicate action initiation.
    if (aAction == LockWidget::LOCK_ACTION)
    {
        WdmFeature().GetBoltLockTraitDataSource().InitiateLock(aActor);
        NRF_LOG_INFO("Lock Action has been initiated")
    }
    else if (aAction == LockWidget::UNLOCK_ACTION)
    {
        WdmFeature().GetBoltLockTraitDataSource().InitiateUnlock(aActor);
        NRF_LOG_INFO("Unlock Action has been initiated")
    }

    lockLED.Blink(50,50);
}

void AppTask::ActionCompleted(LockWidget::Action_t aAction)
{
    // if the action has been completed by the lock, update the bolt lock trait.
    // Turn on the lock LED if in a LOCKED state OR
    // Turn off the lock LED if in an UNLOCKED state.
    if (aAction == LockWidget::LOCK_ACTION)
    {
        NRF_LOG_INFO("Lock Action has been completed")

        WdmFeature().GetBoltLockTraitDataSource().LockingSuccessful();

        lockLED.Set(true);
    }
    else if (aAction == LockWidget::UNLOCK_ACTION)
    {
        NRF_LOG_INFO("Unlock Action has been completed")

        WdmFeature().GetBoltLockTraitDataSource().UnlockingSuccessful();

        lockLED.Set(false);
    }
}

void AppTask::PostLockActionRequest(int32_t aActor, LockWidget::Action_t aAction)
{
    AppEvent event;
    event.Type = AppEvent::kEventType_Lock;
    event.LockEvent.Actor = aActor;
    event.LockEvent.Action = aAction;
    event.Handler = LockActionEventHandler;
    PostEvent(&event);
}

void AppTask::PostEvent(const AppEvent *aEvent)
{
    if (sAppEventQueue != NULL)
    {
        if (!xQueueSend(sAppEventQueue, aEvent, 1))
        {
            NRF_LOG_INFO("Failed to post event to app task event queue");
        }
    }
}

void AppTask::DispatchEvent(AppEvent *aEvent)
{
    if (aEvent->Handler)
    {
        aEvent->Handler(aEvent);
    }
    else
    {
        NRF_LOG_INFO("Event recieved with no handler. Dropping event.");
    }
}