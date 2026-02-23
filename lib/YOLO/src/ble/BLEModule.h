#pragma once
#include "common/Module.h"
#include "YoloServer.h"
#include "YoloClient.h"


class BLEModule : public Module<Component>
{
public:

  BLEModule();
  static constexpr uint8_t uuid = 0x00;
  uint8_t getModuleID() const override { return uuid; }

  void refresh() override;

  void loadConfig(JsonObject const &config) override;
  void start(const uint8_t* configData, size_t configDataSize);

  void notify(Parameter *param);

  YoloClient *client;

protected:
  BoolParameter *isServerConnected;
  YoloServer *server;
};

