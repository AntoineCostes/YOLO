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

enum class ParamAccess : uint8_t
{
    READ_ONLY,
    READ_ONLY_ALWAYS_NOTIFY, // notify set() even if value didn't change
    WRITE_ONLY,
    READ_WRITE,
    READ_WRITE_ALWAYS_NOTIFY  // notify set() even if value didn't change
};
#pragma once
#include <vector>
#include <string>
#include <cstring>

struct PacketWriter
{
    std::vector<uint8_t> data;

    void u8(uint8_t v)
    {
        data.push_back(v);
    }

    void i32(int32_t v)
    {
        uint8_t* p = reinterpret_cast<uint8_t*>(&v);
        data.insert(data.end(), p, p + sizeof(v));
    }

    void f32(float v)
    {
        uint8_t* p = reinterpret_cast<uint8_t*>(&v);
        data.insert(data.end(), p, p + sizeof(v));
    }

    void boolean(bool v)
    {
        u8(v ? 1 : 0);
    }

    void str(const std::string& s)
    {
        u8((uint8_t)s.size());

        const uint8_t* p =
            reinterpret_cast<const uint8_t*>(s.data());

        data.insert(data.end(), p, p + s.size());
    }
};

struct PacketReader
{
    const uint8_t* data;
    size_t pos = 0;

    uint8_t u8()
    {
        return data[pos++];
    }

    int32_t i32()
    {
        int32_t v;
        memcpy(&v, data + pos, sizeof(v));
        pos += sizeof(v);
        return v;
    }

    float f32()
    {
        float v;
        memcpy(&v, data + pos, sizeof(v));
        pos += sizeof(v);
        return v;
    }

    bool boolean()
    {
        return u8() != 0;
    }

    std::string str()
    {
        uint8_t len = u8();

        std::string s(
            reinterpret_cast<const char*>(data + pos),
            len
        );

        pos += len;

        return s;
    }
};

class Parameter
{
public:
    using ChangeCallback = void (*)(void *context, Parameter *);

    Parameter(const char *name, ParameterType type, ParamAccess access, bool alwaysNotify = true) : name(name),
                                                                          type(type),
                                                                          access(access),
                                                                          alwaysNotify(alwaysNotify),
                                                                          callback(nullptr),
                                                                          context(nullptr) {}
    virtual ~Parameter() {}
    virtual bool setFromJson(const JsonVariantConst &v) = 0;

    const char *getName() const { return name; }
    ParameterType getType() const { return type; }
    ParamAccess getAccess() const { return access; }

    // virtual const uint8_t *toBytes() const = 0;
    // virtual size_t getSize() const = 0;
    // virtual bool setFromBytes(const uint8_t* data, size_t len) = 0;

    void setChangeCallback(ChangeCallback cb, void *ctx)
    {
        callback = cb;
        context = ctx;
    }
    virtual void serialize(PacketWriter&) const = 0;
    virtual void deserialize(PacketReader&) = 0;

protected:
    const char *name;
    ParameterType type;
    ParamAccess access;
    bool alwaysNotify;
    ChangeCallback callback;
    void *context;

    void onChange()
    {
        if (callback && context && access != ParamAccess::WRITE_ONLY)
            callback(context, this);
    }
};
