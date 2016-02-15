#pragma once

#include <stdint.h>

extern "C"
{
	void* initializeNotifier(void (*process)(uint64_t, void*), void* param, int32_t *status);
	void cleanNotifier(void* notifier_pointer, int32_t *status);
  void* initializeNotifierThreaded(int32_t (*init)(void*), void (*process)(uint64_t, void*), void (*end)(void*), void *param, int32_t *status);
  void* cleanNotifierThreaded(void* notifier_pointer, int32_t *status);
	void* getNotifierParam(void* notifier_pointer, int32_t *status);
	void updateNotifierAlarm(void* notifier_pointer, uint64_t triggerTime, int32_t *status);
	void stopNotifierAlarm(void* notifier_pointer, int32_t *status);
}
