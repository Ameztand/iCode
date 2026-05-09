/*
26.4.29V3.4版贪吃蛇

简单实现分文件，未修改任何bug
*/



//#include <queue>//为了队列
//#include <stack>//为了栈
#include <iostream>//数据流
//#include <vector>//动态数组
//#include <algorithm>//快速清空数组
//#include <memory>     // std::unique_ptr 智能指针
//#include <typeindex>  // std::type_index 用于运行时类型信息
//#include <functional>
#include <unordered_map>
#include <easyx.h>
#include <windows.h>  // 为了 Sleep
#include <chrono>
#include <thread>
#include <cstdlib>  // rand(), srand()

#pragma comment(lib, "winmm.lib")   // 添加这一行


#include "common.h"
#include "context.h"
#include "gamemode.h"
#include "gameState.h"
#include "logic.h"
#include "render.h"
#include "busi.h"


// ================== 主函数 =====================
int main()
{
    timeBeginPeriod(1);  // // 提高定时器精度,全局设置，程序结束时记得 timeEndPeriod(1)

    Time gameTime;
    State state;
    Logic logic;
    Msg msg;
    Renderer render;

    Context ctx{ state, logic, render, msg, gameTime };


    //生成画布
    initgraph(1080, 720);
    setbkcolor(WHITE);             //设置背景（白）
    cleardevice();
    //初始化必备数据
    setbkmode(OPAQUE);//TRANSPARENT
    srand((unsigned int)time(NULL));//加载随机数

    ctx.state.pushSta(ctx, std::make_unique<Lobby>());//初始化栈


    //加载帧率计数器
    const double FRAME_TIME_MS = 1000.0 / FPS_LOGIC;  // 16.666... ms
    using namespace std::chrono;
    auto nextFrameTime = steady_clock::now();//第0帧时间节点


    //程序主循环
    while (TRUE) {

        if (ctx.state.empty())break;//退出逻辑

        ctx.msg.updataKey();//清空松开的键盘
        ctx.msg.GetMsg();//监听键盘

        if (auto* cur = ctx.state.getState()) cur->tick(ctx);//运行栈顶


        //等待
        nextFrameTime += milliseconds((long long)FRAME_TIME_MS);
        std::this_thread::sleep_until(nextFrameTime);

        // 防追帧
        if (steady_clock::now() > nextFrameTime) {
            // 重置时间基准，防止疯狂追帧
            nextFrameTime = steady_clock::now();
        }
    }

    timeEndPeriod(1);
    closegraph();
    return 0;
}





