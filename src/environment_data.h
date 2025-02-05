#ifndef XPROFILER_SRC_ENVIRONMENT_DATA_H
#define XPROFILER_SRC_ENVIRONMENT_DATA_H

#include <functional>
#include <list>

#include "commands/cpuprofiler/cpu_profiler.h"
#include "commands/gcprofiler/gc_profiler.h"
#include "library/common.h"
#include "logbypass/gc.h"
#include "logbypass/heap.h"
#include "logbypass/http.h"
#include "logbypass/libuv.h"
#include "nan.h"
#include "xpf_mutex-inl.h"

namespace xprofiler {

enum class InterruptKind {
  kBusy,
  kIdle,
};

using InterruptCallback = std::function<void(EnvironmentData*, InterruptKind)>;

class EnvironmentData {
 public:
  static EnvironmentData* GetCurrent(v8::Isolate* isolate);
  static EnvironmentData* GetCurrent(
      const Nan::FunctionCallbackInfo<v8::Value>& info);
  // Lock-free operation to get EnvironmentData for current thread, GetCurrent
  // should be preferred in most cases.
  static EnvironmentData* TryGetCurrent();
  static void Create(v8::Isolate* isolate);

  static void JsSetupEnvironmentData(
      const Nan::FunctionCallbackInfo<v8::Value>& info);

  void SendCollectStatistics();

  void RequestInterrupt(InterruptCallback interrupt);
  void AddGCEpilogueCallback(Nan::GCEpilogueCallback callback,
                             v8::GCType gc_type_filter = v8::kGCTypeAll);
  void RemoveGCEpilogueCallback(Nan::GCEpilogueCallback callback);
  void AddGCPrologueCallback(Nan::GCPrologueCallback callback,
                             v8::GCType gc_type_filter = v8::kGCTypeAll);
  void RemoveGCPrologueCallback(Nan::GCPrologueCallback callback);

  uint64_t GetUptime() const;

  inline v8::Isolate* isolate() const { return isolate_; }
  inline uv_loop_t* loop() const { return loop_; }

  inline bool is_main_thread() const { return is_main_thread_; }
  inline ThreadId thread_id() const { return thread_id_; }
  inline std::string node_version() const { return node_version_; }

  inline GcStatistics* gc_statistics() { return &gc_statistics_; }
  inline HttpStatistics* http_statistics() { return &http_statistics_; }
  inline MemoryStatistics* memory_statistics() { return &memory_statistics_; }
  inline UvHandleStatistics* uv_handle_statistics() {
    return &uv_handle_statistics_;
  }

  std::unique_ptr<GcProfiler> gc_profiler;
  std::unique_ptr<CpuProfiler> cpu_profiler;

 private:
  static void AtExit(void* arg);
  template <uv_async_t EnvironmentData::*field>
  static void CloseCallback(uv_handle_t* handle);
  static void InterruptBusyCallback(v8::Isolate* isolate, void* data);
  static void InterruptIdleCallback(uv_async_t* handle);

  static void CollectStatistics(uv_async_t* handle);
  EnvironmentData(v8::Isolate* isolate, uv_loop_t* loop);

  const uint64_t time_origin_;

  v8::Isolate* isolate_;
  uv_loop_t* loop_;
  uv_async_t statistics_async_;

  bool is_main_thread_ = false;
  /* We don't have a native method to get the uint64_t thread id.
   * Use the JavaScript number representation.
   */
  ThreadId thread_id_ = ThreadId(-1);
  std::string node_version_ = "";

  Mutex interrupt_mutex_;
  std::list<InterruptCallback> interrupt_requests_;
  uv_async_t interrupt_async_;
  std::list<Nan::GCEpilogueCallback> gc_epilogue_callbacks_;
  std::list<Nan::GCPrologueCallback> gc_prologue_callbacks_;

  GcStatistics gc_statistics_;
  MemoryStatistics memory_statistics_;
  HttpStatistics http_statistics_;
  UvHandleStatistics uv_handle_statistics_;

  uint32_t closed_handle_count_ = 0;
  static const uint32_t kHandleCount = 2;
};

}  // namespace xprofiler

#endif /* XPROFILER_SRC_ENVIRONMENT_DATA_H */
