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

#include "AppEvent.h"
#include "AppTask.h"
#include "ButtonManager.h"

#include "app_config.h"
#include "app_timer.h"
#include "app_button.h"
#include "boards.h"

#include "nrf_log.h"

#include <Weave/Support/TimeUtils.h>

ButtonManager ButtonManager::sButtonManager;

int ButtonManager::Init(void)
{
    int ret = NRF_SUCCESS;

    mLongPressActivatedFlag = 0;
    mLongPressTimeoutMs = LONG_PRESS_TIMEOUT_MS;
    mAppEventHandlerList = NULL;
    memset(mLongPressButtonTimerExpirationMs, 0, sizeof(mLongPressButtonTimerExpirationMs));

    // Initialize buttons
    static app_button_cfg_t sButtons[] = {
        { BUTTON_1, APP_BUTTON_ACTIVE_LOW, BUTTON_PULL, ButtonEventHandler },
        { BUTTON_2, APP_BUTTON_ACTIVE_LOW, BUTTON_PULL, ButtonEventHandler },
        { BUTTON_3, APP_BUTTON_ACTIVE_LOW, BUTTON_PULL, ButtonEventHandler },
        { BUTTON_4, APP_BUTTON_ACTIVE_LOW, BUTTON_PULL, ButtonEventHandler },
    };

    ret = app_button_init(sButtons, ARRAY_SIZE(sButtons), pdMS_TO_TICKS(FUNCTION_BUTTON_DEBOUNCE_PERIOD_MS));
    if (ret != NRF_SUCCESS)
    {
        NRF_LOG_INFO("app_button_init() failed");
        VerifyOrExit(ret == NRF_SUCCESS, );
    }

    ret = app_button_enable();
    if (ret != NRF_SUCCESS)
    {
        NRF_LOG_INFO("app_button_enable() failed");
        VerifyOrExit(ret == NRF_SUCCESS, );
    }

exit:
    return ret;
}

bool ButtonManager::GetButtonIdFromPinNo(uint8_t aPinNo, uint8_t & aButtonId)
{
    for (uint8_t index = 0; index < ARRAY_SIZE(mButtonIndex); index++)
    {
        if (mButtonIndex[index] == aPinNo)
        {
            aButtonId = index;
            return true;
        }
    }

    return false;
}

void ButtonManager::SetLongPressTimeoutMs(uint16_t aLongPressTimeoutMs)
{
    mLongPressTimeoutMs = aLongPressTimeoutMs;
}

bool ButtonManager::IsLongPressActive(uint8_t button_id)
{
    return mLongPressActivatedFlag & (1 << button_id);
}

void ButtonManager::StartLongPressTimer(uint8_t button_id)
{
    uint32_t now = ::nl::Weave::System::Platform::Layer::GetClock_MonotonicMS();

    mLongPressButtonTimerExpirationMs[button_id] = now + mLongPressTimeoutMs;
}

void ButtonManager::CancelLongPressTimer(uint8_t button_id)
{
    mLongPressButtonTimerExpirationMs[button_id] = 0;
}

void ButtonManager::ButtonEventHandler(uint8_t pin_no, uint8_t button_action)
{
    ButtonAction action = kButtonAction_Press;
    AppEvent event;

    event.Type                  = AppEvent::kEventType_Button;
    event.ButtonEvent.PinNo     = pin_no;

    if (button_action == APP_BUTTON_PUSH)
    {
        action = kButtonAction_Press;
    }
    else if (button_action == APP_BUTTON_RELEASE)
    {
        action = kButtonAction_Release;
    }

    event.ButtonEvent.Action = action;
    event.Handler = ButtonManagerEventHandler;

    GetAppTask().PostEvent(&event);
}

