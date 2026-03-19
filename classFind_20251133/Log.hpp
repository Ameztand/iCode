#ifndef LOG_H
#define LOG_H

#ifdef __cplusplus
extern "C" {  // 告诉C++编译器按照C语言的命名规则编译
#endif
	// C语言风格的函数声

	//文件相关函数声明
	int initLog();
	void writeLog(const char* message);
	void closeLog();

#ifdef __cplusplus
}
#endif

#endif