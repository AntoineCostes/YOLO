#pragma once
#include "Parameter.h"

class IntParameter : public Parameter
{
public:
    IntParameter(const char *name, ParamAccess access, int value, int min, int max)
        : Parameter(name, ParameterType::Int, access), value(value), min(min), max(max) {}

    bool setFromJson(const JsonVariantConst &v) override
    {
        if (!v.is<int>())
            return false;

        set(v.as<int>());
        return true;
    }

    void set(int v)
    {
        if (v > max)
        {
            Serial.println("ERROR int value too high");
            return;
        }
        if (v < min)
        {
            Serial.println("ERROR int value too low");
            return;
        }
        if (value != v)
        {
            value = v;
            onChange();
        }
        else if (access == ParamAccess::READ_ONLY_ALWAYS_NOTIFY || access == ParamAccess::READ_WRITE_ALWAYS_NOTIFY)
            onChange();
    }
    int get() const { return value; }
    // const uint8_t *toBytes() const override
    // {
    //     return reinterpret_cast<const uint8_t *>(&value);
    // }
    // size_t getSize() const override { return sizeof(value); }

    void serialize(PacketWriter& w) const override
    {
        w.i32(value);
    }
    void deserialize(PacketReader& r) override
    {
        value = r.i32();
    }

private:
    int value;
    int min;
    int max;
};
