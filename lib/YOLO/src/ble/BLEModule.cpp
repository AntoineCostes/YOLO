#include "BLEModule.h"


BLEModule::BLEModule() : Module<Component>("ble"),client(nullptr),server(nullptr)
{
}

void BLEModule::refresh()
{
}

void BLEModule::loadConfig(JsonObject const &config)
{
    Module::loadConfig(config);

    for (JsonPair kv : config)
        if (kv.value().is<JsonObject>())
        {
            if (kv.key() == "client")
            {
                client = new YoloClient("BLEclient");
                registerComponent(client);
            }
            if (kv.key() == "server")
            {
                server = new YoloServer("BLEserver");
                registerComponent(server);
            }
        }
}

void BLEModule::start(const uint8_t* configData, size_t configDataSize)
{
    log("START");
    if (server)
        server->initService(configData, configDataSize);

    if (client)
    {
        client->initScan();
    }
}

void BLEModule::notify(Parameter* param)
{
    if (server)
        if (server->isConnected())
            server->notify(param);
        else err("not yet connected");
}