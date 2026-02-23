#pragma once
#include "common/Module.h"
#include <Wire.h>
#include "I2CSensor.h"

class I2CModule : public Module<I2CSensor>
{
public:
    I2CModule();
    void refresh() override {}
    static constexpr uint8_t uuid = 0x00;
    uint8_t getModuleID() const override { return uuid; }

    void loadConfig(JsonObject const &config) override;

protected:
};
