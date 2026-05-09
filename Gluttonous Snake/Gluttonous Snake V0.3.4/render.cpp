

#include "render.h"



// ========== 渲染层 ===========
void Renderer::renderDeath(Context& ctx) {
    //printf("Frame_/RenderCnt_: %d / %d\n", Frame_, RenderCnt_);

    BeginBatchDraw();   // 开始批量绘图（双缓冲）

    if (Frame_ >= SNAKE_UPDATE) {
        Frame_ -= SNAKE_UPDATE;

        setfillcolor(WHITE);
        solidrectangle(75, 75, 645, 650);

        // 画棋盘表格底层阴影
        setlinecolor(colors[6]);             // 线条颜色
        setlinestyle(PS_SOLID, 5);     // 线型：实线，宽度3像素
        for (int i = 0; i < 20; i++) {//30,75
            line(78, 78 + 30 * i, 648, 78 + 30 * i);
            line(78 + 30 * i, 78, 78 + 30 * i, 648);
        }

        //打印原点
        Print(0, 0, 8);

        //打印果子
        for (int i = 0; i < 3; i++) {
            const Position& pos = ctx.logic.getDotData(i);
            Print(pos.x, pos.y, 2);
        }

        //打印蛇
        const std::vector<Player>& players = ctx.logic.getPlayerList();
        int playerID = -1;
        for (const Player& player : players) {// 遍历所有玩家进行绘制
            playerID++;

            if (Death_[playerID] == true) {
                const Position& npos = player.getNextPos();
                printX(npos.x, npos.y);
            }

            if (Death_[playerID] == true && RenderCnt_ % 2 == 0) { //闪烁
                printf("跳过P%d ", playerID + 1);;
                continue;
            }

            printf("渲染P%d ", playerID + 1);
            std::queue<Position> temp = player.getQueue();
            int x = 0;
            int y = 0;
            for (int i = temp.size(); i > 0; i--) {
                const Position& pos = temp.front();
                x = pos.x;
                y = pos.y;
                Print(x, y, (i == 1 ? 4 : (playerID == ctx.logic.getMirror() ? 1 : 3)));
                temp.pop();
            }

            if (Death_[playerID] == true) {
                const Position& npos = player.getNextPos();
                printX(npos.x, npos.y);
            }
        }
        printf("\n");

        //蓝框
        setlinecolor(colors[7]);             // 线条颜色
        setlinestyle(PS_SOLID, 2);     // 线型：实线，宽度3像素
        for (int i = 0; i < 20; i++) {
            line(75, 75 + 30 * i, 645, 75 + 30 * i);
            line(75 + 30 * i, 75, 75 + 30 * i, 645);
        }
        setlinecolor(BLUE);             // 线条颜色
        setlinestyle(PS_SOLID, 4);     // 线型：实线，宽度3像素
        line(75, 75, 75, 645);
        line(645, 75, 645, 645);
        line(75, 75, 645, 75);
        line(75, 645, 645, 645);

        //叉叉
        playerID = -1;
        for (const Player& player : players) {// 遍历所有玩家进行绘制
            playerID++;

            if (Death_[playerID] == true) {
                const Position& npos = player.getNextPos();
                printX(npos.x, npos.y);
            }
        }

        RenderCnt_--;
    }
    else {
        Frame_++;
    }

    EndBatchDraw();     // 结束批量绘图，一次性显示所有内容
}

void Renderer::initRenderDeath(const bool(&Death)[2]) {
    Death_[0] = Death[0];
    Death_[1] = Death[1];
    printf("%d / %d \n", Death_[0], Death_[1]);
    Frame_ = SNAKE_UPDATE;
    RenderCnt_ = 9;
}

int Renderer::getRenderCnt() {
    return RenderCnt_;
}

