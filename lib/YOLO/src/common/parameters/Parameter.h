#pragma once
#include <Arduino.h>

enum class ParameterType : uint8_t
{
    Bool = 0x01,
    Byte = 0x02,
    Int = 0x03,
    Float = 0x04,
    String = 0x05
};

enum class ParameterAccess : uint8_t
{
    READ_ONLY = 0x01,
    READ_ONLY_ALWAYS_NOTIFY = 0x02, // notify set() even if value didn't change
    WRITE_ONLY = 0x03,
    READ_WRITE = 0x04,
    READ_WRITE_ALWAYS_NOTIFY = 0x05 // notify set() even if value didn't change
};

struct PacketWriter {
  uint8_t* buf;
  size_t pos = 0;
  size_t capacity;
  bool ok = true;

  PacketWriter(uint8_t* b, size_t c)
    : buf(b), capacity(c) {}

  void u8(uint8_t v)  {
  if (!ok || pos + 1 > capacity) {
    ok = false;
    return;
  }
  buf[pos++] = v;
}

void bytes(const uint8_t* data, size_t len) {
  if (!ok || pos + len > capacity) {
    ok = false;
    return;
  }
  memcpy(buf + pos, data, len);
  pos += len;
}

  void str(const char* s) {
  uint8_t len = strlen(s);
  if (!ok || pos + 1 + len > capacity) {
    ok = false;
    return;
  }
  u8(len);
  bytes((const uint8_t*)s, len);
}

  size_t size() const { return pos; }
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
    
    void serializeMetadata(PacketWriter& w) const {
    // w.str(name);
    // w.str("helloworld123456789");
    w.u8((uint8_t)type);
    w.u8((uint8_t)access);
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
