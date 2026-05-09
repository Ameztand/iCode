//demo 第一版
#define SNAKE_UPDATE 20 //初始蛇蛇刷新
#define FRAME_DALAY 10 //刷新率延迟
#define LOW_DOWN_TIME 200 //最短按下时间
#define X_CODE_PIONE  347   //(0,0)左上坐标
#define Y_CODE_PIONE  373   //(0,0)右下坐标


#include <queue>//为了队列
#include <iostream>//数据流
#include <easyx.h>
#include <windows.h>  // 为了 Sleep
#include <cstdlib>  // rand(), srand()
#include <ctime>    // time()

void InitData();

void GenGameMode();

void BackDownTime(int mode);
void UpdateSnake(int nx, int ny);
void UpdateMap();//更新网格棋盘
void Update();

void UpdateMenu(int mode);
void Setting(int mode);
void GetMsg();
void GameTimeSetting(int mode);


void WheelQueue(int x, int y);
void Print(int x, int y, int color);
void CreatDot();
int Random();


typedef struct {
    long long start;
    long long curr;
    long long sum;
    long long item;
} Time;

Time GameT = { 0 };//游戏时间
Time LimFPS = { 0 };//限制帧率
Time BackT = { 0 };//空格按下时间

COLORREF colors[] = {
    RGB(255,255,255),  // 白0
    RGB(255, 0, 0),    // 红1
    RGB(255, 128, 0),  // 橙2
    RGB(255, 255, 0),  // 黄3
    RGB(0, 255, 0),    // 绿4
    RGB(0, 0, 255),    // 蓝5
    RGB(128, 0, 255),  // 紫6
};

ExMessage msg;
RECT Larea = { 362, 402, 458, 448 };//左按钮
RECT Marea = { 492, 402, 588, 448 };//中按钮
RECT Rarea = { 622, 402, 718, 448 };//右按钮
RECT Sarea = { 492, 502, 588, 548 };//开始按钮

// 创建队列
std::queue<int> XQueue;
std::queue<int> YQueue;
//果子坐标
int EDot[3][2];

int Dir = 0;//方向参数
int Sta = 0; //状态参数( 0开始菜单/ 1选择菜单/ 2游戏暂停 /3游戏中)

int UdCnt = 0;//刷新次数
int DMode = 4;//按下按键代码

int SnakeUpdateV = SNAKE_UPDATE;//渲染速度

int dx[4] = { 0,1,0,-1 };//上右下左
int dy[4] = { 1,0,-1,0 };

int MapData[19][19] = { 0 };

int main()
{
    //加载随机数
    srand((unsigned int)time(NULL));

    //生成画布
    initgraph(1080, 720);
    setbkcolor(WHITE);             //设置背景（白）
    setbkmode(OPAQUE);//TRANSPARENT

    //绘画基本按键
    Setting(0);

    //程序主循环
    while (TRUE) {
        GetMsg();//检测输入
        if (Sta == -1)break;//退出逻辑

        //游戏主循环
        if (Sta == 3) {
            GenGameMode();
        }
    }

    closegraph();
    return 0;
}

void InitData() {
    //队列初始化
    XQueue.push(0);
    YQueue.push(-2);
    XQueue.push(0);
    YQueue.push(-1);
    XQueue.push(0);
    YQueue.push(0);

    //地图初始化
    MapData[9][9] = 1; //（0，0）
    MapData[9][10] = 1;//（0，-1）
    MapData[9][11] = 1;//（0，-2）

    //果子坐标初始化
    for (int i = 0; i < 3; i++) {
        EDot[i][0] = 10;
        EDot[i][1] = 10;
    }

    //画表格
    UpdateMap();

    //更新
    Update();
}

void GenGameMode() {
    //游戏地图初始化
    InitData();
    int WhichFrame = 0;
    for (int i = 0; i < 3; i++) {//添加3个果子
        CreatDot();
    }

    Sleep(3000);//开局3秒准备

    GameTimeSetting(0);// 开始记录游戏时间
    //游戏主循环
    while (TRUE) {
        LimFPS.start = clock();//帧率采集
        if (Sta <= 0) break;//结束进程

        GetMsg();//检测输入
        GameTimeSetting(1);//时间更新
        BackDownTime(2); //加速检测

        //蛇吃果子关键帧判定
        if (Sta == 3 && SnakeUpdateV <= WhichFrame) {
            //清空帧数计数
            WhichFrame = 0;

            //蛇头检测
            int nx = XQueue.back() + dx[Dir];
            int ny = YQueue.back() + dy[Dir];
            if (nx < -9 || ny < -9 || nx>9 || ny>9 || MapData[nx + 9][-ny + 9] == 1) {
                //gameover
                outtextxy(800, 300, L"GameOver!");
            }
            else {
                UpdateSnake(nx, ny);//刷新蛇
            }

            /*
            //命令行地图（建议修改FPSd==100再使用）
            printf("====================\n ");
            for (int i = 0; i < 19; i++) {
                for (int j = 0; j < 19; j++) {
                    printf("%d ", MapData[j][i]);
                }
                printf("\n ");
            }
            printf("====================\n ");
            */
        }

        //刷新画面
        if (Sta == 3) {
            Update();
        }


        //限制帧率
        WhichFrame++;
        LimFPS.curr = clock();
        int item = FRAME_DALAY + LimFPS.start - LimFPS.curr;
        if (item > 0)Sleep(item);
    }
}

