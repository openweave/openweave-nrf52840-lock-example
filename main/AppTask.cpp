#include "AppTask.h"
#include "WDMFeature.h"
#include "LEDWidget.h"
#include <schema/include/BoltLockTrait.h>

#include "boards.h"
#include "app_timer.h"
#include "app_button.h"

#include "nrf_log.h"

#include <Weave/DeviceLayer/WeaveDeviceLayer.h>

#include "FreeRTOS.h"

using namespace nl::Weave::DeviceLayer;

#define FACTORY_RESET_TRIGGER_TIMEOUT           5000
#define FACTORY_RESET_CANCEL_WINDOW_TIMEOUT     10000
#define APP_TASK_STACK_SIZE                     (4096)
#define APP_TASK_PRIORITY                       2

#define LOCK_BUTTON                             BUTTON_2
#define FACTORY_RESET_BUTTON                    BUTTON_4
#define FUNCTION_BUTTON_DEBOUNCE_PERIOD_MS      50

#define LOCK_STATE_LED                          BSP_LED_0
#define SYSTEM_STATE_LED                        BSP_LED_1

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

static bool isThreadProvisioned;
static bool isThreadEnabled;
static bool isThreadAttached;
static bool isPairedToAccount;
static bool isServiceSubscriptionEstablished;
static bool haveBLEConnections;
static bool haveServiceConnectivity;