void Renderer::renderGame(Context& ctx) {
    BeginBatchDraw();   // 开始批量绘图（双缓冲）

    cleardevice();

    // 画棋盘表格底层阴影
    setlinecolor(colors[6]);             // 线条颜色
    setlinestyle(PS_SOLID, 5);     // 线型：实线，宽度3像素
    for (int i = 0; i < 20; i++) {//30,75
        line(78, 78 + 30 * i, 648, 78 + 30 * i);
        line(78 + 30 * i, 78, 78 + 30 * i, 648);
    }

    //打印原点
    Print(0, 0, 8);

    //打印果子
    for (int i = 0; i < 3; i++) {
        const Position& pos = ctx.logic.getDotData(i);
        Print(pos.x, pos.y, 2);
    }

    //打印蛇
    const std::vector<Player>& players = ctx.logic.getPlayerList();
    int playerID = 0;
    for (const Player& player : players) {// 遍历所有玩家进行绘制
        std::queue<Position> temp = player.getQueue();
        int x = 0;
        int y = 0;
        for (int i = temp.size(); i > 0; i--) {
            const Position& pos = temp.front();
            x = pos.x;
            y = pos.y;
            Print(x, y, (i == 1 ? 4 : (playerID == ctx.logic.getMirror() ? 1 : 3)));//第563行
            temp.pop();
        }
        playerID++;
    }

    //蓝框
    setlinecolor(colors[7]);             // 线条颜色
    setlinestyle(PS_SOLID, 2);     // 线型：实线，宽度3像素
    for (int i = 0; i < 20; i++) {
        line(75, 75 + 30 * i, 645, 75 + 30 * i);
        line(75 + 30 * i, 75, 75 + 30 * i, 645);
    }
    setlinecolor(BLUE);             // 线条颜色
    setlinestyle(PS_SOLID, 4);     // 线型：实线，宽度3像素
    line(75, 75, 75, 645);
    line(645, 75, 645, 645);
    line(75, 75, 645, 75);
    line(75, 645, 645, 645);

    //显示title标语
    settextcolor(BLACK);
    outtextxy(75, 55, L"Hello, HungerSnake!");

    //打印游玩时长
    wchar_t buffer[256];
    swprintf(buffer, 100, L"游戏时间：%llds               ", ctx.gameTime.getSum() / 1000);
    outtextxy(800, 360, buffer);

    //打印速度
    if (ctx.state.getStaID() == STA_PU_GAME) {
        swprintf(buffer, 256, L"当前速度：%d / %lld                                                                      ",
            1000 / ctx.logic.getPlayer(0).getV(), ctx.msg.getPressTime(VK_SPACE));
        outtextxy(800, 340, buffer);
    }
    else if (ctx.state.getStaID() == STA_AT_GAME) {
        swprintf(buffer, 256, L"当前速度：%d / %d / %lld / %lld                                                                      ",
            1000 / ctx.logic.getPlayer(0).getV(), 1000 / ctx.logic.getPlayer(1).getV(), ctx.msg.getPressTime(VK_SPACE), ctx.msg.getPressTime(VK_RETURN));
        outtextxy(800, 340, buffer);
    }
    else if (ctx.state.getStaID() == STA_EX_GAME) {
        swprintf(buffer, 256, L"当前速度：%d / %d / %d / %d                                                                      ",
            1000 / ctx.logic.getPlayer(0).getV(), 1000 / ctx.logic.getPlayer(1).getV(), ctx.logic.getPlayer(0).getE(), ctx.logic.getPlayer(1).getE());
        outtextxy(800, 340, buffer);
    }


    //打印长度提示
    if (ctx.state.getStaID() == STA_PU_GAME) {
        swprintf(buffer, 256, L"当前长度：%d                                          ", (int)ctx.logic.getPlayer(0).sizeQ());
        outtextxy(800, 380, buffer);
    }
    else if (ctx.state.getStaID() == STA_AT_GAME || ctx.state.getStaID() == STA_EX_GAME) {
        swprintf(buffer, 256, L"当前长度：%d / %d                                          ", (int)ctx.logic.getPlayer(0).sizeQ(), (int)ctx.logic.getPlayer(1).sizeQ());
        outtextxy(800, 380, buffer);
    }

    setbkmode(TRANSPARENT);

    // 定义颜色常量
    COLORREF normal = colors[6];              // 灰（未按下）
    COLORREF pressedGreen = colors[1];        // 亮绿（按下）
    COLORREF pressedBlue = colors[3];         // 亮蓝（按下）

    // ----- 玩家1（WASD + 空格） -----
    // 1. W 键（上）
    if (ctx.msg.getKeySta('W'))
        setfillcolor(pressedGreen);
    else
        setfillcolor(normal);
    solidrectangle(800, 575, 825, 600);   // 上

    // 2. A 键（左）
    if (ctx.msg.getKeySta('A'))
        setfillcolor(pressedGreen);
    else
        setfillcolor(normal);
    solidrectangle(765, 610, 790, 635);   // 左

    // 3. S 键（下）
    if (ctx.msg.getKeySta('S'))
        setfillcolor(pressedGreen);
    else
        setfillcolor(normal);
    solidrectangle(800, 610, 825, 635);   // 下

    // 4. D 键（右）
    if (ctx.msg.getKeySta('D'))
        setfillcolor(pressedGreen);
    else
        setfillcolor(normal);
    solidrectangle(835, 610, 860, 635);   // 右

    // 5. 空格键（中心方块）
    if (ctx.msg.getKeySta(VK_SPACE))
        setfillcolor(pressedGreen);
    else
        setfillcolor(normal);
    solidrectangle(775, 640, 850, 657);   // 中心

    // ----- 玩家2（方向键 + 回车） -----
    // 6. 上方向键
    if (ctx.msg.getKeySta(VK_UP))
        setfillcolor(pressedBlue);
    else
        setfillcolor(normal);
    solidrectangle(950, 575, 975, 600);   // 上

    // 7. 左方向键
    if (ctx.msg.getKeySta(VK_LEFT))
        setfillcolor(pressedBlue);
    else
        setfillcolor(normal);
    solidrectangle(915, 610, 940, 635);   // 左

    // 8. 下方向键
    if (ctx.msg.getKeySta(VK_DOWN))
        setfillcolor(pressedBlue);
    else
        setfillcolor(normal);
    solidrectangle(950, 610, 975, 635);   // 下

    // 9. 右方向键
    if (ctx.msg.getKeySta(VK_RIGHT))
        setfillcolor(pressedBlue);
    else
        setfillcolor(normal);
    solidrectangle(985, 610, 1010, 635);  // 右

    // 10. 回车键（中心方块）
    if (ctx.msg.getKeySta(VK_RETURN))
        setfillcolor(pressedBlue);
    else
        setfillcolor(normal);
    solidrectangle(925, 640, 1000, 657);  // 中心

    setbkmode(OPAQUE);//TRANSPARENT

    EndBatchDraw();     // 结束批量绘图，一次性显示所有内容
}

