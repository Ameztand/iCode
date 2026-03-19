#define _CRT_SECURE_NO_WARNINGS


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "StructFunc.hpp"
#include "Log.hpp"

//初始化函数和关闭函数声明
int init(FILE* dataFile);
void close(FILE* dataFile);

struct StuNode* head = NULL;

int main() {
    //初始化
    FILE* dataFile = NULL;
    if (init(dataFile) == 1)return 1;
    
    //操作
    int choice=-1;   //功能代数

    int num=-3;            //结构对应成员缓冲
    char name[16];
    char NumDorm[16];
    char NumPhone[12];
    char NumCard[19];

    char buffer[256];     //整体缓冲

    int cnt=-1;           //次功能代数
    char ch[19];          //次功能缓冲

    struct StuNode* p = head;
    struct StuNode* nextnode = NULL;

    while (choice != 0) {
        //菜单输入
        printf("1添加 2删除 3查找 4修改 5保存 0退出\n");
        scanf("%2d", &choice);
        clean_input_buffer();

        //辨别代码
        switch (choice) {
        case 1:
            //插入
            printf("请输入需要添加的序号\n");
            scanf("%3d", &num); clean_input_buffer();

            //按零退出
            if (num == 0)break;

            printf("请输入需要添加的名字\n"); scanf("%15s",name); clean_input_buffer();
            printf("请输入需要添加的宿舍号\n"); scanf("%15s",NumDorm); clean_input_buffer();
            printf("请输入需要添加的手机号\n"); scanf("%11s",NumPhone); clean_input_buffer();
            printf("请输入需要添加的身份证号\n"); scanf("%18s",NumCard); clean_input_buffer();
            sprintf(buffer, "%d\t%s\t%s\t%s\t%s", num, name, NumDorm, NumPhone, NumCard);

            printf("请输入需要添加的位置（-1添加到末尾）\n");
            scanf("%3d", &cnt);
            clean_input_buffer();

            while (cnt < -1) {
                printf("非法数值！\n");
                scanf("%3d", &cnt);
                clean_input_buffer();
            }

            head = Add(head, buffer,cnt);
            break;
        case 2:
            //删除
            printf("请输入需要删除的序号\n");;
            scanf("%3d", &num);
            clean_input_buffer();

            //按零退出
            if (num == 0)break;

            while (num < 0) {
                printf("非法数值！\n");
                scanf("%3d", &num);
                clean_input_buffer();
            }
            
            head = Delete(head, num);
            break;
        case 3:case 4:
            //修改或查找
            printf("提示：\n");
            printf("%s0退出 @序号查询 #名字查询\n",(choice==3?"-1全部打印 ":""));

            scanf("%18s", ch);
            clean_input_buffer();

            if (ch[0] == '0') {
                break;
            }
            else if (strcmp(ch, "-1") == 0) {
                cnt=-1;
            }
            else {
                cnt=choice-3;
            }

            //printf("%d\n", cnt);
            Find(head,ch,cnt);//-1打印 0查找 1修改 
            break;
        case 5:
            //保存
            saveFile(head,dataFile);
            break;
        case 0:
            //退出
            printf("关闭系统...\n");
            //释放链表
            cnt = 0;
            while (p != NULL) {
                nextnode = p->next;
                free(p);
                p = nextnode;
                cnt++;
            }
            if (cnt != 0) {
                writeLog("成功释放链表");
            }
            else {
                writeLog("释放链表失败");
            }
            
            break;
        default:
            printf("非法数字！\n");
            break;
        }

        //重置
        memset(name, '\0', sizeof(name));
        memset(NumDorm, '\0', sizeof(NumDorm));
        memset(NumPhone, '\0', sizeof(NumPhone));
        memset(NumCard, '\0', sizeof(NumCard));
        memset(ch, '\0', sizeof(ch));
        num = -3, cnt = -1;
    }
    
    //关闭文件
    close(dataFile);
    printf("感谢您的使用，再见！\n");

    return 0;
}

//初始化
int init(FILE* dataFile) {
    //日志初始化
    printf("初始化...\n");
    if (initLog() != 0) {
        return 1;
    }

    //data文件初始化
    dataFile = fopen("classdata.txt", "r");
    if (dataFile == NULL) {
        printf("数据文件打开失败！\n");
        return 1;
    }
    writeLog("数据文件打开");

    //链表初始化
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), dataFile) != NULL) {    //读取
        buffer[strcspn(buffer, "\n")] = '\0';                    // 去除换行符
        head = Add(head, buffer, -2);
    }

    if (head == NULL) {
        writeLog("数据加载失败");
        return 1;
    }

    writeLog("数据加载成功");
    return 0;
}

// 关闭文件
void close(FILE* dataFile) {
    // 关闭data文件
    if (dataFile) {
        writeLog("数据文件关闭");
        fclose(dataFile);
        dataFile = NULL;
    }
    // 关闭log文件
    closeLog();
}