void BackDownTime(int mode) {
    //空格加速（0清除计时 1开始计时 2检测计时长短并加速）
    if (mode == 0) {
        BackT.start = 0;
        BackT.sum = 0;
        SnakeUpdateV = SNAKE_UPDATE;
    }
    else if (mode == 1) {
        if (BackT.start == 0) {
            BackT.start = clock();
        }
    }
    else if (mode == 2) {
        if (BackT.start != 0) {
            BackT.curr = clock();
            BackT.sum = BackT.curr - BackT.start;
            if (BackT.sum >= LOW_DOWN_TIME) {
                SnakeUpdateV = 5 + 3000 / BackT.sum;//15-5////////???????????
            }
            else {
                SnakeUpdateV = SNAKE_UPDATE;
            }
        }
        else {
            BackT.sum = 0;
            SnakeUpdateV = SNAKE_UPDATE;
        }
    }
}

void UpdateSnake(int nx, int ny) {
    //消去尾巴
    if (MapData[nx + 9][-ny + 9] != 2) {
        Print(XQueue.front(), YQueue.front(), 0);
        MapData[XQueue.front() + 9][-YQueue.front() + 9] = 0;
        XQueue.pop();
        YQueue.pop();
    }
    else {
        //吃到果子，立即刷新一个果子
        for (int i = 0; i < 3; i++) {
            if (EDot[i][0] == nx && EDot[i][1] == ny) {
                EDot[i][0] = 10;
                EDot[i][1] = 10;
                break;
            }
        }
        CreatDot();
    }

    //添加蛇头
    MapData[nx + 9][-ny + 9] = 1;//蛇头
    XQueue.push(nx);
    YQueue.push(ny);

}

void UpdateMap() {
    //清空，填充背景色
    setfillcolor(colors[0]);
    solidrectangle(308, 238, 772, 482);

    // 画棋盘表格
    setlinecolor(BLUE);             // 线条颜色
    setlinestyle(PS_SOLID, 3);     // 线型：实线，宽度3像素
    for (int i = 0; i < 20; i++) {//30,75
        line(75, 75 + 30 * i, 645, 75 + 30 * i);
        line(75 + 30 * i, 75, 75 + 30 * i, 645);
    }

    //显示title标语
    settextcolor(BLACK);
    outtextxy(75, 55, L"Hello, HungerSnake!");

    //打印果子
    for (int i = 0; i < 3; i++) {
        //printf("ESC EDot%d(%d,%d)\n", i, EDot[i][0], EDot[i][1]);
        if (EDot[i][0] != 10) {
            Print(EDot[i][0], EDot[i][1], 3);
        }
    }
}

void Update() {
    //打印蛇身/头
    int len = XQueue.size();
    for (int i = 0; i < len; i++) {
        Print(XQueue.front(), YQueue.front(), (i == (len - 1) ? 1 : 4));
        WheelQueue(XQueue.front(), YQueue.front());
    }

    //原点提示
    if (MapData[9][9] == 0) {
        Print(0, 0, 6);
    }

    //打印游玩时长
    wchar_t buffer[100];
    swprintf(buffer, 100, L"游戏时间：%llds               ", GameT.sum / 1000);
    outtextxy(800, 360, buffer);

    //打印速度
    swprintf(buffer, 100, L"当前速度：%d / %lld               ", 1000 / SnakeUpdateV, BackT.sum);
    outtextxy(800, 340, buffer);

    //打印菜单提示
    if (Sta == 3) {
        swprintf(buffer, 100, L"当前长度：%d                                          ", (int)XQueue.size());
        outtextxy(800, 380, buffer);
    }
    else if (Sta == 2) {
        swprintf(buffer, 100, L"暂停（按空格退出进程）                      ");
        outtextxy(800, 380, buffer);
    }
    else if (Sta == 1) {
        swprintf(buffer, 100, L"确定要退出吗（按空格退出进程）        ");
        outtextxy(800, 380, buffer);
    }

    //打印操作提示
    if (DMode == 0) {
        swprintf(buffer, 100, L"当前按下：↑              ");
    }
    else if (DMode == 1) {
        swprintf(buffer, 100, L"当前按下：→              ");
    }
    else if (DMode == 2) {
        swprintf(buffer, 100, L"当前按下：↓              ");
    }
    else if (DMode == 3) {
        swprintf(buffer, 100, L"当前按下：←              ");
    }
    else if (DMode == -1) {
        swprintf(buffer, 100, L"当前按下：ESC             ");
    }
    else if (DMode == -2) {
        swprintf(buffer, 100, L"当前按下：BaclSpace");
    }
    else {
        swprintf(buffer, 100, L"当前按下：None             ");
    }
    outtextxy(800, 400, buffer);
}

