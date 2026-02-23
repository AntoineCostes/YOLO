#include "Component.h"

Component::Component(const char *name, uint32_t refreshMs, bool serialDebug) : name(name), refreshTimer(refreshMs, true)
{
    initializedParam = new BoolParameter("initialized", ParamAccess::READ_ONLY, false);
    registerParam(initializedParam);
    serialDebugParam = new BoolParameter("serialDebug", ParamAccess::WRITE_ONLY, true);
    registerParam(serialDebugParam);

    refreshTimer.setCallback(std::bind(&Component::refresh, this));
    refreshTimer.start();
}

void Component::update()
{
    refreshTimer.update();
}

void Component::registerParam(Parameter *param)
{
    for (auto p : parameters)
    {
        if (strcmp(p->getName(), param->getName()) == 0)
        {
            err("Parameter already exists: %s", param->getName());
            return;
        }
    }
    parameters.push_back(param);
}

Parameter* Component::getParam(const char *name)
{
    for (auto p : parameters)
        if (strcmp(p->getName(), name) == 0)
            return p;
    return nullptr;
}

void Component::dbg(const char *fmt, ...)
{
    if (!serialDebugParam->get())
        return;

    Serial.print("[");
    Serial.print(name);
    Serial.print("] ");
    va_list args;
    va_start(args, fmt);
    char buffer[128];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    Serial.println(buffer);
    va_end(args);
}

void Component::log(const char *fmt, ...)
{
    Serial.print("[");
    Serial.print(name);
    Serial.print("] ");
    // Serial.println(message);
    va_list args;
    va_start(args, fmt);
    char buffer[128];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    Serial.println(buffer);
    va_end(args);
}

void Component::err(const char *fmt, ...)
{

    Serial.print("[");
    Serial.print(name);
    Serial.print(" ERROR] ");
    // Serial.println(message);
    va_list args;
    va_start(args, fmt);
    char buffer[128];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    Serial.println(buffer);
    va_end(args);
}
