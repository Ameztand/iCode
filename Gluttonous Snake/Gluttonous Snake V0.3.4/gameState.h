#pragma once

#include <easyx.h>
#include<stack>
#include <memory> 
#include <chrono>
#include <unordered_map>

#include "common.h"
#include "context.h"



// ========== 状态机管理器 ==========
class State {
private:
    std::stack<std::unique_ptr<IGameState>> stack;

public:
    // 压栈
    void pushSta(Context& ctx, std::unique_ptr<IGameState> newState);

    //弹出
    void popSta(Context& ctx);

    //替换
    void changeSta(Context& ctx, std::unique_ptr<IGameState> newState);

    //获取栈顶对象
    IGameState* getState();

    //是否为空
    bool empty();

    // 获取栈深度
    int getStaDepth() const;

    //获取栈顶id
    int getStaID()const;
};

// =========== 时间管理器 ===========
class Time {
private:
    long long start_ = 0;
    long long curr_ = 0;
    long long sum_ = 0;
    long long item_ = 0;

public:
    //Time() : start(0), curr(0), sum(0), item(0) {}

    void start();

    void updata();

    void pause();

    void resume();

    long long getSum() const;

    void clear();

    bool empty() const;
};

// ========== 监听层 ===========
class Msg {
private:
    struct keySta {
        std::chrono::steady_clock::time_point pressTime;//按下时间ms
        bool pressSta = { true };//T短按状态,F长按状态
        bool pressPls = { true };//T短按脉冲
        bool keyUp = { false };//松开标识（清理标识）
    };

    std::unordered_map<int, keySta> keys;//键盘按下情况表
    bool leftButtonDown_ = false;//鼠标按下情况

    ExMessage msg_;
    POINT mousePos_;

public:
    void GetMsg();

    //获取按下时间
    long long getPressTime(int key);

    //消费短按脉冲
    bool pressPlsS(int key);

    //键盘按下情况相关//0为没按 1为短按 2为长按
    int getKeySta(int keyCode) const;

    void clearKey();

    void updataKey();

    //鼠标按下情况相关
    const POINT& getMousePos()const;

    bool getMouse()const;

    void clearMouse();
};