void Renderer::renderLobby(Context& ctx) {
    BeginBatchDraw();   // 开始批量绘图（双缓冲）

    cleardevice();
    //画中按钮
    setlinecolor(RGB(0, 0, 200));
    setlinestyle(PS_SOLID, 3);
    line(490, 500, 590, 500);
    line(490, 550, 590, 550);
    line(490, 500, 490, 550);
    line(590, 500, 590, 550);
    setfillcolor(RGB(255, 220, 75));//0Xc0c15e//250, 252, 156
    solidrectangle(492, 502, 588, 548);
    //文字
    setbkmode(TRANSPARENT);
    settextcolor(BLACK);
    outtextxy(510, 518, L"开始游戏");
    setbkmode(OPAQUE);//TRANSPARENT

    EndBatchDraw();     // 结束批量绘图，一次性显示所有内容
}

void Renderer::renderSelect(Context& ctx) {
    BeginBatchDraw();   // 开始批量绘图（双缓冲）

    cleardevice();
    // 画后表格
    setlinecolor(RGB(0, 0, 200));
    setlinestyle(PS_SOLID, 3);
    line(310, 240, 770, 240);//上
    line(310, 480, 770, 480);//下
    line(310, 240, 310, 480);//左
    line(770, 240, 770, 480);//右
    setfillcolor(RGB(250, 250, 156));//0Xc0c15e//250, 252, 156
    solidrectangle(312, 242, 768, 478);
    //画左按钮
    setlinecolor(RGB(0, 0, 200));
    setlinestyle(PS_SOLID, 3);
    line(360, 400, 460, 400);
    line(360, 450, 460, 450);
    line(360, 400, 360, 450);
    line(460, 400, 460, 450);
    setfillcolor(RGB(255, 39, 0));//0Xc0c15e//250, 252, 156//50, 200, 200
    solidrectangle(362, 402, 458, 448);
    //画中按钮
    setlinecolor(RGB(0, 0, 200));
    setlinestyle(PS_SOLID, 3);
    line(490, 400, 590, 400);
    line(490, 450, 590, 450);
    line(490, 400, 490, 450);
    line(590, 400, 590, 450);
    setfillcolor(RGB(255, 220, 75));//0Xc0c15e//250, 252, 156
    solidrectangle(492, 402, 588, 448);
    //画右按钮
    setlinecolor(RGB(0, 0, 200));
    setlinestyle(PS_SOLID, 3);
    line(620, 400, 720, 400);
    line(620, 450, 720, 450);
    line(620, 400, 620, 450);
    line(720, 400, 720, 450);
    setfillcolor(RGB(124, 254, 86));//0Xc0c15e//250, 252, 156
    solidrectangle(622, 402, 718, 448);
    //文字
    setbkmode(TRANSPARENT);
    settextcolor(BLACK);
    outtextxy(395, 418, L"极限");
    outtextxy(525, 418, L"竞速");
    outtextxy(655, 418, L"普通");
    outtextxy(510, 310, L"选择模式");
    setbkmode(OPAQUE);//TRANSPARENT

    EndBatchDraw();     // 结束批量绘图，一次性显示所有内容
}

