#include "translate.h" 

//成对出现，保护两个量。
#define save() do{type_d*__current_struct=current_struct;int __current_size=current_size;
#define load() current_struct=__current_struct;current_size=__current_size;}while(0);

//当前的struct定义处，用于嵌套定义struct的访问。
static type_d* current_struct=NULL;
//当前数组单元大小。
static int current_size=4;
//当前函数参数表，用来确定某一个变量是结构体的引用还是结构体本身。
static func_d* current_func=NULL;

/* 
 * 翻译exp，其中的option表示place的主动、被动。 
 * 0：被动：place上层仅仅提供了位置，可以没有左值。你填写任何，上层会直接使用，比如填写立即数也可以。
 * 1：主动：place是由上层给定的，确定的变量，请将你的结果放入place
 * 如果place==NULL,则本层不要生成变量返回该exp的结果。
 */
void translate_exp(Node* h,char* place,int option);
//翻译COND，由上层提供跳转label
void translate_cond(Node*h,char* label_true,char* label_false);
//得到这个exp的地址。
void get_location(Node*h,char* place);
//翻译参数列表。需要当前函数定义中的参数表，当前是第几个参数。
void translate_args(Node* h,val_d** args,int count);

void translate(Node* h)
{
	if(h==NULL)return;
	switch(h->type)
	{
		case Specifier:return;
		case FunDec:
		{
			func_d* temp=find_function(h->child[0]->name);
			add_code(3,"FUNCTION",h->child[0]->name,":");
			for(int i=0;i<temp->parameter_count;i++)
				add_code(2,"PARAM",temp->parameter_list[i]->name);
			current_func=temp;
			break;
		}
		case Dec:
		{
			Node* p=h->child[0]->child[0];
			if(p->type!=_ID)
			{
				p=p->child[0];
				if(p->type!=_ID)
				{
					printf("Cannot translate: Code contains variables of multi-dimensional \
					array type or parameters of array type.\n");
					exit(0);
				}
			}
			val_d* temp=find_value(p->name);
			if(temp->kind==USER_DEFINED)
			{
				char length[8];
				itoa(struct_get_size(temp->val_type),length,10);
				add_code(3,"DEC",temp->name,length);
			}
			if(h->child_count==3)
			{
				translate_exp(h->child[2],temp->name,1);
			}
			break;
		}
		case Stmt:
		{
			//根据第一个子结点的不同，分别作不同的处理
			switch(h->child[0]->type)
			{
				case Exp:
				{
					translate_exp(h->child[0],NULL,0);
					break;
				}
				case CompSt:
				{
					translate(h->child[0]);
					break;
				}
				case _RETURN:
				{
					char temp[32];
					translate_exp(h->child[1],temp,0);
					add_code(2,"RETURN",temp);
					break;
				}
				case _IF:
				{
					char label1[32],label2[32],label3[32];
					new_label(label1);
					new_label(label2);
					if(h->child_count==7)
					{
						new_label(label3);
					}
					translate_cond(h->child[2],label1,label2);
					add_code(3,"LABEL",label1,":");
					translate(h->child[4]);
					if(h->child_count==7)
						add_code(2,"GOTO",label3);
					add_code(3,"LABEL",label2,":");
					if(h->child_count==7)
					{
						translate(h->child[6]);
						add_code(3,"LABEL",label3,":");
					}
					break;
				}
				case _WHILE:
				{
					char label1[32],label2[32],label3[32];
					new_label(label1);
					new_label(label2);
					new_label(label3);
					add_code(3,"LABEL",label1,":");
					translate_cond(h->child[2],label2,label3);
					add_code(3,"LABEL",label2,":");
					translate(h->child[4]);
					add_code(2,"GOTO",label1);
					add_code(3,"LABEL",label3,":");
					break;
				}
			}
			break;
		}
		default:
		{
			for(int i=0;i<h->child_count;i++)
				translate(h->child[i]);
			break;
		}
	}
}