void UpdateMenu(int mode) {
    printf("menu更新，当前Sta%d\n", Sta);

    //画按钮和提示框
    if (Sta == 0) {
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
    }
    else {
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
        setfillcolor(RGB(50, 200, 200));//0Xc0c15e//250, 252, 156
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
    }

    //文字
    setbkmode(TRANSPARENT);
    if (mode == 1) {
        //画提示框
        settextcolor(BLACK);
        outtextxy(380, 418, L"极限");
        outtextxy(510, 418, L"竞速");
        outtextxy(640, 418, L"普通");
        outtextxy(527, 310, L"选择模式");
        //清空，填充背景色
        setfillcolor(colors[0]);
        solidrectangle(488, 498, 592, 552);
    }
    if (mode == 2) {
        settextcolor(BLACK);
        outtextxy(380, 418, L"返回开始");
        outtextxy(510, 418, L"重新开始");
        outtextxy(640, 418, L"继续游戏");
        outtextxy(527, 310, L"暂停");
    }
    setbkmode(OPAQUE);//TRANSPARENT
}

void Setting(int mode) {
    //0回到开始界面
    //1从开始界面跳到选择难度
    //2从游戏暂停设置到游戏继续
    //3从游戏中设置成游戏暂停
    //4重新开始游戏
    printf("开始调用Setting\n当前参数Sta：%d\n", Sta);
    int len;
    switch (mode) {
    case 0:case 1:
        Sta = mode;
        UpdateMenu(mode);
        break;
    case 2:
        Sta = 3;
        BackDownTime(0);//清空
        UpdateMap();
        GameTimeSetting(0);//开始计时
        break;
    case 3:
        Sta = 2;
        BackDownTime(0);
        GameTimeSetting(2);//暂停
        UpdateMenu(2);
        break;
    case 4:
        Sta = 3;
        BackDownTime(0);
        GameTimeSetting(3);//清空

        len = XQueue.size();
        //printf("len:%d\n", len);
        for (int i = len; i > 0; i--) {
            //printf("第%d个\n", i);
            MapData[XQueue.front() + 9][-YQueue.front() + 9] = 0;
            Print(XQueue.front(), YQueue.front(), 0);
            XQueue.pop();
            YQueue.pop();
        }

        for (int i = 0; i < 3; i++) {
            MapData[EDot[i][0] + 9][-EDot[i][1] + 9] = 0;
            Print(EDot[i][0], EDot[i][1], 0);
            EDot[i][0] = 10;
            EDot[i][1] = 10;
        }
        InitData();
        for (int i = 0; i < 3; i++) {//添加3个果子
            CreatDot();
        }
        Dir = 0;
        GameTimeSetting(0);
        break;
    case 5:
        BackDownTime(0);
        GameTimeSetting(3);//清空

        len = XQueue.size();
        //printf("len:%d\n", len);
        for (int i = len; i > 0; i--) {
            //printf("第%d个\n", i);
            MapData[XQueue.front() + 9][-YQueue.front() + 9] = 0;
            Print(XQueue.front(), YQueue.front(), 0);
            XQueue.pop();
            YQueue.pop();
        }

        for (int i = 0; i < 3; i++) {
            MapData[EDot[i][0] + 9][-EDot[i][1] + 9] = 0;
            Print(EDot[i][0], EDot[i][1], 0);
            EDot[i][0] = 10;
            EDot[i][1] = 10;
        }
        Dir = 0;
        Setting(0);
        break;
    }
}

