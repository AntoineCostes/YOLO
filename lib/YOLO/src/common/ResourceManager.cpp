#include "ResourceManager.h"

ResourceManager &ResourceManager::instance()
{
    static ResourceManager _instance;
    return _instance;
}

bool ResourceManager::isFree(uint8_t pin) const
{
    return !gpioUsed[pin];
}

bool ResourceManager::reserveGPIO(uint8_t pin)
{
    if (!BoardTraits::isValidGPIO(pin))
        return false;
    if (gpioUsed[pin])
        return false;

    gpioUsed.set(pin);
    return true;
}
