#pragma once

#include <easyx.h>
#include <memory>     // std::unique_ptr 智能指针

#include "context.h"
#include "common.h"
#include "IGameState.h"
#include "gameMode.h"

#include "logic.h"
#include "render.h"
#include "gameState.h"



void ESCpop(Context& ctx);
void returnToLobby(Context& ctx);
void clearPastData(Context& ctx);
void MoveMsg(Context& ctx, int p1, int p2);
void MirrorMoveMsg(Context& ctx, int p1, int p2);

//class Lobby;
//class Select;
//class Pause;
//class Over;
//class Death;


//暂停
class Pause :public IGameState {
private:
    int ID = STA_PAUSE;

public:
    void onEnter(Context& ctx)override;

    void onExit(Context& ctx)override;

    void tick(Context& ctx)override;

    int getID() const override;
};

//结算
class Over :public IGameState {
private:
    int ID = STA_OVER;

public:
    void onEnter(Context& ctx)override;

    void onExit(Context& ctx)override;

    void tick(Context& ctx)override;

    int getID() const override;

};

//死亡动画
class Death :public IGameState {
private:
    int ID = STA_DEATH;

public:
    void onEnter(Context& ctx)override;

    void onExit(Context& ctx)override;

    void tick(Context& ctx)override;

    int getID() const override;
};

//选择难度
class Select :public IGameState {
private:
    int ID = STA_SELECT;

public:
    void onEnter(Context& ctx)override;

    void onExit(Context& ctx)override;

    void tick(Context& ctx)override;

    int getID() const override;
};

//大厅
class Lobby :public IGameState {
private:
    int ID = STA_LOBBY;

public:
    void onEnter(Context& ctx)override;

    void onExit(Context& ctx)override;

    void tick(Context& ctx)override;

    int getID() const override;
};