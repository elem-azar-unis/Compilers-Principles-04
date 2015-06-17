#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include "symbols.h"

#ifndef _IR_BUFFER_H_
#define _IR_BUFFER_H_
/* args_count=2:
 * 	GOTO x
 * 	RETURN x
 * 	ARG x
 * 	PARAM x
 * 	READ x
 * 	WRITE x
 * args_count=3:
 * 	LABEL x :
 * 	FUNCTION f :
 * 	x := y
 * 	x := &y
 * 	x := *y
 * 	*x := y
 * 	DEC x [size]
 * args_count=4:
 * 	x := CALL f
 * args_count=5:
 * 	x := y + z
 * 	x := y - z
 * 	x := y * z
 * 	x := y / z
 * args_count=6:
 * IF x [relop] y GOTO z
 */
//双向循环链表，存储代码在内存中的表示
typedef struct code_node
{
	struct code_node* prev;		//前一个
	struct code_node* next;		//后一个
	int args_count;				//有多少个词。具体见上面注释内的分类。
	char args[6][32];			//每个词都是什么
}code_node;
#endif

//生成新的label，名称放在提供好的name里面。
void new_label(char* name);
//生成新的临时变量，名称放在提供好的name里面。
void new_temp(char* name);
//添加一条代码，指明这条代码的词数，然后传入各个词语，各个词语都是char*，即传入多个字符串
void add_code(int args_count,...);
//将内存中的代码打印到文件中，传入新文件路径，并顺便清理内存中的代码存储
void print_code(char* name);
//关闭优化
void close_opt();