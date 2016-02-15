#include "SafeThread.h"

// Thread where callbacks are actually performed.
//
// In C++, this allows notifiers to run long. With the standard notifier
// if you had a notifier callback that took 1 second to run, no other
// notifiers would run until that callback completed. This allows it to
// run in a seperate thread, which make notifiers non blocking.
//
// For Java, JNI's AttachCurrentThread() creates a Java Thread object on every
// invocation, which is both time inefficient and causes issues with Eclipse
// (which tries to keep a thread list up-to-date and thus gets swamped).
// Instead, this class attaches just once.  When a hardware notification
// occurs, a condition variable wakes up this thread and this thread actually
// makes the callback.
//
// We don't want to use a FIFO here. If the user code takes too long to
// process, we will just ignore the redundant wakeup.
class CallbackThreadHAL : public SafeThread {
 public:
  void Main();

  bool m_notify = false;
  void* m_param = nullptr;
  int32_t (*m_init)(void*) = nullptr;
  void (*m_process)(uint64_t, void*) = nullptr;
  void (*m_end)(void*) = nullptr;
  uint64_t m_mask;
};

class CallbackThread : public SafeThreadOwner<CallbackThreadHAL> {
 public:
  void SetFunc(int32_t (*init)(void*), void (*process)(uint64_t, void*),
               void (*end)(void*), void* param);
  void* GetParam();

  void Notify(uint64_t mask) {
    auto thr = GetThread();
    if (!thr) return;
    thr->m_mask = mask;
    thr->m_notify = true;
    thr->m_cond.notify_one();
  }
};

void callbackHandler(uint64_t mask, void* param);