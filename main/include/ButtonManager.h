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

#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include "boards.h"

// Forward declaration for AppEvent
struct AppEvent;

class ButtonManager
{
public:
    typedef void (*ButtonEventHandlerFunct)(const AppEvent * event);

    struct ButtonEventHandler_t
    {
        uint8_t PinNo;
        ButtonEventHandler_t * Next;
        ButtonEventHandlerFunct Handler;
    };

    enum ButtonAction
    {
        kButtonAction_Press             = 0,
        kButtonAction_Release,
        kButtonAction_LongPress,
        kButtonAction_LongPress_Release
    };

    int Init(void);
    void Poll(void);

    // API to set long press timeout
    void SetLongPressTimeoutMs(uint16_t aLongPressTimeoutMs);

    // API to add or remove a button event handler.
    int AddButtonEventHandler(uint8_t aButtonPinNo, ButtonEventHandlerFunct Handler);
    void RemoveEventHandler(uint8_t aButtonPinNo, ButtonEventHandlerFunct handler);

private:

    friend ButtonManager & ButtonMgr(void);

    bool IsLongPressActive(uint8_t button_id);
    bool GetButtonIdFromPinNo(uint8_t aPinNo, uint8_t & aButtonId);

    void CancelLongPressTimer(uint8_t button_id);
    void DispatchButtonEvent(const AppEvent * event);
    void LongPressActivated(uint8_t button_id);
    void StartLongPressTimer(uint8_t button_id);

    static void ButtonManagerEventHandler(const AppEvent * aButtonEvent);
    static void ButtonEventHandler(uint8_t pin_no, uint8_t button_action);

    uint8_t mButtonIndex[BUTTONS_NUMBER] = BUTTONS_LIST;
    uint8_t mLongPressActivatedFlag;
    uint16_t mLongPressTimeoutMs;
    uint64_t mLongPressButtonTimerExpirationMs[BUTTONS_NUMBER];
    ButtonEventHandler_t * mAppEventHandlerList;

    static ButtonManager sButtonManager;

};

inline ButtonManager & ButtonMgr(void)
{
    return ButtonManager::sButtonManager;
}

#endif // BUTTON_MANAGER_H