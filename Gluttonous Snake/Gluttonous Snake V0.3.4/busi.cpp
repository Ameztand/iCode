


#include "busi.h"

//====== 辅助函数 =====
void ESCpop(Context& ctx) {
    if (ctx.msg.pressPlsS(VK_ESCAPE)) {
        ctx.state.popSta(ctx);
    }
}

void returnToLobby(Context& ctx) {
    while (ctx.state.getStaDepth() > 1) {
        ctx.state.popSta(ctx);
    }
}

void clearPastData(Context& ctx) {
    ctx.gameTime.clear();
    ctx.logic.initDotData();
    ctx.logic.clearMapData();
    ctx.logic.clearPlayerList();
    ctx.logic.initMirror();
}

void MoveMsg(Context& ctx, int p1, int p2) {
    //p1
    if (ctx.msg.getKeySta('W') != 0) ctx.logic.getPlayer(p1).up();
    if (ctx.msg.getKeySta('D') != 0) ctx.logic.getPlayer(p1).right();
    if (ctx.msg.getKeySta('S') != 0) ctx.logic.getPlayer(p1).down();
    if (ctx.msg.getKeySta('A') != 0) ctx.logic.getPlayer(p1).left();

    //p2
    if (ctx.msg.getKeySta(VK_UP) != 0)    ctx.logic.getPlayer(p2).up();
    if (ctx.msg.getKeySta(VK_RIGHT) != 0) ctx.logic.getPlayer(p2).right();
    if (ctx.msg.getKeySta(VK_DOWN) != 0)  ctx.logic.getPlayer(p2).down();
    if (ctx.msg.getKeySta(VK_LEFT) != 0)  ctx.logic.getPlayer(p2).left();
}

void MirrorMoveMsg(Context& ctx, int p1, int p2) {
    if ((ctx.msg.getKeySta('W') != 0) || (ctx.msg.getKeySta(VK_UP) != 0)) { ctx.logic.getPlayer(p1).up();     ctx.logic.getPlayer(p2).down(); }
    if ((ctx.msg.getKeySta('D') != 0) || (ctx.msg.getKeySta(VK_RIGHT) != 0)) { ctx.logic.getPlayer(p1).right();  ctx.logic.getPlayer(p2).left(); }
    if ((ctx.msg.getKeySta('S') != 0) || (ctx.msg.getKeySta(VK_DOWN) != 0)) { ctx.logic.getPlayer(p1).down();   ctx.logic.getPlayer(p2).up(); }
    if ((ctx.msg.getKeySta('A') != 0) || (ctx.msg.getKeySta(VK_LEFT) != 0)) { ctx.logic.getPlayer(p1).left();   ctx.logic.getPlayer(p2).right(); }
}


//暂停
void Pause::onEnter(Context& ctx) {
    //绘画暂停按键
    ctx.render.renderPuase(ctx);
    ctx.gameTime.pause();
}

void Pause::onExit(Context& ctx) {}

void Pause::tick(Context& ctx) {
    //暂停界面
    if (ctx.msg.getMouse()) {
        POINT pt = ctx.msg.getMousePos();
        if (pt.x >= Larea.left && pt.x <= Larea.right && pt.y >= Larea.top && pt.y <= Larea.bottom) {
            //左
            //返回大厅
            ctx.logic.clearPlayerList();
            for (int i = 0; i < 3; i++) {
                ctx.state.popSta(ctx);
            }
            ctx.render.renderLobby(ctx);
            printf("左\n");
        }
        if (pt.x >= Marea.left && pt.x <= Marea.right && pt.y >= Marea.top && pt.y <= Marea.bottom) {
            //中
            //重开（我应该使用StaID？）
            ctx.state.popSta(ctx);
            ctx.logic.clearPlayerList();
            if (auto* cur = ctx.state.getState()) cur->onEnter(ctx);//初始化
            //ctx.stata.changeSta(ctx, std::make_unique<Lobby>());
            printf("中\n");
        }
        if (pt.x >= Rarea.left && pt.x <= Rarea.right && pt.y >= Rarea.top && pt.y <= Rarea.bottom) {
            //右
            //继续游戏
            ctx.state.popSta(ctx);
            ctx.gameTime.resume();
            ctx.render.renderGame(ctx);
            printf("右\n");
        }
    }

    //ESC
    /*
    ESCpop(ctx);
    */
    // 处理 ESC 暂停（通用）
    if (ctx.msg.pressPlsS(VK_ESCAPE)) {
        printf("键盘按下，Sta=%d\n", ctx.state.getStaID());
        ctx.state.popSta(ctx);
        ctx.gameTime.resume();
    }
}

