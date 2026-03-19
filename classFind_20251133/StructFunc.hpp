#ifndef STRUCTFUNC_H
#define STRUCTFUNC_H

#ifdef __cplusplus
extern "C" {  // 告诉C++编译器按照C语言的命名规则编译
#endif
	// C语言风格的函数声

	//结构
	struct StuNode {
		int num;
		//学号
		char name[16];
		char NumDorm[16];
		char NumPhone[12];
		char NumCard[19];
		struct StuNode* next;
	};

	

	//结构变量声明
	struct StuNode* Add(struct StuNode* head, char buffer[256], int cnt);             //添加
	struct StuNode* Delete(struct StuNode* head, int num);                            //删除
	void Find(struct StuNode* head, char* ch, int choice);                            //修改打印
	void saveFile(struct StuNode* head, FILE* dataFile);                              //写入文件

	void clean_input_buffer(void);      //清理缓冲区                                                

#ifdef __cplusplus
}
#endif

#endif