void ButtonManager::ButtonManagerEventHandler(const AppEvent * aButtonEvent)
{
    uint8_t button_id;
    ButtonManager & _this = static_cast<ButtonManager &>(ButtonMgr());

    _this.GetButtonIdFromPinNo(aButtonEvent->ButtonEvent.PinNo, button_id);

    AppEvent event;
    event.Type                  = AppEvent::kEventType_Button;
    event.ButtonEvent.PinNo     = aButtonEvent->ButtonEvent.PinNo;
    event.Handler               = NULL;

    if (aButtonEvent->ButtonEvent.Action == kButtonAction_Press)
    {
        // NRF_LOG_INFO("Button[%u] Press Detected.", button_id);

        /* Notify interested parties that the button was just pressed.
         * Also start a timer to detect if the user intends to do a long press.
         */
        _this.StartLongPressTimer(button_id);

        event.ButtonEvent.Action = kButtonAction_Press;
        _this.DispatchButtonEvent(&event);
    }
    else if (aButtonEvent->ButtonEvent.Action == kButtonAction_Release)
    {
        if (_this.IsLongPressActive(button_id))
        {
            // NRF_LOG_INFO("Button[%u] Release from Long Press Detected.", button_id);

            _this.mLongPressActivatedFlag &= ~(1 << button_id);

            /* Notify interested parties that the button was released after a long press.*/
            event.ButtonEvent.Action = kButtonAction_LongPress_Release;
            _this.DispatchButtonEvent(&event);
        }
        else if (_this.mLongPressButtonTimerExpirationMs[button_id] != 0)
        {
            // NRF_LOG_INFO("Button[%u] Release Detected.", button_id);

            _this.CancelLongPressTimer(button_id);

            /* Notify interested parties that the button was released after a short press
             * since the user released the button before the long press timer expired.*/
            event.ButtonEvent.Action = kButtonAction_Release;
            _this.DispatchButtonEvent(&event);
        }
    }
}

void ButtonManager::LongPressActivated(uint8_t button_id)
{
    AppEvent event;
    event.Type               = AppEvent::kEventType_Button;
    event.ButtonEvent.PinNo  = mButtonIndex[button_id];
    event.ButtonEvent.Action = kButtonAction_LongPress;
    event.Handler            = NULL;

    // NRF_LOG_INFO("Button[%u] Long Press Detected.", button_id);

    CancelLongPressTimer(button_id);

    mLongPressActivatedFlag |= (1 << button_id);

    DispatchButtonEvent(&event);
}

void ButtonManager::Poll(void)
{
    uint32_t now = ::nl::Weave::System::Platform::Layer::GetClock_MonotonicMS();

    for (uint8_t button_idx = 0; button_idx < 4; button_idx++)
    {
        if (mLongPressButtonTimerExpirationMs[button_idx] != 0 && now >= mLongPressButtonTimerExpirationMs[button_idx])
        {
            LongPressActivated(button_idx);
        }
    }
}

int ButtonManager::AddButtonEventHandler(uint8_t aButtonPinNo, ButtonEventHandlerFunct handler)
{
    int ret = NRF_SUCCESS;
    ButtonEventHandler_t * eventHandler;

    // Do nothing if the event handler is already registered.
    for (eventHandler = mAppEventHandlerList; eventHandler != NULL; eventHandler = eventHandler->Next)
    {
        if (eventHandler->PinNo == aButtonPinNo && eventHandler->Handler == handler)
        {
            ExitNow();
        }
    }

    eventHandler = (ButtonEventHandler_t *)malloc(sizeof(ButtonEventHandler_t));
    VerifyOrExit(eventHandler != NULL, ret = NRF_ERROR_NO_MEM);

    eventHandler->Next = mAppEventHandlerList;
    eventHandler->PinNo = aButtonPinNo;
    eventHandler->Handler = handler;

    mAppEventHandlerList = eventHandler;

exit:
    return ret;
}

void ButtonManager::RemoveEventHandler(uint8_t aButtonPinNo, ButtonEventHandlerFunct handler)
{
    ButtonEventHandler_t ** eventHandlerIndirectPtr;

    for (eventHandlerIndirectPtr = &mAppEventHandlerList; *eventHandlerIndirectPtr != NULL; )
    {
        ButtonEventHandler_t * eventHandler = (*eventHandlerIndirectPtr);

        if (eventHandler->PinNo == aButtonPinNo && eventHandler->Handler == handler)
        {
            *eventHandlerIndirectPtr = eventHandler->Next;
            free(eventHandler);
        }
        else
        {
            eventHandlerIndirectPtr = &eventHandler->Next;
        }
    }
}

void ButtonManager::DispatchButtonEvent(const AppEvent * event)
{
    // Dispatch the event to each of the registered application event handlers.
    for (ButtonEventHandler_t * eventHandler = mAppEventHandlerList;
         eventHandler != NULL;
         eventHandler = eventHandler->Next)
    {
        if (eventHandler->PinNo == event->ButtonEvent.PinNo)
        {
            eventHandler->Handler(event);
        }
    }
}