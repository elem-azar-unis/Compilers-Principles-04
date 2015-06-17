#include <stdlib.h>
#include <string.h>
#include "tree.h"

#ifndef _SYMBOLS_H_
#define _SYMBOLS_H_

//链表头部插入宏。head为头指针，p为要插入元素的指针。
#define insert_head(head,p) \
	do \
	{ \
		(p)->next=(head); \
		(head)=(p); \
	}while(0)

struct type_d;
struct val_d;

//变量的类型：int，float，用户自定义类型（数组，结构体）
typedef enum val_kind
{
	_int,_float,USER_DEFINED
}val_kind;
//用户自定义类型：结构体，数组
typedef enum type_kind
{
	_struct,_array
}type_kind;

//数组定义的单元，数组定义用链表表示。
typedef struct array_def_list
{
	int dimension;					//该层的维度。从0层开始。即一级（如int a[]）是0层。
	int number;						//该层有几个元素
	int size_of_each;				//每个元素多大。（目前认为，struct的大小和int一样，即4字节）
	val_kind kind;					//基本元素是什么：int,float,struct
	struct type_d* val_type;		//struct定义指针，如果需要
	struct array_def_list* next;	//降一维度指针
}array_def_list;
//结构体定义单元。该结构体有几个域，每个域的定义
typedef struct struct_def_list
{
	int define_count;			//该结构体有几个域
	struct val_d** def_list;	//每个域的定义
}struct_def_list;

//变量定义单元。有函数参数，用户变量，结构体内变量。
typedef struct val_d
{
	char name[MAX_LEN_OF_NAME];		//名称
	int is_true_value;				//是否是真的变量，即是否可以直接使用，因为结构体中的域不能直接使用
	val_kind kind;					//类型：int，float，用户定义类型
	struct type_d* val_type;		//用户定义类型的定义结构体指针（如果需要）
	struct val_d* next;				//下一个单元地址
}val_d;
//类型定义单元。
typedef struct type_d
{
	char name[MAX_LEN_OF_NAME];	//名称。数组没有名称
	type_kind kind;				//类型：结构体，数组
	union
	{
		array_def_list* a;
		struct_def_list* s;
	}def;						//类型具体定义。a为数组定义地址，s为结构体定义地址
	struct type_d* next;		//下一个单元地址
}type_d;
//函数定义单元。
typedef struct func_d
{
	char name[MAX_LEN_OF_NAME];	//名称
	int parameter_count;		//参数个数
	val_kind* kinds;			//参数类型标志
	type_d** parameters;			//参数定义列表(如果某参数需要)
	val_kind return_kind;
	struct val_d** parameter_list;
	type_d* return_type;		//返回值类型定义
	struct func_d* next;		//下一个单元地址
}func_d;
//为了作用域而实现的变量定义表栈。利用头部插入，取头部，头部删除等实现。
typedef struct value_stack
{
	val_d* values;				//变量定义表
	struct value_stack* next;	//下一个
}value_stack;
#endif

//栈操作函数。push和pop，查看某一个变量能否在栈顶定义（true表示栈顶没有这个变量定义，false表示栈顶已有这个变量定义）。
void value_stack_push();
void value_stack_pop();
int value_stack_check(const char* name);

//下面初始化函数和整个符号表的析构函数。
void init_symbol_table();
void destroy_symbol_table();

//下面是3个构造函数，动态分配出新的表项，返回其地址。
type_d* new_type(const char* name);
func_d* new_function(const char* name);
val_d* new_value(const char* name);

//辅助函数，操作数组定义时候使用。分别为创建一个基类型层，以及在已经有基类型层基础上拓展一个维度。
void array_generate_basic_dimension(type_d* t,int number,val_kind kind,type_d* val_type);
void array_expand_dimension(type_d* t,int number);

//辅助函数，判断两个类型是否相等。
int type_equal(type_d* p,type_d* q);

//辅助函数。获得下一个别名,别名不会重复，一般而言不会与用户定义变量重名。用于匿名struct等场景使用。别名放在dest中。
void get_a_name(char* name);

//下面是3个查询函数。查询成功则返回对应定义指针，失败则返回NULL。
type_d* find_type(const char* name);
func_d* find_function(const char* name);
val_d* find_value(const char* name);

//下面是3个添加函数。在已经构造好表项后，将其添加进符号表中对应的位置。返回刚刚被添加进去的表项。
type_d* add_type_declaration(type_d* r);
func_d* add_function_declaration(func_d* r);
val_d* add_value_declaration(val_d* r);

//得到结构体的大小；得到一个域(一定在该结构体中存在)在结构体内从头部开始的偏移量。
int struct_get_size(type_d* s);
int struct_get_offset(type_d* s,char* field);

//int to string
void itoa(unsigned long val, char* buf,unsigned radix);