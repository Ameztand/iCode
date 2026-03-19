/*
我是先写DFS再写BFS，感觉BFS更难，在画布之类的框架都搭建好的情况下用了和DFS差不多的时间。
而且，还有很多小瑕疵，相比DFS更粗糙。（比如后面判断路线的i的边界值没处理好，用魔数强行调
配的）
只能说能跑就行（苦笑）

整体思路就是，用一个大数组实现队列（记录步骤，坐标，还有父节点的下标），用两个不断向前推
进的指针“划出”队列范围，再用一个临时变量i在队列里遍历。用一个变量记录这一轮增加了几个可
能性，用来推进队列边界指针的移动距离后面路本来逆向回溯就可以了，我多加了一步，把路线copy
到另一个小数组，然后反推来实现“从起点到终点寻路”的可视化动画，导致代码粗糙了不少。

说实话，与其加入更多可视化内容/DLC的内容，不如好好提炼一下核心代码
*/

#include<stdio.h>
#include <graphics.h>
#include <conio.h>
#include <windows.h>  // 为了 Sleep

void initEaxyX();
void Print(int x, int y, int sign);

//恒量表
int X = 1100, Y = 20, V = 100;
int front = 0, len = 1, step = 1;

int dx[4] = { 0,1,0,-1 };//上右下左
int dy[4] = { -1,0,1,0 };

int queue[100][4];//0下标就是step,12是坐标，3是父节点
int inn[100][2];
int buMap[10][10];
int mapData[10][10] = {  //0为路1为墙
        {0,1,0,0,0,1,0,0,0,1},
        {0,0,0,1,0,1,1,1,0,1},
        {1,1,0,1,0,0,0,0,0,1},
        {0,1,0,1,0,1,0,1,0,1},
        {0,1,1,1,0,0,0,1,0,1},
        {0,0,0,0,0,1,1,1,0,0},
        {1,1,0,1,1,1,0,1,1,0},
        {0,0,0,1,0,1,0,0,0,0},
        {0,1,0,1,0,0,0,1,0,1},
        {0,1,0,0,0,1,0,0,0,0}
};


int main()
{
    //加载图像
    initEaxyX();

    //寻路主循环
    int x = 0, y = 0;
    int nx, ny;
    int L;

    queue[0][0] = 1;
    queue[0][1] = 0;
    queue[0][2] = 0;
    queue[0][3] = -1;

    while (1) {
        L = 0;
        printf("开始扩散(%d,%d)\n", x, y);

        for (int i = front; i < len; i++) {  //同步骤队伍推进
            x = queue[i][1];
            y = queue[i][2];
            for (int j = 0; j < 4; j++) {    //方向循环
                nx = x + dx[j];
                ny = y + dy[j];
                printf("(%d,%d)", nx, ny);
                //新节点入队
                if (nx >= 0 && ny >= 0 && nx <= 9 && ny <= 9 && buMap[ny][nx] == 0) {
                    queue[len + L][0] = step + 1;
                    queue[len + L][1] = nx;
                    queue[len + L][2] = ny;
                    queue[len + L][3] = i;
                    Print(nx, ny, 0);
                    L++;
                    printf("jem\n");
                    continue;
                }
                printf("none\n");
            }
            printf("F/i/L:%d/%d/%d\n", front, i, len);
            if (x == 9 && y == 9) {
                setfillcolor(YELLOW);
                solidrectangle(569, 569, 618, 618);
                Print(x, y, i);
                break;
            }
        }

        front = len;
        len += L;
        step++;

        printf("step:%d\n", step);
        printf("front/len:%d/%d\n", front, len);
        if (x == 9 && y == 9) {
            break;
        }
        Sleep(V);
    }


    _getch();
    closegraph();
    return 0;
}

void initEaxyX()
{
    initgraph(1280, 720);          //生成画布
    setbkcolor(WHITE);             //设置背景（白）
    cleardevice();                 //清空，填充背景色

    // 画表格
    setlinecolor(BLUE);             // 线条颜色
    setlinestyle(PS_SOLID, 3);     // 线型：实线，宽度3像素

    for (int i = 0; i <= 10; i++) {
        moveto(100, 100 + 52 * i);
        lineto(620, 100 + 52 * i);
    }
    for (int i = 0; i <= 10; i++) {
        moveto(100 + 52 * i, 100);
        lineto(100 + 52 * i, 620);
    }

    //显示墙
    settextcolor(BLACK);
    outtextxy(100, 80, L"Hello, EasyX!");

    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            if (mapData[i][j] == 1) {
                outtextxy(122 + 52 * j, 120 + 52 * i, L"#");
            }
        }
    }

    //起点与路径
    setfillcolor(GREEN);
    /*
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            if (mapData[i][j] == 1) {
                solidrectangle(101 + 52 * i, 101 + 52 * j, 150 + 52 * i, 150 + 52 * j);
            }
        }
    }
    */
    setfillcolor(CYAN);
    solidrectangle(101, 101, 150, 150);
    setfillcolor(YELLOW);
    solidrectangle(569, 569, 618, 618);

    //复制地图
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            buMap[i][j] = -mapData[i][j];//把墙的1变成-1
        }
    }
    buMap[0][0] = -1;//起点
}

void Print(int x, int y, int sign) {
    if (sign == 0) {
        //填色
        setfillcolor(step * 5 + 100);//设置填充色块颜色
        solidrectangle(101 + 52 * x, 101 + 52 * y, 150 + 52 * x, 150 + 52 * y);
        //更新地图
        buMap[y][x] += step;
    }
    else {
        int item = sign;
        int i = 0;
        while (i < step - 2) {//为什么-2？
            item = queue[item][3];
            inn[i][0] = queue[item][1];
            inn[i][1] = queue[item][2];
            i++;
            //Sleep(V);
        }
        printf("i:%d\n", i);
        while (i > 0) {
            x = inn[i - 1][0];
            y = inn[i - 1][1];
            //打印画布右侧
            wchar_t buffer[100];
            swprintf(buffer, 100, L"坐标: (%d, %d)", x, y);
            outtextxy(X, Y + 20 * (step - i - 2), buffer);
            //填色
            setfillcolor(GREEN);//设置填充色块颜色
            solidrectangle(101 + 52 * x, 101 + 52 * y, 150 + 52 * x, 150 + 52 * y);
            i--;
            Sleep(V);
        }
    }
}


