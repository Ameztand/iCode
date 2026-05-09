
#include "gameState.h"
#include "IGameState.h"


// ========== 状态机管理器 ==========
// 压栈
void State::pushSta(Context& ctx, std::unique_ptr<IGameState> newState) {
    if (newState) {
        newState->onEnter(ctx);           // 自动调用具体状态的 onEnter
        stack.push(std::move(newState));
    }
}

//弹出
void State::popSta(Context& ctx) {
    if (!stack.empty()) {
        stack.top()->onExit(ctx);          // 自动调用具体状态的 onExit
        stack.pop();
    }
}

//替换
void State::changeSta(Context& ctx, std::unique_ptr<IGameState> newState) {
    if (!stack.empty()) {
        stack.top()->onExit(ctx);          // 自动调用具体状态的 onExit
        stack.pop();
    }
    if (newState) {
        newState->onEnter(ctx);           // 自动调用具体状态的 onEnter
        stack.push(std::move(newState));
    }
}

//获取栈顶对象
IGameState* State::getState() {
    return stack.empty() ? nullptr : stack.top().get();
}

//是否为空
bool State::empty() {
    return stack.empty();
}

// 获取栈深度
int State::getStaDepth() const {
    return (int)stack.size();
}

//获取栈顶id
int State::getStaID()const {
    return (stack.empty()) ? 0 : stack.top()->getID();
}





// =========== 时间管理器 ===========
void Time::start() {
    start_ = clock();
}

void Time::updata() {
    curr_ = clock();
    sum_ = curr_ - start_;
}

void Time::pause() {
    curr_ = clock();
    sum_ = curr_ - start_;
    item_ += sum_;
    start_ = 0;
}

void Time::resume() {
    start_ = clock();
}

long long Time::getSum() const {
    return sum_ + item_;
}

void Time::clear() {
    sum_ = 0;
    item_ = 0;
    start_ = 0;
}

bool Time::empty() const {
    return start_ == 0;
}







// ========== 监听层 ===========
void Msg::GetMsg() {
    while (peekmessage(&msg_, EX_MOUSE | EX_KEY)) {
        if (msg_.message == WM_KEYDOWN) {
            if (msg_.vkcode >= 0 && msg_.vkcode < 256) {
                auto time = getPressTime(msg_.vkcode);
                if (time == 0) {
                    keys[msg_.vkcode] = { std::chrono::steady_clock::now(), true, true, false };//键盘按下
                }
                else if (time >= LOW_DOWN_TIME) {
                    keys.find(msg_.vkcode)->second.pressSta = false; // 长按状态
                }
            }
        }
        else if (msg_.message == WM_KEYUP) {
            if (msg_.vkcode >= 0 && msg_.vkcode < 256) {
                auto time = getPressTime(msg_.vkcode);
                if (time != 0) {
                    if (time < LOW_DOWN_TIME) {
                        keys.find(msg_.vkcode)->second.pressSta = true; // 短按状态
                    }
                    keys.find(msg_.vkcode)->second.keyUp = true; // 松开
                }
            }
        }
        else if (msg_.message == WM_LBUTTONDOWN) {
            leftButtonDown_ = true;
            mousePos_ = { msg_.x, msg_.y };
        }
        else if (msg_.message == WM_LBUTTONUP) {
            leftButtonDown_ = false;
            mousePos_ = { msg_.x, msg_.y };
        }
    }

}
//获取按下时间
long long Msg::getPressTime(int key) {
    auto it = keys.find(key);
    if (it != keys.end()) {
        auto now = std::chrono::steady_clock::now();//当下的时间戳
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second.pressTime).count();//计算按下时长
        return duration;
    }
    else {
        return 0;
    }
}

//消费短按脉冲
bool Msg::pressPlsS(int key) {
    if (key > 0 && key <= 256) {
        auto it = keys.find(key);
        if (it != keys.end() && it->second.pressPls) {//能找到并且存在脉冲
            it->second.pressPls = false;
            return true;
        }
    }
    return false;
}

//键盘按下情况相关//0为没按 1为短按 2为长按
int Msg::getKeySta(int keyCode) const {
    if (keyCode > 0 && keyCode <= 256) {
        auto it = keys.find(keyCode);
        if (it != keys.end()) {//能找到
            return (it->second.pressSta ? 1 : 2);
        }
    }
    return 0;
}

void Msg::clearKey() {
    keys.clear();
}

void Msg::updataKey() {//每帧更新
    for (auto it = keys.begin(); it != keys.end();) {
        if (it->second.keyUp) {
            it = keys.erase(it);   // 删除后迭代器自动指向下一个
        }
        else {
            ++it;
        }
    }
}

//鼠标按下情况相关
const POINT& Msg::getMousePos()const {
    return mousePos_;
}

bool Msg::getMouse()const {
    return leftButtonDown_;
}

void Msg::clearMouse() {
    leftButtonDown_ = false;
}