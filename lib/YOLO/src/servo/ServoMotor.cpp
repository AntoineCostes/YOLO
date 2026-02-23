#include "ServoMotor.h"

ServoMotor::ServoMotor(const char* name, 
                       uint8_t pin,
                       float min,
                       float max,
                       bool inverse) : Component(name),
                                        startPosition(-1),
                                        targetPosition(-1),
                                        motionDurationMs(0),
                                        motionStartMs(0)
{
    if (servo.attach(pin))
        initializedParam->set(true);

    // TODO check min < value < max
    positionParam = new FloatParameter("value", ParamAccess::READ_WRITE, (float)(servo.read())/180.0f);
    registerParam(positionParam);
    minParam = new FloatParameter("min", ParamAccess::WRITE_ONLY, min);
    registerParam(minParam);
    maxParam = new FloatParameter("max", ParamAccess::WRITE_ONLY, max);
    registerParam(maxParam);
}

void ServoMotor::refresh()
{
    if (!initializedParam->get())
        return;

    if (motionDurationMs > 0)
    {
        if (millis() > motionStartMs + motionDurationMs)
        {
            goTo(targetPosition); // in case we didn't reach yet
            motionDurationMs = 0; // stop moving
        }
        else if (millis() > lastMoveMs + 10)
        {
            goTo(lerp(startPosition, targetPosition, (float)(millis() - motionStartMs) / (float)motionDurationMs));
            lastMoveMs = millis();
        }
    }
}

float ServoMotor::lerp(float a, float b, float f)
{
    return a * (1.0 - f) + (b * f);
}

void ServoMotor::goTo(float relative, uint32_t durationMs)
{
    if (durationMs < 50)
        goTo(relative);
    else
    {
        startPosition = currentPosition;
        targetPosition = relative;
        motionDurationMs = durationMs;
        motionStartMs = millis();
    }
}

void ServoMotor::goTo(float relative)
{
    if (!initializedParam->get())
        return;

    if (relative < 0.0f || relative > 1.0f)
    {
        err("servo position: %f should be [0:1]", relative);
        return;
    }
    currentPosition = relative;
    lastMoveMs = millis();

    float min = minParam->get();
    float max = maxParam->get();
    float targetPosition = min + relative * (max - min);
    if (inverseParam->get())
        targetPosition = max + relative * (min - max);

    dbg("go to %f (%i us)", targetPosition, PWM_MIN + (PWM_MAX - PWM_MIN) * targetPosition);
    
    servo.writeMicroseconds(DEFAULT_uS_LOW + targetPosition * (DEFAULT_uS_HIGH - DEFAULT_uS_LOW));
}
