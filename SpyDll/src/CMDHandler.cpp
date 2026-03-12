#include "pch.h"
#include "CMDHandler.h"
#include "messageQueue.h"
#include <vector>
#include "APIHook.h"
#include "Flags.h"
#include <HookThread.h>

void extractCMD(std::string msg){
    std::vector<std::string> tokens;
    char delimiter = '|';
    size_t start = 0;
    size_t end = msg.find(delimiter);
    while (end != std::string::npos) {
        if (end > start)
            tokens.push_back(msg.substr(start, end - start));
        start = end + 1;
        end = msg.find(delimiter, start);
    }
    if (start < msg.size())
        tokens.push_back(msg.substr(start));

    if (tokens.size() < 3) return;

    const std::string& action = tokens[1];
    const std::string& label = tokens[2];

    bool state;
    bool isEnabledFlag;

    if (action == "Track") { state = true;  isEnabledFlag = false; }
    else if (action == "NoTrack") { state = false; isEnabledFlag = false; }

    else if (action == "Block") { state = false; isEnabledFlag = true; }
    else if (action == "NoBlock") { state = true;  isEnabledFlag = true; }

    else {
        messenger::PutMessage("[INTERNAL ERROR] Unknown action: " + action + "\n");
        return;
    }

    std::string err;
    if (!APIHook::setHook(label, state, isEnabledFlag, err)) {
        messenger::PutMessage("[INTERNAL ERROR] " + err + "\n");
        return;
    }

    refreshHooks = true;

    std::string str;
    str += "[System] Hook updated | ";
    str += (isEnabledFlag ? "Block" : "Debug");
    str += " = ";
    str += (state ? "true" : "false");
    str += " | Target: " + label + "\n";
    messenger::PutMessage(str);
}