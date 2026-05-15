#include "I2CModule.h"

I2CModule::I2CModule() : Module<MODULE_ID::I2C, I2CSensor>("i2c")
{
    Wire.begin();
}

void I2CModule::loadConfig(JsonObject const &config)
{
    Module::loadConfig(config);

    for (JsonPair kv : config)
        if (kv.value().is<JsonObject>())
        {
            if (kv.key() == "nox")
            {
                registerComponent(new NoxProbe(STC3X_BINARY_GAS_CO2_AIR_100, true));
            }
        }
}
