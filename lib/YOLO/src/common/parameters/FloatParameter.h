#pragma once
#include "Parameter.h"

class FloatParameter : public Parameter
{
public:
    FloatParameter(const char *name, ParameterAccess access, float value, float min = 0.0f, float max = 1.0f)
        : Parameter(name, ParameterType::Float, access), value(value), min(min), max(max) {}

    bool setFromJson(const JsonVariantConst &v) override
    {
        if (!v.is<float>())
            return false;

        set(v.as<float>());
        return true;
    }

    void set(float v)
    {
        if (v > max)
        {
            Serial.println("ERROR float value too high");
            return;
        }
        if (v < min)
        {
            Serial.println("ERROR float value too low");
            return;
        }
        if (value != v)
        {
            value = v;
            onChange();
        }
        else if (access == ParameterAccess::READ_ONLY_ALWAYS_NOTIFY || access == ParameterAccess::READ_WRITE_ALWAYS_NOTIFY)
            onChange();
    }

    void setMin(float v) { min = v; }
    void setMax(float v) { max = v; }

    float get() const { return value; }
    // void serialize(PacketWriter& w) const override
    // {
    //     w.f32(value);
    // }
    // void deserialize(PacketReader& r) override
    // {
    //     value = r.f32();
    // }

    const uint8_t *toBytes() const override
    {
        return reinterpret_cast<const uint8_t *>(&value);
    }
    size_t getSize() const override { return sizeof(value); }










private:
    float value;
    float min;
    float max;
};