void translate_exp(Node* h,char* place,int option)
{
	if(h->child[0]->type==_LP)
	{
		return translate_exp(h->child[1],place,option);
	}
	else if(h->child_count==3 && h->child[1]->type==_ASSIGNOP)
	{
		char temp[32];
		translate_exp(h->child[0],temp,0);
		translate_exp(h->child[2],temp,1);
		if(place!=NULL)
		{
			if(option==0)
			{
				strcpy(place,temp);
			}
			else
			{
				add_code(3,place,":=",temp);
			}
		}
	}
	else if(h->child_count==3 && h->child[1]->type==_LP)
	{
		if(place==NULL)
		{
			if(strcmp(h->child[0]->name,"read")==0)
				return;
			char temp[32];
			new_temp(temp);
			add_code(4,temp,":=","CALL",h->child[0]->name);
		}
		else
		{
			if(option==0)
				new_temp(place);
			if(strcmp(h->child[0]->name,"read")==0)
				add_code(2,"READ",place);
			else
				add_code(4,place,":=","CALL",h->child[0]->name);
		}
	}
	else if(h->child_count==4 && h->child[1]->type==_LP)
	{
		if(place==NULL)
		{
			if(strcmp(h->child[0]->name,"write")==0)
			{
				char temp[32];
				translate_exp(h->child[2]->child[0],temp,0);
				add_code(2,"WRITE",temp);
			}
			else
			{
				func_d* f=find_function(h->child[0]->name);
				translate_args(h->child[2],f->parameter_list,0);
				char temp[32];
				new_temp(temp);
				add_code(4,temp,":=","CALL",h->child[0]->name);
			}
		}
		else
		{
			if(strcmp(h->child[0]->name,"write")==0)
			{
				char temp[32];
				translate_exp(h->child[2]->child[0],temp,0);
				add_code(2,"WRITE",temp);
				if(option==0)
				{
					place[0]='#';
					place[1]='0';
					place[2]='\0';
				}
				else
				{
					add_code(3,place,":=","#0");
				}
			}
			else
			{
				func_d* f=find_function(h->child[0]->name);
				translate_args(h->child[2],f->parameter_list,0);
				if(option==0)
					new_temp(place);
				add_code(4,place,":=","CALL",h->child[0]->name);
			}
		}
	}
	if(place==NULL)return;
	if(h->child[0]->type==_INT)
	{
		if(option==0)
		{
			sprintf(place,"#%d",h->child[0]->value_i);
		}
		else
		{
			char temp[32];
			sprintf(temp,"#%d",h->child[0]->value_i);
			add_code(3,place,":=",temp);
		}
	}
	else if(h->child[0]->type==_ID && h->child_count==1)
	{
		if(option==0)
		{
			strcpy(place,h->child[0]->name);
		}
		else
		{
			add_code(3,place,":=",h->child[0]->name);
		}
	}
	else if(h->child_count==3 && (h->child[1]->type==_PLUS || h->child[1]->type==_MINUS || h->child[1]->type==_STAR || h->child[1]->type==_DIV))
	{
		char temp1[32],temp2[32];
		translate_exp(h->child[0],temp1,0);
		if((temp1[0]=='#' && temp1[1]=='0') && (h->child[1]->type==_STAR || h->child[1]->type==_DIV))
		{
			if(option==0)
			{
				place[0]='#';
				place[1]='0';
				place[2]='\0';
			}
			else
			{
				add_code(3,place,":=","#0");
			}
		}
		translate_exp(h->child[2],temp2,0);
		if((temp2[0]=='#' && temp2[1]=='0') && (h->child[1]->type==_STAR || h->child[1]->type==_DIV))
		{
			if(option==0)
			{
				place[0]='#';
				place[1]='0';
				place[2]='\0';
			}
			else
			{
				add_code(3,place,":=","#0");
			}
		}
		else if(temp1[0]=='#' && temp2[0]=='#')
		{
			int i1=strtol(&temp1[1],NULL,10);
			int i2=strtol(&temp2[1],NULL,10);
			if(h->child[1]->type==_PLUS)
			{
				if(option==0)
				{
					sprintf(place,"#%d",i1+i2);
				}
				else
				{
					char temp[32];
					sprintf(temp,"#%d",i1+i2);
					add_code(3,place,":=",temp);
				}
			}
			else if(h->child[1]->type==_MINUS)
			{
				if(option==0)
				{
					sprintf(place,"#%d",i1-i2);
				}
				else
				{
					char temp[32];
					sprintf(temp,"#%d",i1-i2);
					add_code(3,place,":=",temp);
				}
			}
			else if(h->child[1]->type==_STAR)
			{
				if(option==0)
				{
					sprintf(place,"#%d",i1*i2);
				}
				else
				{
					char temp[32];
					sprintf(temp,"#%d",i1*i2);
					add_code(3,place,":=",temp);
				}
			}
			else
			{
				if(option==0)
				{
					sprintf(place,"#%d",i1/i2);
				}
				else
				{
					char temp[32];
					sprintf(temp,"#%d",i1/i2);
					add_code(3,place,":=",temp);
				}
			}
		}
		else if(temp2[0]=='#' && temp2[1]=='0')
		{
			if(option==0)
				strcpy(place,temp1);
			else
				add_code(3,place,":=",temp1);
		}
		else if(temp1[0]=='#' && temp1[1]=='0' && h->child[1]->type==_PLUS)
		{
			if(option==0)
				strcpy(place,temp2);
			else
				add_code(3,place,":=",temp2);
		}
		else
		{
			char op[2];
			op[1]='\0';
			if(h->child[1]->type==_PLUS) op[0]='+';
			else if(h->child[1]->type==_MINUS) op[0]='-';
			else if(h->child[1]->type==_STAR) op[0]='*';
			else op[0]='/';
			if(option==0)
			{
				new_temp(place);
			}
			add_code(5,place,":=",temp1,op,temp2);
		}
	}
	else if(h->child[0]->type==_MINUS)
	{
		char temp[32];
		translate_exp(h->child[1],temp,0);
		if(temp[0]=='#')
		{
			int i=strtol(&temp[1],NULL,10);
			if(option==0)
			{
				sprintf(place,"#%d",-i);
			}
			else
			{
				char temp[32];
				sprintf(temp,"#%d",-i);
				add_code(3,place,":=",temp);
			}
		}
		else
		{
			if(option==0)
			{
				new_temp(place);
			}
			add_code(5,place,":=","#0","-",temp);
		}
	}
	else if(h->child[0]->type==_NOT || h->child[1]->type==_RELOP || h->child[1]->type==_AND || h->child[1]->type==_OR)
	{
		char label1[32],label2[32];
		new_label(label1);
		new_label(label2);
		if(option==0)
			new_temp(place);
		add_code(3,place,":=","#0");
		translate_cond(h,label1,label2);
		add_code(3,"LABEL",label1,":");
		add_code(3,place,":=","#1");
		add_code(3,"LABEL",label2,":");
	}
	else if(h->child_count==4 && h->child[1]->type==_LB)
	{
		char temp1[32],temp2[32];
		get_location(h->child[0],temp1);
		translate_exp(h->child[2],temp2,0);
		if(temp2[0]=='#'&& temp2[1]=='0')
		{
			char temp[32];
			if(temp1[0]=='&')
				strcpy(temp,&temp1[1]);
			else
				sprintf(temp,"*%s",temp1);
			if(option==0)
			{
				strcpy(place,temp);
			}
			else
			{
				add_code(3,place,":=",temp);
			}
		}
		else if(temp2[0]=='#')
		{
			int i=strtol(&temp2[1],NULL,10);
			char temp[32],tempi[32];
			sprintf(tempi,"#%d",i*current_size);
			new_temp(temp);
			add_code(5,temp,":=",temp1,"+",tempi);
			char result[32];
			sprintf(result,"*%s",temp);
			if(option==0)
				strcpy(place,result);
			else
				add_code(3,place,":=",result);
		}
		else
		{
			char temp[32],tempi[32],result[32],num[32];
			sprintf(num,"#%d",current_size);
			new_temp(temp);
			new_temp(tempi);
			add_code(5,tempi,":=",temp2,"*",num);
			add_code(5,temp,":=",temp1,"+",tempi);
			sprintf(result,"*%s",temp);
			if(option==0)
				strcpy(place,result);
			else
				add_code(3,place,":=",result);
		}
	}
	else if(h->child_count==3 && h->child[1]->type==_DOT)
	{
		close_opt();
		char left[32],temp[32],result[32];
		get_location(h->child[0],left);
		int offseti=struct_get_offset(current_struct,h->child[2]->name);
		char offset[32];
		sprintf(offset,"#%d",offseti);
		new_temp(temp);
		add_code(5,temp,":=",left,"+",offset);
		sprintf(result,"*%s",temp);
		if(option==0)
			strcpy(place,result);
		else
			add_code(3,place,":=",result);
	}
}
void get_location(Node*h,char* place)
{
	if(h->child_count==1)
	{
		val_d* va=find_value(h->child[0]->name);
		if(va->val_type->kind==_struct)
			current_struct=va->val_type;
		else
			current_size=va->val_type->def.a->size_of_each;
		for(int i=0;i<current_func->parameter_count;i++)
			if(current_func->parameter_list[i]==va)
			{
				strcpy(place,h->child[0]->name);
				return;
			}
		sprintf(place,"&%s",h->child[0]->name);
	}
	else if(h->child_count==4 && h->child[1]->type==_LB)
	{
		char temp1[32],temp2[32];
		get_location(h->child[0],temp1);
		save();
		translate_exp(h->child[2],temp2,0);
		load();
		if(temp2[0]=='#'&& temp2[1]=='0')
		{
			strcpy(place,temp1);
		}
		else if(temp2[0]=='#')
		{
			int i=strtol(&temp2[1],NULL,10);
			char temp[32],tempi[32];
			new_temp(temp);
			sprintf(tempi,"#%d",i*current_size);
			add_code(5,temp,":=",temp1,"+",tempi);
			strcpy(place,temp);
		}
		else
		{
			char temp[32],tempi[32],num[32];
			sprintf(num,"#%d",current_size);
			new_temp(temp);
			new_temp(tempi);
			add_code(5,tempi,":=",temp2,"*",num);
			add_code(5,temp,":=",temp1,"+",tempi);
			strcpy(place,temp);
		}
	}
	else
	{
		close_opt();
		char left[32];
		get_location(h->child[0],left);
		int offseti=struct_get_offset(current_struct,h->child[2]->name);
		char offset[32];
		sprintf(offset,"#%d",offseti);
		new_temp(place);
		add_code(5,place,":=",left,"+",offset);
		val_d* va=find_value(h->child[2]->name);
		if(va->kind==USER_DEFINED)
		{
			if(va->val_type->kind==_array)
				current_size=va->val_type->def.a->size_of_each;
			if(va->val_type->kind==_struct)
				current_struct=va->val_type;
			else if(va->val_type->def.a->kind==USER_DEFINED)
				current_struct=va->val_type->def.a->val_type;
		}
	}
}
void translate_cond(Node*h,char* label_true,char* label_false)
{
	if(h->child[0]->type==_NOT)
		return translate_cond(h->child[1],label_false,label_true);
	if(h->child_count==3 && h->child[1]->type==_RELOP)
	{
		char temp1[32],temp2[32];
		translate_exp(h->child[0],temp1,0);
		translate_exp(h->child[2],temp2,0);
		if(temp1[0]=='#' && temp2[0]=='#')
		{
			int i1=strtol(&temp1[1],NULL,10);
			int i2=strtol(&temp2[1],NULL,10);
			if(strcmp(h->child[1]->name,">")==0)
			{
				if(i1>i2)
					add_code(2,"GOTO",label_true);
				else
					add_code(2,"GOTO",label_false);
			}
			if(strcmp(h->child[1]->name,"<")==0)
			{
				if(i1<i2)
					add_code(2,"GOTO",label_true);
				else
					add_code(2,"GOTO",label_false);
			}
			if(strcmp(h->child[1]->name,">=")==0)
			{
				if(i1>=i2)
					add_code(2,"GOTO",label_true);
				else
					add_code(2,"GOTO",label_false);
			}
			if(strcmp(h->child[1]->name,"<=")==0)
			{
				if(i1<=i2)
					add_code(2,"GOTO",label_true);
				else
					add_code(2,"GOTO",label_false);
			}
			if(strcmp(h->child[1]->name,"==")==0)
			{
				if(i1==i2)
					add_code(2,"GOTO",label_true);
				else
					add_code(2,"GOTO",label_false);
			}
			if(strcmp(h->child[1]->name,"!=")==0)
			{
				if(i1!=i2)
					add_code(2,"GOTO",label_true);
				else
					add_code(2,"GOTO",label_false);
			}
		}
		else
		{
			add_code(6,"IF",temp1,h->child[1]->name,temp2,"GOTO",label_true);
			add_code(2,"GOTO",label_false);
		}
	}
	else if(h->child_count==3 && h->child[1]->type==_AND)
	{
		char label[32];
		new_label(label);
		translate_cond(h->child[0],label,label_false);
		add_code(3,"LABEL",label,":");
		translate_cond(h->child[2],label_true,label_false);
	}
	else if(h->child_count==3 && h->child[1]->type==_OR)
	{
		char label[32];
		new_label(label);
		translate_cond(h->child[0],label_true,label);
		add_code(3,"LABEL",label,":");
		translate_cond(h->child[2],label_true,label_false);
	}
	else
	{
		char temp[32];
		translate_exp(h,temp,0);
		if(temp[0]=='#')
		{
			if(temp[1]=='0')
				add_code(2,"GOTO",label_false);
			else
				add_code(2,"GOTO",label_true);
		}
		else
		{
			add_code(6,"IF",temp,"!=","#0","GOTO",label_true);
			add_code(2,"GOTO",label_false);
		}
	}
}
void translate_args(Node* h,val_d** args,int count)
{
	if(h->child_count==3)
		translate_args(h->child[2],args,count+1);
	if(args[count]->kind!=USER_DEFINED)
	{
		char temp[32];
		translate_exp(h->child[0],temp,0);
		add_code(2,"ARG",temp);
	}
	else if(args[count]->val_type->kind==_struct)
	{
		char temp[32];
		get_location(h->child[0],temp);
		add_code(2,"ARG",temp);
	}
	else
	{
		printf("can't deal with using array as args.\n");
		exit(0);
	}
}