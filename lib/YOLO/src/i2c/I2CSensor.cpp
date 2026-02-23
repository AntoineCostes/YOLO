#include "I2CSensor.h"

I2CSensor::I2CSensor(const char *name, uint32_t refreshMs, uint8_t address) : Component(name, refreshMs)
{
    initializedParam->set(true);
}

NoxProbe::NoxProbe(STC3X_binary_gas_type_e gasType, bool debug) : I2CSensor("nox", 10000, STC3x_DEFAULT_ADDRESS)
{
    initializedParam->set(false);

    temperatureParam = new FloatParameter("temperature", ParamAccess::READ_ONLY_ALWAYS_NOTIFY, 0.0f, -50.f, 100.0f);
    registerParam(temperatureParam);
    humidityParam = new FloatParameter("humidity", ParamAccess::READ_ONLY_ALWAYS_NOTIFY, 0.0f, 0.f, 100.0f);
    registerParam(humidityParam);
    pressureParam = new IntParameter("pressure", ParamAccess::READ_ONLY_ALWAYS_NOTIFY, 0, -1000, 2000);
    registerParam(pressureParam);
    co2Param = new FloatParameter("co2", ParamAccess::READ_ONLY_ALWAYS_NOTIFY, 0.0f, 0.f, 100.0f);
    registerParam(co2Param);

    if (debug)
        stc.enableDebugging();

    if (!stc.begin())
        return;

    if (!stc.setBinaryGas(gasType))
        return;

    if (!shtc.begin())
        return;

    if (!ms5637.begin())
        return;

    initializedParam->set(true);
}

void NoxProbe::refresh()
{
    if (!initializedParam->get())
        return;

    if (stc.measureGasConcentration())
    {
        sensors_event_t humidity, temp;
        shtc.getEvent(&humidity, &temp);

        float temperature = temp.temperature;
        if (stc.setTemperature(temperature))
        {
            float RH = humidity.relative_humidity;
            if (stc.setRelativeHumidity(RH))
            {
                uint16_t pressure = (uint16_t)ms5637.getPressure();
                if (stc.setPressure((uint16_t)pressure))
                {
                    float co2 = stc.getCO2();

                    temperatureParam->set(temperature);
                    humidityParam->set(RH);
                    pressureParam->set(pressure);
                    co2Param->set(co2);
                }
            }
            else
                err("could not set RH");
        }
        else
            err("could not set RH");
    }
    else
        err("sensor not ready !");
}