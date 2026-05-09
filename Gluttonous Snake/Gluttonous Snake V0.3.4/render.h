#pragma once

#define X_CODE_PIONE  346   //(0,0)左上坐标
#define Y_CODE_PIONE  373   //(0,0)右下坐标

#include <easyx.h>
#include <vector>
#include <queue>

#include "context.h"
#include "common.h"
#include "logic.h"
#include "gameState.h"


// ========== 渲染层 ===========
class Renderer {
private:
    bool Death_[2] = { false };//用于结算模式计算哪条蛇闪烁  
    int Frame_ = 0;//计算频闪帧率
    int RenderCnt_ = 9;//计算频闪次数

    COLORREF colors[9] = {//124, 229, 255 淡蓝蓝
        //0路 1蛇 2果子 4蛇头 5原点
        RGB(255,255,255),  // 白0
        RGB(0, 255, 0),    // 绿1
        RGB(255, 255, 0),  // 黄2
        RGB(0, 243, 255),  // 蓝3//0, 143, 255//100, 100, 240
        RGB(255, 0, 0),    // 红4
        RGB(128, 0, 255), // 紫5
        RGB(240, 240, 240),//灰6
        RGB(80, 80 ,230), //深灰7
        RGB(255, 128, 0)  // 橙8
    };

public:
    void renderDeath(Context& ctx);

    void initRenderDeath(const bool(&Death)[2]);

    int getRenderCnt();

    void renderGame(Context& ctx);

    void renderLobby(Context& ctx);

    void renderSelect(Context& ctx);

    void renderPuase(Context& ctx);

    void renderOver(Context& ctx);

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

    void Print(int x, int y, int color);

    void printX(int x, int y);
};