void Renderer::renderPuase(Context& ctx) {
    BeginBatchDraw();   // 开始批量绘图（双缓冲）

    //cleardevice();
    // 画后表格
    setlinecolor(RGB(0, 0, 200));
    setlinestyle(PS_SOLID, 3);
    line(310, 240, 770, 240);//上
    line(310, 480, 770, 480);//下
    line(310, 240, 310, 480);//左
    line(770, 240, 770, 480);//右
    setfillcolor(RGB(250, 250, 156));//0Xc0c15e//250, 252, 156
    solidrectangle(312, 242, 768, 478);
    //画左按钮
    setlinecolor(RGB(0, 0, 200));
    setlinestyle(PS_SOLID, 3);
    line(360, 400, 460, 400);
    line(360, 450, 460, 450);
    line(360, 400, 360, 450);
    line(460, 400, 460, 450);
    setfillcolor(RGB(50, 200, 200));//0Xc0c15e//250, 252, 156//50, 200, 200
    solidrectangle(362, 402, 458, 448);
    //画中按钮
    setlinecolor(RGB(0, 0, 200));
    setlinestyle(PS_SOLID, 3);
    line(490, 400, 590, 400);
    line(490, 450, 590, 450);
    line(490, 400, 490, 450);
    line(590, 400, 590, 450);
    setfillcolor(RGB(255, 220, 75));//0Xc0c15e//250, 252, 156
    solidrectangle(492, 402, 588, 448);
    //画右按钮
    setlinecolor(RGB(0, 0, 200));
    setlinestyle(PS_SOLID, 3);
    line(620, 400, 720, 400);
    line(620, 450, 720, 450);
    line(620, 400, 620, 450);
    line(720, 400, 720, 450);
    setfillcolor(RGB(124, 254, 86));//0Xc0c15e//250, 252, 156
    solidrectangle(622, 402, 718, 448);
    //文字
    setbkmode(TRANSPARENT);
    settextcolor(BLACK);
    outtextxy(380, 418, L"返回开始");
    outtextxy(510, 418, L"重新开始");
    outtextxy(640, 418, L"继续游戏");
    outtextxy(527, 310, L"暂停");
    setbkmode(OPAQUE);//TRANSPARENT

    EndBatchDraw();     // 结束批量绘图，一次性显示所有内容
}

