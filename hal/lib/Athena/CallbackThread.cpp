#include "HAL/CallbackThread.hpp"
#include <atomic>
#include <cstdlib>
#include <mutex>

void CallbackThread::SetFunc(int32_t (*init)(void*), void (*process)(uint64_t, void*),
                             void (*end)(void*), void* param) {
  auto thr = GetThread();
  if (!thr) return;
  thr->m_param = param;
  thr->m_init = init;
  thr->m_process = process;
  thr->m_end = end;
  thr->m_notify = true;
  thr->m_cond.notify_one();
}

void* CallbackThread::GetParam() {
  auto thr = GetThread();
  if (!thr) return nullptr;
  return thr->m_param;
}

void CallbackThreadHAL::Main() {
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cond.wait(lock, [&] { return !m_active || m_notify; });
    if (!m_active) return;
    if (m_init)
    {
      auto init = m_init;
      auto param = m_param;
      lock.unlock(); // don't hold mutex during init execution
      int32_t rv = init(param);
      lock.lock();
      if (rv != 0) return;
    }
  }

  std::unique_lock<std::mutex> lock(m_mutex);
  while (m_active) {
    m_cond.wait(lock, [&] { return !m_active || m_notify; });
    if (!m_active) break;
    m_notify = false;
    if (!m_process) continue;
    auto process = m_process;
    auto param = m_param;
    uint64_t mask = m_mask;
    lock.unlock();  // don't hold mutex during callback execution
    process(mask, param);
    lock.lock();
  }
  
  if (m_end)
  {
    auto end = m_end;
    auto param = m_param;
    lock.unlock(); // don't hold mutex during end execution
    end(param);
    lock.lock();
  }
}

void callbackHandler(uint64_t mask, void* param) {
  ((CallbackThread*)param)->Notify(mask);
}