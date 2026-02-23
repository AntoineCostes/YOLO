#pragma once
#include "common/Module.h"
#include "ServoMotor.h"

class ServoModule : public Module<ServoMotor>
{
public:
    ServoModule();
    void refresh() override {}
    static constexpr uint8_t uuid = 0x00; 
    uint8_t getModuleID() const override { return uuid;  }

    void loadConfig(JsonObject const &config) override;

    bool registerServo(const char* name, uint8_t pin, float min, float max, bool inverse);

    void set(uint8_t index, float value);
    void move(uint8_t index, float value, float durationSec);

protected:
    void registerServo(const char* name, JsonObject const &config);
    
};