void GetMsg() {
    if (peekmessage(&msg, EX_KEY | EX_MOUSE)) {
        if (msg.message == WM_KEYDOWN) {  // 按键按下
            switch (msg.vkcode) {
                //上右下左
            case VK_UP:   Dir = 0; DMode = 0; break;
            case VK_RIGHT:Dir = 1; DMode = 1; break;
            case VK_DOWN: Dir = 2; DMode = 2; break;
            case VK_LEFT: Dir = 3; DMode = 3; break;
                //ESC
            case VK_ESCAPE:
                DMode = -1;
                Setting(Sta);
                break;
                //空格
            case VK_SPACE:
                DMode = -2;
                if (Sta == 3) {
                    BackDownTime(1);
                }
                else {
                    Sta--;
                }
                break;
            }
        }
        if (msg.message == WM_KEYUP) {  // 按键松开
            printf("键盘按下，Sta=%d\n", Sta);
            //清空空格长按时间
            switch (msg.vkcode) {
            case VK_SPACE:
                BackDownTime(0); // BackT.start = 0;
                break;
            }

            //刷新按键码
            DMode = 4;
        }
        if (msg.message == WM_LBUTTONUP) {
            printf("鼠标点击，StaSta=%d\n", Sta);
            //检测鼠标按下（确定or取消）
            if (msg.x >= Larea.left && msg.x <= Larea.right && msg.y >= Larea.top && msg.y <= Larea.bottom) {
                //左
                if (Sta == 2) {
                    printf("鼠标调用Setting\n");
                    Setting(5);
                }
                printf("左\n");
            }
            if (msg.x >= Marea.left && msg.x <= Marea.right && msg.y >= Marea.top && msg.y <= Marea.bottom) {
                //中
                if (Sta == 2) {
                    printf("鼠标调用Setting\n");
                    Setting(4);
                }
                printf("中\n");
            }
            if (msg.x >= Rarea.left && msg.x <= Rarea.right && msg.y >= Rarea.top && msg.y <= Rarea.bottom) {
                //右
                if (Sta == 1) {
                    printf("鼠标调用Setting\n");
                    Setting(3);
                }
                if (Sta == 2) {
                    printf("鼠标调用Setting\n");
                    Setting(2);
                }
                printf("右\n");
            }
            if (msg.x >= Sarea.left && msg.x <= Sarea.right && msg.y >= Sarea.top && msg.y <= Sarea.bottom) {
                //开始
                if (Sta == 0) {
                    printf("鼠标调用Setting");
                    Setting(1);
                }
                printf("按下开始按钮\n");
            }
        }
    }
}

void GameTimeSetting(int mode) {
    //0更新游戏初始时间戳 1游戏时长自增 2暂停游戏时长自增 3清空计时
    if (mode == 0) {
        GameT.start = clock();
    }
    if (mode == 1) {
        //Start为0为游戏中
        GameT.curr = clock();
        GameT.sum = GameT.curr - GameT.start + GameT.item;
    }
    if (mode == 2) {
        //暂停状态
        GameT.item = GameT.sum;
    }
    if (mode == 3) {
        //清空
        GameT.item = 0;
        GameT.sum = 0;
    }
}

void WheelQueue(int x, int y) {
    XQueue.push(x);
    YQueue.push(y);
    XQueue.pop();
    YQueue.pop();
}

void Print(int x, int y, int color) {
    setfillcolor(colors[color]);
    solidrectangle(X_CODE_PIONE + x * 30, X_CODE_PIONE - y * 30, Y_CODE_PIONE + x * 30, Y_CODE_PIONE - y * 30);
}

void CreatDot() {
    //if (Sta != 3 || UdCnt % UpdateV != 0) return ;//限制刷新

    //获取随机数
    int x = Random() * (Random() > 4 ? 1 : -1);
    int y = Random() * (Random() > 4 ? 1 : -1);
    while (MapData[x + 9][-y + 9] != 0) {
        x = Random() * (Random() > 4 ? 1 : -1);
        y = Random() * (Random() > 4 ? 1 : -1);
    }

    //接入数组
    for (int i = 0; i < 3; i++) {
        if (EDot[i][0] == 10) {
            EDot[i][0] = x;
            EDot[i][1] = y;
            break;
        }
    }

    //接入MapData地图
    MapData[x + 9][-y + 9] = 2;

    //打印色块
    Print(x, y, 3);

    /*
    * //打印
    for (int i = 0; i < 3; i++) {
        printf("Creat EDotP %d:(%d,%d)\n", i, EDot[i][0], EDot[i][1]);
    }
    */
}

int Random() {
    return rand() % 10;
}



