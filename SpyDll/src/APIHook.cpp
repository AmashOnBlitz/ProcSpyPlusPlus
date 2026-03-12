#include "pch.h"
#include "APIHook.h"

bool APIHook::setHook(const std::string& name, bool state, bool isEnabledFlag, std::string& err)
{
    if (name.empty()) {
        err = "[INTERNAL ERROR] Function Name Not Provided!";
        return false;
    }
    return updateHookTable(name, state, isEnabledFlag, err);
}

bool APIHook::updateHookTable(const std::string& name, bool state, bool isEnabledFlag, std::string& err)
{
    bool found = false;
    std::lock_guard<std::mutex> lock(APITableMutex);
    for (APIFunctionsStruct& APIFunc : APIFunctions) {
        if (APIFunc.label == name) {
            if (isEnabledFlag)
                APIFunc.funcEnabled = state;
            else
                APIFunc.debug = state;
            found = true;
            break;
        }
    }
    if (!found)
        err = "[INTERNAL ERROR] Function Not Found!";
    return found;
}