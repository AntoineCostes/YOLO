#pragma once
#include "Parameter.h"

class ByteParameter : public Parameter
{
public:
    ByteParameter(const char *name, ParamAccess access, byte value)
        : Parameter(name, ParameterType::Byte, access), value(value) {}

    bool setFromJson(const JsonVariantConst &v) override
    {
        if (!v.is<short>())
            return false;

        set(v.as<short>());
        return true;
    }

    void set(byte v)
    {
        if (v > 255)
        {
            Serial.println("ERROR int value too high");
            return;
        }
        if (v < 0)
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

private:
    byte value;
};
