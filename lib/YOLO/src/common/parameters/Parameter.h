#pragma once
#include <Arduino.h>

enum class ParameterType : uint8_t
{
    Bool,
    Byte,
    Int,
    Float,
    String
};

enum class ParameterAccess : uint8_t
{
    READ_ONLY,
    READ_ONLY_ALWAYS_NOTIFY, // notify set() even if value didn't change
    WRITE_ONLY,
    READ_WRITE,
    READ_WRITE_ALWAYS_NOTIFY // notify set() even if value didn't change
};

class Parameter
{
public:
    using ChangeCallback = void (*)(void *context, Parameter *);

    Parameter(const char *name, ParameterType type, ParameterAccess access, bool alwaysNotify = true) : name(name),
                                                                                                        type(type),
                                                                                                        access(access),
                                                                                                        alwaysNotify(alwaysNotify),
                                                                                                        callback(nullptr),
                                                                                                        context(nullptr) {}
    virtual ~Parameter() {}
    virtual bool setFromJson(const JsonVariantConst &v) = 0;

    const char *getName() const { return name; }
    ParameterType getType() const { return type; }
    ParameterAccess getAccess() const { return access; }
    
    // virtual void serialize(PacketWriter &) const = 0;
    // virtual void deserialize(PacketReader &) = 0;

    virtual const uint8_t *toBytes() const = 0;
    virtual size_t getSize() const = 0;
    // virtual bool setFromBytes(const uint8_t* data, size_t len) = 0;

    void setChangeCallback(ChangeCallback cb, void *ctx)
    {
        callback = cb;
        context = ctx;
    }

protected:
    const char *name;
    ParameterType type;
    ParameterAccess access;
    bool alwaysNotify;
    ChangeCallback callback;
    void *context;

    void onChange()
    {
        if (callback && context && access != ParameterAccess::WRITE_ONLY)
            callback(context, this);
    }
};
