#ifndef APP_TASK_H
#define APP_TASK_H

#include <stdint.h>
#include <stdbool.h>

#include "LockWidget.h"
#include "LEDWidget.h"

struct AppActionEvent
{
    LockWidget::Action_t Action;
    int32_t Actor;
};

class AppTask
{
public:
	int Init();
	static void AppTaskMain(void * pvParameter);
    void PostLockActionRequest(int32_t aActor, LockWidget::Action_t aAction);

private:

	friend AppTask & GetAppTask(void);

    int InitRoutine();

    static void ActionInitiated(LockWidget::Action_t aAction, int32_t aActor);
    static void ActionCompleted(LockWidget::Action_t aAction);

	static void FunctionTimerEventHandler(void * p_context);
	static void FunctionButtonHandler(uint8_t pin_no, uint8_t button_action);
	static void LockButtonEventHandler(uint8_t pin_no, uint8_t button_action);

    void LockActionEventHandler(AppActionEvent *aEvent);

	void CancelTimer(void);
	void StartTimer(uint32_t aTimeoutInMs);
	void Alive(void);

	enum Function_t {
		kFunction_NoneSelected = 0,
		kFunction_SoftwareUpdate = 0,
		kFunction_FactoryReset,

		kFunction_Invalid
	} Function;

	Function_t mFunction;
	bool mFunctionTimerActive;

	static AppTask sAppTask;
};

inline AppTask & GetAppTask(void)
{
	return AppTask::sAppTask;
}

#endif // APP_TASK_H
