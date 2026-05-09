#pragma once

#include "context.h"

//========== ×´Ì¬»ù ==========
class IGameState {
public:
    virtual ~IGameState() = default;
    virtual void onEnter(Context& ctx) = 0;
    virtual void onExit(Context& ctx) = 0;
    virtual void tick(Context& ctx) = 0;
    virtual int getID() const = 0;
};