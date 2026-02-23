#pragma once
#include "common/Component.h"

class I2CSensor : public Component
{
public:
    I2CSensor(const char* name, uint32_t refreshMs, uint8_t address);
    
protected:
};

#include "SparkFun_STC3x_Arduino_Library.h"
#include "Adafruit_SHTC3.h"
#include "SparkFun_MS5637_Arduino_Library.h"
class NoxProbe : public I2CSensor
{
public:
    NoxProbe(STC3X_binary_gas_type_e gasType = STC3X_BINARY_GAS_CO2_AIR_100, bool debug = false);
    void refresh() override;

    FloatParameter* temperatureParam;
    FloatParameter* humidityParam;
    IntParameter* pressureParam;
    FloatParameter* co2Param;
    
protected:
    STC3x stc;
    Adafruit_SHTC3 shtc = Adafruit_SHTC3();
    MS5637 ms5637;
};
