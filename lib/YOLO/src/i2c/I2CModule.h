#pragma once
#include "common/Module.h"
#include <Wire.h>
#include "I2CSensor.h"

class I2CModule : public Module<MODULE_ID::I2C, I2CSensor>
{
public:
    I2CModule();
    void refresh() override {}

    void loadConfig(JsonObject const &config) override;

protected:
};
