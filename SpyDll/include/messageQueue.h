#pragma once
#include <queue>
#include <string>
#include <mutex>
#include <atomic>

namespace messenger {

    inline std::queue<std::string> queue;
    inline std::atomic<int> writeOffSet{ 0 };
    inline std::atomic<int> readOffSet{ 0 };
    inline std::mutex queueMutex;  

    inline void PutMessage(const std::string& message) {
        std::lock_guard<std::mutex> lock(queueMutex);
        queue.push(message);   
        writeOffSet++;         
    }

    inline std::string ReadMessage() {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (queue.empty()) return "";
        std::string msg = queue.front();
        queue.pop();
        readOffSet++;        
        return msg;
    }
}
