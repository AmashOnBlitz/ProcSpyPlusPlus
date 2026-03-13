#pragma once
#include <atomic>
inline std::atomic<bool> ThreadExpectedToStop{ false };
inline std::atomic<bool> MessagePipelineAllocMem{ false };
inline thread_local bool InMemHookLogging{ false };