int AppTask::Init()
{
	ret_code_t ret = NRF_SUCCESS;

    sAppEventQueue = xQueueCreate(APP_EVENT_QUEUE_SIZE, sizeof(AppActionEvent));
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

int AppTask::InitRoutine()
{
    ret_code_t ret;

    // Initialize LEDs
    statusLED.Init(LOCK_STATE_LED);

    lockLED.Init(SYSTEM_STATE_LED);
    lockLED.Set(!sLock.IsUnlocked());

    unusedLED.Init(BSP_LED_2);
    unusedLED_1.Init(BSP_LED_3);

    // Initialize buttons
    static app_button_cfg_t sButtons[] =
    {
        { LOCK_BUTTON, APP_BUTTON_ACTIVE_LOW, BUTTON_PULL, LockButtonEventHandler },
        { FACTORY_RESET_BUTTON, APP_BUTTON_ACTIVE_LOW, BUTTON_PULL, FunctionButtonHandler },
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

    ret = app_timer_create(&sFunctionTimer, APP_TIMER_MODE_SINGLE_SHOT, FunctionTimerEventHandler);
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
    AppActionEvent event;

    ret = sAppTask.InitRoutine();
    if (ret != NRF_SUCCESS)
    {
        NRF_LOG_INFO("app_timer_create() failed");
        APP_ERROR_HANDLER(ret);
    }

	while (true)
    {
        BaseType_t eventReceived = xQueueReceive(sAppEventQueue, &event, pdMS_TO_TICKS(10));
        if (eventReceived == pdTRUE)
        {
           NRF_LOG_INFO("Handling Event");

           sAppTask.LockActionEventHandler(&event);
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

        // Update the status LED...
        //
        // If system has "full connectivity", keep the LED On constantly.
        //
        // If thread and service provisioned, but not attached to the thread network yet OR no
        // connectivity to the service OR subscriptions are not fully established
        // THEN blink the LED Off for a short period of time.
        //
        // If the system is thread provisioned but not paired to account, blink the LED at an even
        // 500ms rate.
        //
        // If we are not thread provisioned but have atleast 1 BLE connection, blink the LED fast
        // an even 100ms rate.
        //
        // Otherwise, blink the LED ON for a very short time.
        if (isFullyConnected || sAppTask.mFunction == kFunction_FactoryReset)
        {
            statusLED.Set(true);
        }
        else if (isThreadProvisioned && isThreadEnabled && isPairedToAccount && (!isThreadAttached ||
            !haveServiceConnectivity || !isServiceSubscriptionEstablished))
        {
            statusLED.Blink(950,50);
        }
        else if (isThreadProvisioned && !isPairedToAccount)
        {
            statusLED.Blink(500,500);
        }
        else if(haveBLEConnections && !isThreadProvisioned)
        {
            statusLED.Blink(100, 100);
        }
        else
        {
            statusLED.Blink(50,950);
        }

        statusLED.Animate();
        lockLED.Animate();
    }
}

void AppTask::LockActionEventHandler(AppActionEvent *aEvent)
{
    bool initiated = false;

    initiated = sLock.InitiateAction(aEvent->Actor, aEvent->Action);

    if (!initiated)
    {
        NRF_LOG_INFO("AAction is already in progress or active.");
    }
}

void AppTask::LockButtonEventHandler(uint8_t pin_no, uint8_t button_action)
{
    if (pin_no != LOCK_BUTTON)
        return;

    if (button_action == APP_BUTTON_PUSH)
    {
        LockWidget::Action_t action;
        if (sLock.IsUnlocked())
        {
            action = LockWidget::LOCK_ACTION;
        }
        else
        {
            action = LockWidget::UNLOCK_ACTION;
        }

        sAppTask.PostLockActionRequest((int32_t)Schema::Weave::Trait::Security::BoltLockTrait::BOLT_LOCK_ACTOR_METHOD_PHYSICAL, action);
    }
}

void AppTask::FunctionTimerEventHandler(void * p_context)
{
	// If we reached here, the button was held past FACTORY_RESET_TRIGGER_TIMEOUT, initiate factory reset
	if (sAppTask.mFunctionTimerActive && sAppTask.mFunction == kFunction_SoftwareUpdate)
	{
        NRF_LOG_INFO("Factory Reset Triggered. Press button again to cancel");

        // Signal Factory reset has been triggered by turning all LEDs ON.
        bsp_board_leds_on();

		// Start timer for FACTORY_RESET_CANCEL_WINDOW_TIMEOUT to allow user to cancel, if required.
		sAppTask.StartTimer(FACTORY_RESET_CANCEL_WINDOW_TIMEOUT);

		sAppTask.mFunction = kFunction_FactoryReset;
	}
	else if (sAppTask.mFunctionTimerActive && sAppTask.mFunction == kFunction_FactoryReset)
	{
		// Actually trigger Factory Reset
        nl::Weave::DeviceLayer::ConfigurationMgr().InitiateFactoryReset();
	}
}

void AppTask::FunctionButtonHandler(uint8_t pin_no, uint8_t button_action)
{
    if (pin_no != FACTORY_RESET_BUTTON)
        return;

    // Short Press on the FUNCTION_BUTTON triggers software update.
    // Long Press (> FACTORY_RESET_TRIGGER_TIMEOUT) on the button triggers factory reset.
    // Once factory reset has been triggered, a short press within FACTORY_RESET_CANCEL_WINDOW_TIMEOUT
    // cancels factory reset
    if (button_action == APP_BUTTON_PUSH)
    {
        if (!sAppTask.mFunctionTimerActive && sAppTask.mFunction == kFunction_NoneSelected)
        {
        	sAppTask.StartTimer(FACTORY_RESET_TRIGGER_TIMEOUT);

		    sAppTask.mFunction = kFunction_SoftwareUpdate;
        }
        else if (sAppTask.mFunctionTimerActive && sAppTask.mFunction == kFunction_FactoryReset)
        {
            bsp_board_leds_off();

            // Set lock status LED back to show state of lock.
            lockLED.Set(!sLock.IsUnlocked());

        	sAppTask.CancelTimer();

        	// Change the function to none selected since factory reset has been cancelled.
        	sAppTask.mFunction = kFunction_NoneSelected;

        	NRF_LOG_INFO("Factory Reset has been Cancelled");
        }
    }
    else
    {
    	// If the button was released before factory reset for initiated, trigger a software udpate.
    	if (sAppTask.mFunctionTimerActive && sAppTask.mFunction == kFunction_SoftwareUpdate)
    	{
    		sAppTask.CancelTimer();

            NRF_LOG_INFO("Software Update NOT IMPLEMENTED.");
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
    if (sAppEventQueue != NULL)
    {
        AppActionEvent event;
        event.Actor = aActor;
        event.Action = aAction;

        if (!xQueueSend(sAppEventQueue, &event, 1))
        {
            NRF_LOG_INFO("Failed to post event to app task event queue");
        }
    }
}
