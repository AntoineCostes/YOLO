#pragma once
#include "common/Dependencies.h"

class Timer
{
public: 
    typedef std::function<void(void)> TimerCallback;
    TimerCallback cb;
    Timer(long durationMs, bool loop = false) : isRunning(false), timeAtStart(0), intervalMs(durationMs), doLoop(loop)
    {}

    ~Timer() {}

    void setCallback(TimerCallback callback)
    {
        cb = callback;
    }

    bool isRunning;
    long timeAtStart;
    long intervalMs;
    bool doLoop;

    void set(long durationMs, bool loop = false)
    {
        intervalMs = durationMs;
        doLoop = loop;
    }

    void start()
    {
        timeAtStart = millis();
        isRunning = true;
    }

    void stop()
    {
        isRunning = false;
    }

    void update()
    {
        if(!isRunning) return;
        if(millis() - timeAtStart >= intervalMs)
        {
            isRunning = false;
            if (cb) cb();
            if (doLoop) start();
        }
    }
};