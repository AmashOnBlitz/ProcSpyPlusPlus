#pragma once
#include <queue>
#include <string>
#include <mutex>
#include <atomic>
#include "Flags.h"

namespace messenger {
    inline std::queue<std::string> queue;
    inline std::atomic<int> writeOffSet{ 0 };
    inline std::atomic<int> readOffSet{ 0 };
    inline std::mutex queueMutex;
    inline void PutMessage(const std::string& message) {
        InMemHookLogging = true;
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            queue.push(message);
            writeOffSet++;
        }
        InMemHookLogging = false;
    }
    inline std::string ReadMessage() {
        std::string msg;
        InMemHookLogging = true;
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (!queue.empty()) {
                msg = std::move(queue.front());
                queue.pop();
                readOffSet++;
            }
        }
        InMemHookLogging = false;
        return msg;
    }
}