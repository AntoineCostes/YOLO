#pragma once
#include "Component.h"
#include "ResourceManager.h"

class IModule : public Component
{
public:
    IModule(const char *name, bool serialDebug = false)
        : Component(name, serialDebug) {}
    virtual ~IModule() {}

    virtual void loadConfig(JsonObject const &) = 0;
    virtual const std::vector<Component *> &getComponents() const = 0;
    virtual uint8_t getModuleID() const = 0;
};

template <typename TComponent>
class Module : public IModule
{
public:
    Module(const char *name, bool serialDebug = false)
        : IModule(name, serialDebug) {}

    void update() override
    {
        Component::update();
        for (auto comp : typedComponents)
            comp->update();
    }

    const std::vector<TComponent *> &getTypedComponents() const { return typedComponents; }
    const std::vector<Component *> &getComponents() const override { return untypedComponents; }

    TComponent *getComponent(uint8_t index)
    {
        return (index < typedComponents.size()) ? typedComponents[index] : nullptr;
    }

    TComponent *getComponent(const char *name)
    {
        for (auto c : typedComponents)
            if (strcmp(c->getName(), name) == 0)
                return c;
        return nullptr;
    }

    bool registerComponent(TComponent *comp, std::set<uint8_t> requiredPins = {})
    {
        for (uint8_t pin : requiredPins)
        {
            if (!RM.isFree(pin))
            {
                err("Failed to register %s, pin #%i is not available !", comp->getName(), pin);
                return false;
            }
        }
        for (uint8_t pin : requiredPins)
            RM.reserveGPIO(pin);

        if (addComponent(comp))
        {
            if (requiredPins.size())
            {
                Serial.printf("-- %s registered on pins:\n", comp->getName());
                for (auto pin : requiredPins)
                {
                    Serial.print(" #");
                    Serial.print(pin);
                }
                Serial.println();
            }
            else
                Serial.printf("-- %s registered\n", comp->getName());
            return true;
        }
        err("Failed to register %s, this component exists already !", comp->getName());
        return false;
    }

    void loadConfig(JsonObject const &config)
    {
        if (config)
            dbg("load config");

        // object entries are Components, to be registered by children classes
        // set parameters from non-object entries
        for (JsonPair kv : config)
            if (!kv.value().is<JsonObject>())
            {
                const char *key = kv.key().c_str();
                const JsonVariantConst value = kv.value();

                Parameter *p = getParam(kv.key().c_str());
                if (p == nullptr)
                    err("Parameter '%s' not found", key);
                else if (!p->setFromJson(value))
                    err("Invalid type for parameter '%s'", key);
            }

        initializedParam->set(true);
    }

protected:
    std::vector<TComponent *> typedComponents;
    std::vector<Component *> untypedComponents;
    bool addComponent(TComponent *comp)
    {
        for (auto c : typedComponents)
            if (strcmp(c->getName(), comp->getName()) == 0)
                return false;

        for (auto c : untypedComponents)
            if (strcmp(c->getName(), c->getName()) == 0)
                return false;

        typedComponents.push_back(comp);
        untypedComponents.push_back(comp);
        return true;
    }
};