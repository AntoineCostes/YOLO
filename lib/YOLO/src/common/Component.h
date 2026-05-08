#pragma once
#include "Dependencies.h"
#include "util/Timer.h"
#include "parameters/BoolParameter.h"
#include "parameters/ByteParameter.h"
#include "parameters/IntParameter.h"
#include "parameters/FloatParameter.h"
#include "parameters/StringParameter.h"

#include <functional>

template<typename... Args>
using EventCallback = std::function<void(Args...)>;

class Component
{
public:
    Component(const char* name, uint32_t refreshMs = 20, bool serialDebug = true);
    const char* getName() const { return name; }
    virtual void update(); // to be called as often as possible

    bool isInitialized() { return initializedParam->get();}
    
    void registerParam(Parameter* param);
    Parameter* getParam(const char* name);
    
    std::vector<Parameter*> getParameters() const {return parameters;}

protected:
    Timer refreshTimer;
    virtual void refresh() = 0;

    BoolParameter* initializedParam;
    BoolParameter* serialDebugParam;

    void dbg(const char* fmt, ...);
    void log(const char* fmt, ...);
    void err(const char* fmt, ...);
    
private:
    const char* name;
    std::vector<Parameter*> parameters;
};
