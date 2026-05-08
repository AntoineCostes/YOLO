#include "ServoModule.h"

ServoModule::ServoModule() : Module<ServoMotor>("servos")
{
}

void ServoModule::loadConfig(JsonObject const &config)
{
    Module::loadConfig(config);

    for (JsonPair kv : config)
        if (kv.value().is<JsonObject>())
            registerServo(kv.key().c_str(), config[kv.key()]);
}

void ServoModule::registerServo(const char* name, JsonObject const &config)
{
    int pin = config["pin"] | -1;
    float min = config["min"] | static_cast<float>(0.0);
    float max = config["max"] | static_cast<float>(1.0);
    float start = config["start"] | static_cast<float>(0.5);
    bool inverse = config["inverse"] | false;
    
    if (pin >= 0 && min >= 0.0 && max >= 0.0)
    {
        if (registerServo(name, pin, min, max, inverse))
            set(typedComponents.size() - 1, start);
    }
    else
        err("cannot register servo");//, pin (" + String(pin) + "), min (" + String(min) + ") and max (" + String(max) + ") should be positive !");
}

bool ServoModule::registerServo(const char* name, uint8_t pin, float min, float max, bool inverse)
{
    return registerComponent(new ServoMotor(name, pin, min, max, inverse), std::set<uint8_t>{pin});
}

void ServoModule::set(uint8_t index, float value)
{
    if (index < 0 || index >= typedComponents.size())
    {
        err("invalid servo index: %i it should be between 0 and %i", index, typedComponents.size() - 1);
        return;
    }
    dynamic_cast<ServoMotor*>(typedComponents[index])->goTo(value);
}

void ServoModule::move(uint8_t index, float value, float durationSec)
{
    if (index < 0 || index >= typedComponents.size())
    {
        err("invalid servo index: %i it should be between 0 and %i", index, typedComponents.size() - 1);
        return;
    }
    dynamic_cast<ServoMotor*>(typedComponents[index])->goTo(value, durationSec * 1000);
}