int Pause::getID() const  {
    return ID;
}





//结算
void Over::onEnter(Context& ctx) {
    //绘画暂停按键
    ctx.render.renderOver(ctx);

    ctx.gameTime.pause();
    ctx.msg.clearKey();
}

void Over::onExit(Context& ctx) {}

void Over::tick(Context& ctx) {
    //游戏结束界面
    if (ctx.msg.getMouse()) {
        POINT pt = ctx.msg.getMousePos();
        if (pt.x >= Larea.left && pt.x <= Larea.right && pt.y >= Larea.top && pt.y <= Larea.bottom) {
            //左
            //返回大厅
            ctx.logic.clearPlayerList();
            returnToLobby(ctx);
            ctx.render.renderLobby(ctx);
            printf("左\n");
        }
        else if (pt.x >= Rarea.left && pt.x <= Rarea.right && pt.y >= Rarea.top && pt.y <= Rarea.bottom) {
            //右
            //重开（我应该使用StaID？）
            ctx.state.popSta(ctx);
            ctx.logic.clearPlayerList();
            if (auto* cur = ctx.state.getState()) cur->onEnter(ctx);//初始化
            printf("右\n");
        }
    }
}

int Over::getID() const  {
    return ID;
}









//死亡动画
void Death::onEnter(Context& ctx) {
    bool Death[2] = { false };
    const std::vector<Player>& players = ctx.logic.getPlayerList();
    int playerID = 0;
    for (const Player& player : players) {// 遍历所有玩家进行绘制
        Death[playerID] = !player.getLive();
        playerID++;
    }
    ctx.render.initRenderDeath(Death);
}

void Death::onExit(Context& ctx) {
    /*
    if (ctx.stata.getStaID() == STA_PU_GAME || ctx.stata.getStaID() == STA_EX_GAME) {
        ctx.stata.pushSta(ctx, std::make_unique<Over>());
    }
    else if (ctx.stata.getStaID() == STA_AT_GAME) {
        ctx.stata.pushSta(ctx, std::make_unique<Over>());
    }
    */
}

void Death::tick(Context& ctx) {
    if ((!(ctx.msg.getKeySta(VK_ESCAPE) >= 1)) && ctx.render.getRenderCnt() > 0) {
        ctx.render.renderDeath(ctx);
    }
    else {
        ctx.state.changeSta(ctx, std::make_unique<Over>());
    }
}

int Death::getID() const  {
    return ID;
}






//选择难度
void Select::onEnter(Context& ctx) {
    //绘画难度选择按键
    ctx.render.renderSelect(ctx);
}

void Select::onExit(Context& ctx) {}

void Select::tick(Context& ctx) {
    //大厅检测
    if (ctx.msg.getMouse()) {
        POINT pt = ctx.msg.getMousePos();
        if (pt.x >= Larea.left && pt.x <= Larea.right && pt.y >= Larea.top && pt.y <= Larea.bottom) {
            //左
            //极限模式
            ctx.state.pushSta(ctx, std::make_unique<EXGame>());
            printf("左\n");
        }
        else if (pt.x >= Marea.left && pt.x <= Marea.right && pt.y >= Marea.top && pt.y <= Marea.bottom) {
            //中
            //竞技模式
            ctx.state.pushSta(ctx, std::make_unique<ATGame>());
            printf("中\n");
        }
        else if (pt.x >= Rarea.left && pt.x <= Rarea.right && pt.y >= Rarea.top && pt.y <= Rarea.bottom) {
            //右
            //简单模式
            ctx.state.pushSta(ctx, std::make_unique<PUGame>());
            printf("右\n");
        }
    }

    //ESC
    ESCpop(ctx);
}

int Select::getID() const  {
    return ID;
}





//大厅
void Lobby::onEnter(Context& ctx) {
    //绘画大厅按键
    ctx.render.renderLobby(ctx);
}

void Lobby::onExit(Context& ctx) {}

void Lobby::tick(Context& ctx) {
    //大厅检测
    if (ctx.msg.getMouse()) {
        POINT pt = ctx.msg.getMousePos();
        if (pt.x >= Sarea.left && pt.x <= Sarea.right && pt.y >= Sarea.top && pt.y <= Sarea.bottom) {
            //开始
            ctx.state.pushSta(ctx, std::make_unique<Select>());
            printf("按下开始按钮\n");
        }
    }

    //ESC
    ESCpop(ctx);
}

int Lobby::getID() const  {
    return ID;
}


