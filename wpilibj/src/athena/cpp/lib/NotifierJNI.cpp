/*----------------------------------------------------------------------------*/
/* Copyright (c) FIRST 2016. All Rights Reserved.                             */
/* Open Source Software - may be modified and shared by FRC teams. The code   */
/* must be accompanied by the FIRST BSD license file in the root directory of */
/* the project.                                                               */
/*----------------------------------------------------------------------------*/

#include <jni.h>
#include <assert.h>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <stdio.h>
#include "Log.hpp"
#include "edu_wpi_first_wpilibj_hal_NotifierJNI.h"
#include "HAL/Notifier.hpp"
#include "HALUtil.h"
#include "SafeThread.h"

// set the logging level
TLogLevel notifierJNILogLevel = logWARNING;

#define NOTIFIERJNI_LOG(level) \
    if (level > notifierJNILogLevel) ; \
    else Log().Get(level)

struct NotifierData {
  JNIEnv *env;
  jobject func = nullptr;
  jmethodID mid;
};

static int32_t notifierInit(void* param) {
  if (!param) return 1;
  NotifierData* data = (NotifierData*)param;
  JNIEnv *env;
  JavaVMAttachArgs args;
  args.version = JNI_VERSION_1_2;
  args.name = const_cast<char*>("Notifier");
  args.group = nullptr;
  jint rs = jvm->AttachCurrentThreadAsDaemon((void**)&env, &args);
  if (rs != JNI_OK) return 1;
  data->env = env;
  return 0;
}

static void notifierProcess(uint64_t currentTimeInt, void* param) {
  if (!param) return;
  NotifierData* data = (NotifierData*)param;
  if (!data->func) return;
  JNIEnv* env = data->env;
  env->CallVoidMethod(data->func, data->mid, (jlong)currentTimeInt);
  if (env->ExceptionCheck()) {
    env->ExceptionDescribe();
    env->ExceptionClear();
  }
}

static void notifierEnd(void* param) {
  if (param) {
    NotifierData* data = (NotifierData*)param;
    if (data->func) data->env->DeleteGlobalRef(data->func);
  }

  jvm->DetachCurrentThread();
}

extern "C" {

/*
 * Class:     edu_wpi_first_wpilibj_hal_NotifierJNI
 * Method:    initializeNotifier
 * Signature: (Ljava/lang/Runnable;)J
 */
JNIEXPORT jlong JNICALL Java_edu_wpi_first_wpilibj_hal_NotifierJNI_initializeNotifier
  (JNIEnv *env, jclass, jobject func)
{
  NOTIFIERJNI_LOG(logDEBUG) << "Calling NOTIFIERJNI initializeNotifier";

  jclass cls = env->GetObjectClass(func);
  if (cls == 0) {
    NOTIFIERJNI_LOG(logERROR) << "Error getting java class";
    assert(false);
    return 0;
  }
  jmethodID mid = env->GetMethodID(cls, "apply", "(J)V");
  if (mid == 0) {
    NOTIFIERJNI_LOG(logERROR) << "Error getting java method ID";
    assert(false);
    return 0;
  }

  // each notifier runs in its own thread; this is so if one takes too long
  // to execute, it doesn't keep the others from running
  NotifierData* notify = new NotifierData;
  notify->func = env->NewGlobalRef(func);
  notify->mid = mid;
  int32_t status = 0;
  void *notifierPtr = initializeNotifierThreaded(notifierInit, notifierProcess, notifierEnd, notify, &status);

  NOTIFIERJNI_LOG(logDEBUG) << "Notifier Ptr = " << notifierPtr;
  NOTIFIERJNI_LOG(logDEBUG) << "Status = " << status;

  if (!notifierPtr || !CheckStatus(env, status)) {
    // something went wrong in HAL, clean up
    delete notify;
  }

  return (jlong)notifierPtr;
}

/*
 * Class:     edu_wpi_first_wpilibj_hal_NotifierJNI
 * Method:    cleanNotifier
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_edu_wpi_first_wpilibj_hal_NotifierJNI_cleanNotifier
  (JNIEnv *env, jclass, jlong notifierPtr)
{
  NOTIFIERJNI_LOG(logDEBUG) << "Calling NOTIFIERJNI cleanNotifier";

  NOTIFIERJNI_LOG(logDEBUG) << "Notifier Ptr = " << (void *)notifierPtr;
  
  

  int32_t status = 0;
  //NotifierJNI* notify =
      //(NotifierJNI*)getNotifierParam((void*)notifierPtr, &status);
  void* d = cleanNotifierThreaded((void*)notifierPtr, &status);
  NOTIFIERJNI_LOG(logDEBUG) << "Status = " << status;
  CheckStatus(env, status);
  if (!d) return;
  NotifierData* data = (NotifierData*)d;
  delete data;
}

/*
 * Class:     edu_wpi_first_wpilibj_hal_NotifierJNI
 * Method:    updateNotifierAlarm
 * Signature: (JJ)V
 */
JNIEXPORT void JNICALL Java_edu_wpi_first_wpilibj_hal_NotifierJNI_updateNotifierAlarm
  (JNIEnv *env, jclass cls, jlong notifierPtr, jlong triggerTime)
{
  NOTIFIERJNI_LOG(logDEBUG) << "Calling NOTIFIERJNI updateNotifierAlarm";

  NOTIFIERJNI_LOG(logDEBUG) << "Notifier Ptr = " << (void *)notifierPtr;

  NOTIFIERJNI_LOG(logDEBUG) << "triggerTime = " << triggerTime;

  int32_t status = 0;
  updateNotifierAlarm((void*)notifierPtr, (uint64_t)triggerTime, &status);
  NOTIFIERJNI_LOG(logDEBUG) << "Status = " << status;
  CheckStatus(env, status);
}

/*
 * Class:     edu_wpi_first_wpilibj_hal_NotifierJNI
 * Method:    stopNotifierAlarm
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_edu_wpi_first_wpilibj_hal_NotifierJNI_stopNotifierAlarm
  (JNIEnv *env, jclass cls, jlong notifierPtr)
{
  NOTIFIERJNI_LOG(logDEBUG) << "Calling NOTIFIERJNI stopNotifierAlarm";

  NOTIFIERJNI_LOG(logDEBUG) << "Notifier Ptr = " << (void *)notifierPtr;

  int32_t status = 0;
  stopNotifierAlarm((void*)notifierPtr, &status);
  NOTIFIERJNI_LOG(logDEBUG) << "Status = " << status;
  CheckStatus(env, status);
}

}  // extern "C"