void Renderer::renderOver(Context& ctx) {
    BeginBatchDraw();   // 开始批量绘图（双缓冲）

    //cleardevice();
    // 画后表格
    setlinecolor(RGB(0, 0, 200));
    setlinestyle(PS_SOLID, 3);
    line(310, 240, 770, 240);//上
    line(310, 480, 770, 480);//下
    line(310, 240, 310, 480);//左
    line(770, 240, 770, 480);//右
    setfillcolor(RGB(250, 250, 156));//0Xc0c15e//250, 252, 156
    solidrectangle(312, 242, 768, 478);
    //画左按钮
    setlinecolor(RGB(0, 0, 200));
    setlinestyle(PS_SOLID, 3);
    line(360, 400, 460, 400);
    line(360, 450, 460, 450);
    line(360, 400, 360, 450);
    line(460, 400, 460, 450);
    setfillcolor(RGB(50, 200, 200));//0Xc0c15e//250, 252, 156//50, 200, 200
    solidrectangle(362, 402, 458, 448);
    //画右按钮
    setlinecolor(RGB(0, 0, 200));
    setlinestyle(PS_SOLID, 3);
    line(620, 400, 720, 400);
    line(620, 450, 720, 450);
    line(620, 400, 620, 450);
    line(720, 400, 720, 450);
    setfillcolor(RGB(255, 220, 75));//0Xc0c15e//250, 252, 156
    solidrectangle(622, 402, 718, 448);
    //文字
    setbkmode(TRANSPARENT);
    settextcolor(BLACK);
    outtextxy(380, 418, L"返回开始");
    outtextxy(640, 418, L"重新开始");
    outtextxy(500, 310, L"GAME OVER !!");
    setbkmode(OPAQUE);//TRANSPARENT

    EndBatchDraw();     // 结束批量绘图，一次性显示所有内容
}

/*
    else if (ctx.stata.getStaID() == STA_ACCOUNT) {
        settextcolor(BLACK);
        outtextxy(380, 418, L"返回开始");
        outtextxy(640, 418, L"重新开始");

        wchar_t buffer[100];
        int item = ctx.logic.getLoser(ctx);
        if (item == 3) {
            swprintf(buffer, 100, L"真是一对苦命鸳鸯");
            outtextxy(480, 310, buffer);//??
        }
        else {
            swprintf(buffer, 100, L"Player%d VICTORY !!!", (item == 1 ? 2 : 1));
            outtextxy(490, 310, buffer);//??
        }

    }
    */

void Renderer::Print(int x, int y, int color) {
    setfillcolor(colors[color]);
    solidrectangle(X_CODE_PIONE + x * 30, X_CODE_PIONE - y * 30, Y_CODE_PIONE + x * 30, Y_CODE_PIONE - y * 30);
}

void Renderer::printX(int x, int y) {
    setlinecolor(RED);
    line(X_CODE_PIONE + x * 30, X_CODE_PIONE - y * 30, Y_CODE_PIONE + x * 30, Y_CODE_PIONE - y * 30);
    line(Y_CODE_PIONE + x * 30, X_CODE_PIONE - y * 30, X_CODE_PIONE + x * 30, Y_CODE_PIONE - y * 30);
}