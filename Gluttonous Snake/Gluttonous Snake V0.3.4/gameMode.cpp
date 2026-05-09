

#include"gameMode.h"


#include "logic.h"
#include "render.h"
#include "gameState.h"
#include "busi.h"


//========== 游戏基 ==========
void BaseGame::onEnter(Context& ctx) {
    clearPastData(ctx);                 // 通用清理
    initPlayers(ctx);                   // 子类提供初始位置
    ctx.logic.initMapData(getGameModeID()); // 初始化地图
    ctx.render.renderGame(ctx);       // 渲染一帧
    ctx.gameTime.start();               // 开始计时
}

void BaseGame::onExit(Context& ctx)   {
    //ctx.gameTime.start();               // 暂停计时？？
}

void BaseGame::tick(Context& ctx)   {
    ctx.gameTime.updata();

    // 处理 ESC 暂停（通用）
    if (ctx.msg.pressPlsS(VK_ESCAPE)) {
        ctx.state.pushSta(ctx, std::make_unique<Pause>());
        return;
    }

    // 调用子类特有的输入处理（如镜像模式）
    ModeInput(ctx);

    // 对所有玩家增加帧计数
    for (int i = 0; i < getPlayerCount(); ++i) {
        ctx.logic.getPlayer(i).FrameAdd();
    }

    // 收集需要移动的玩家
    std::vector<int> toMove;
    for (int i = 0; i < getPlayerCount(); ++i) {
        if (ctx.logic.getPlayer(i).getV() <= ctx.logic.getPlayer(i).getFrame()) {
            toMove.push_back(i);
        }
    }

    // 移动前预测碰撞（judgeMoveRequest）
    for (int i : toMove) {
        ctx.logic.getPlayer(i).setFrame(0);
        ctx.logic.judgeMoveRequest(ctx, ctx.logic.getPlayer(i));
    }

    // 检查是否有玩家死亡
    int loser = ctx.logic.getLoser(ctx);
    if (loser != 0) {
        onGameOver(ctx);   // 子类决定进入 Death 还是直接结算
        return;
    }

    // 真正更新移动数据
    for (int i : toMove) {
        ctx.logic.updataData(ctx, ctx.logic.getPlayer(i));
    }

    // 渲染
    ctx.render.renderGame(ctx);
}

// 基类提供通用的 getID（返回游戏模式ID）
int BaseGame::getID() const {
    return getGameModeID();
}










//PU
int PUGame::getGameModeID() const  {
    return STA_PU_GAME;
}

int PUGame::getPlayerCount() const  {
    return 1;
}

void PUGame::initPlayers(Context& ctx)  {
    ctx.logic.addPlayer(0, 0, 0);//队列初始化
}

void PUGame::ModeInput(Context& ctx)  {
    ctx.logic.getPlayer(0).setV(ctx.msg.getPressTime(VK_SPACE));
    MoveMsg(ctx, 0, 0);
}

void PUGame::onGameOver(Context& ctx)  {
    ctx.state.pushSta(ctx, std::make_unique<Death>());
}






//AT
int ATGame::getGameModeID() const  {
    return STA_AT_GAME;
}

int ATGame::getPlayerCount() const  {
    return 2;
}

void ATGame::initPlayers(Context& ctx)  {
    ctx.logic.addPlayer(-5, 0, 0);//队列初始化
    ctx.logic.addPlayer(5, 0, 0);
}

void ATGame::ModeInput(Context& ctx)  {
    ctx.logic.getPlayer(0).setV(ctx.msg.getPressTime(VK_SPACE));
    ctx.logic.getPlayer(1).setV(ctx.msg.getPressTime(VK_RETURN));
    MoveMsg(ctx, 0, 1);
}

void ATGame::onGameOver(Context& ctx)  {
    ctx.state.pushSta(ctx, std::make_unique<Death>());
}












//EX
int EXGame::getGameModeID() const  {
    return STA_EX_GAME;
}

int EXGame::getPlayerCount() const  {
    return 2;
}

void EXGame::initPlayers(Context& ctx)  {
    ctx.logic.addPlayer(5, -5, 0);//队列初始化
    ctx.logic.addPlayer(-5, 5, 2);
}

void EXGame::ModeInput(Context& ctx)  {
    if (ctx.msg.pressPlsS(VK_SPACE))ctx.logic.Mirror();//空格
    bool mirror = ctx.logic.getMirror(); // 注意：原代码中用 mirror 变量，但 Logic 中 mirror 是私有，需要提供 getMirror() 已存在
    int mainIdx = mirror ? 1 : 0;
    int subIdx = 1 - mainIdx;
    MirrorMoveMsg(ctx, mainIdx, subIdx);
}

void EXGame::onGameOver(Context& ctx)  {
    ctx.state.pushSta(ctx, std::make_unique<Death>());
}