#pragma once
#include "Parameter.h"

class BoolParameter : public Parameter
{
public:
    BoolParameter(const char *name, ParameterAccess access, bool initial = false)
        : Parameter(name, ParameterType::Bool, access), value(initial) {}

    bool setFromJson(const JsonVariantConst &v) override
    {
        if (!v.is<bool>())
            return false;

        set(v.as<bool>());
        return true;
    }

    void set(bool v)
    {
        if (value != v)
        {
            value = v;
            onChange();
        }
        else if (access == ParameterAccess::READ_ONLY_ALWAYS_NOTIFY || access == ParameterAccess::READ_WRITE_ALWAYS_NOTIFY)
            onChange();
    }

    bool get() const { return value; }
    //  void serialize(PacketWriter& w) const override
    // {
    //     w.boolean(value);
    // }

    // void deserialize(PacketReader& r) override
    // {
    //     value = r.boolean();
    // }
    const uint8_t *toBytes() const override
    {
        return reinterpret_cast<const uint8_t *>(&value);
    }
    size_t getSize() const override { return sizeof(value); }


private:
    bool value;
};
