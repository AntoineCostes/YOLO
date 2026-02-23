#pragma once
#include "Dependencies.h"

#define RM ResourceManager::instance()

#if defined(CONFIG_IDF_TARGET_ESP32C3)
struct BoardTraits {

    static constexpr int GPIO_COUNT = 22;
    static constexpr int PWM_CHANNEL_COUNT = 6;

    inline static const std::set<uint8_t> validGPIO = {
        0,1,2,3,4,5,6,7,8,9,
        10,18,19,20,21
    };

    static bool isValidGPIO(uint8_t pin) {
        return validGPIO.find(pin) != validGPIO.end();
    }

    static bool isPWMPin(uint8_t pin) {
        return isValidGPIO(pin);
    }
};
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
struct BoardTraits {

    static constexpr int GPIO_COUNT = 48;
    static constexpr int PWM_CHANNEL_COUNT = 8;

    static bool isValidGPIO(uint8_t pin) {
        return pin >= 0 && pin <= 48;
    }

    static bool isPWMPin(uint8_t pin) {
        return pin >= 0 && pin <= 48;
    }
};
#else
    #error "Unsupported ESP32 variant"
#endif

class ResourceManager
{
public:
    static ResourceManager &instance();

    bool isFree(uint8_t pin) const;

    bool reserveGPIO(uint8_t pin) ;

private:
    ResourceManager() = default;
    std::bitset<BoardTraits::GPIO_COUNT> gpioUsed;
    std::bitset<BoardTraits::PWM_CHANNEL_COUNT> pwmUsed;
};