#pragma once
#include "common/Component.h"

#define PWM_MIN 163
#define PWM_MAX 530

class ServoMotor : public Component
{
public:
    ServoMotor(const char* name, uint8_t pin, float min, float max, bool inverse);//, Adafruit_MS_PWMServoDriver *pwm);
    void refresh() override;

    void goTo(float relative);
    void goTo(float relative, uint32_t durationMs);

protected:
    FloatParameter* positionParam;
    FloatParameter* minParam;
    FloatParameter* maxParam;
    BoolParameter* inverseParam;

    Servo servo;

    float currentPosition;
    float startPosition;
    float targetPosition;
    uint32_t motionDurationMs;
    uint32_t motionStartMs;
    uint32_t lastMoveMs;

    float lerp(float a, float b, float f);
};
