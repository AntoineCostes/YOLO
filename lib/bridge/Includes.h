#pragma once

#include <Arduino.h>
#include <string>
#include <vector>
#include <variant>
#include <functional>

#define YOLO_SERVICE_UUID        "b91bb233-0000-4de7-97cb-ae80cc439000"
#define YOLO_QUERY_UUID         "b91bb233-0001-4de7-97cb-ae80cc439000"
#define YOLO_RESPONSE_UUID          "b91bb233-0002-4de7-97cb-ae80cc439000"
#define YOLO_STATE_UUID          "b91bb233-0003-4de7-97cb-ae80cc439000"
#define YOLO_CONTROL_UUID        "b91bb233-0004-4de7-97cb-ae80cc439000"

using Variant = std::variant<
    bool,
    int32_t,
    float,
    std::string
>;

enum class ParamType : uint8_t
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

// Remote structs

struct RemoteParam
{
    uint8_t moduleId;
    uint8_t componentIndex;
    uint8_t paramIndex;

    std::string name;

    ParamType type;
    ParamAccess access;

    Variant value;
};

struct RemoteComponent
{
    uint8_t index;

    std::string name;

    std::vector<RemoteParam> params;
};

struct RemoteModule
{
    uint8_t moduleId;

    std::string name;

    std::vector<RemoteParam> params;

    std::vector<RemoteComponent> components;
};

struct RemoteDevice
{
    std::string name;

    std::vector<RemoteModule> modules;
};

// Serialization

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