#pragma once

#include <memory> 
#include <windows.h>

#include "context.h"
#include "IGameState.h"



//========== 游戏基 ==========
class BaseGame :public IGameState {
public:
    // 每个子类必须返回自己的模式ID（用于渲染、速度计算等）
    virtual int getGameModeID() const = 0;

    // 返回该模式的玩家数量（1 或 2）
    virtual int getPlayerCount() const = 0;

    // 初始化玩家出生点（在 onEnter 中调用）
    virtual void initPlayers(Context& ctx) = 0;

    // 处理本模式特有的输入（如极限模式的镜像切换）
    virtual void ModeInput(Context& ctx) = 0;

    // 当玩家死亡时，决定切换到哪个结算状态（Over 或 Account）
    virtual void onGameOver(Context& ctx) = 0;

    void onEnter(Context& ctx) override final;

    void onExit(Context& ctx) override final;

    void tick(Context& ctx) override final;

    // 基类提供通用的 getID（返回游戏模式ID）
    int getID() const override final;
};

//PU
class PUGame :public BaseGame {
public:
    int getGameModeID() const override;

    int getPlayerCount() const override;

    void initPlayers(Context& ctx) override;

    void ModeInput(Context& ctx) override;

    void onGameOver(Context& ctx) override;
};

//AT
class ATGame :public BaseGame {
public:
    int getGameModeID() const override;

    int getPlayerCount() const override;

    void initPlayers(Context& ctx) override;

    void ModeInput(Context& ctx) override;

    void onGameOver(Context& ctx) override;
};

//EX
class EXGame :public BaseGame {
public:
    int getGameModeID() const override;

    int getPlayerCount() const override;

    void initPlayers(Context& ctx) override;

    void ModeInput(Context& ctx) override;

    void onGameOver(Context& ctx) override;
};