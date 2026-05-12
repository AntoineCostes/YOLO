#pragma once
#include "Parameter.h"

class StringParameter : public Parameter
{
public:
    StringParameter(const char *name, ParameterAccess access, const char *initial = "")
        : Parameter(name, ParameterType::String, access)
    {
        strncpy(value, initial, sizeof(value));
        value[sizeof(value) - 1] = '\0';
    }

    bool setFromJson(const JsonVariantConst &v) override
    {
        if (!v.is<const char *>())
            return false;

        set(v.as<const char *>());
        return true;
    }

    void set(const char *v)
    {
        if (strncmp(value, v, sizeof(value)) != 0)
        {
            strncpy(value, v, sizeof(value));
            value[sizeof(value) - 1] = '\0';
            onChange();
        }
        else if (access == ParameterAccess::READ_ONLY_ALWAYS_NOTIFY || access == ParameterAccess::READ_WRITE_ALWAYS_NOTIFY)
            onChange();
    }
    // void serialize(PacketWriter &w) const override
    // {
    //     w.str(value);
    // }
    // void deserialize(PacketReader &r) override
    // {
    //     // value = r.str();
    // }

    const char *get() const { return value; }
    const uint8_t *toBytes() const override

    {
        return reinterpret_cast<const uint8_t *>(&value);
    }
    size_t getSize() const override { return sizeof(value); }

private:
    char value[64]; // fixed buffer size
};
