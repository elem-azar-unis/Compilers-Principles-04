#include "dst_code.h" 
typedef struct var_loc
{
	char name[32];
	int loc;
}var;

static code_node* head;
FILE* fp;
static int arg_count=0;
static int size=0;
static int count=0;
static var table[1024];

int var_get(char* name)
{
	if(name[0]=='#')return -1;
	if(name[0]=='*'||name[0]=='&')name++;
	for(int i=0;i<count;i++)
	{	
		if(strcmp(name,table[i].name)==0)
			return table[i].loc;
	}
	return -1;
}
void var_add(char* name,int sz)
{
	if(name[0]=='#')return;
	if(name[0]=='*'||name[0]=='&')name++;
	if(var_get(name)!=-1)return;
	table[count].loc=size;
	size+=sz;
	strcpy(table[count].name,name);
	count++;
}
void prep_reg(char* name,int num)
{
	char temp[8];
	sprintf(temp,"$t%d",num);
	if(name[0]=='*')
	{
		fprintf(fp,"lw %s,%d($sp)\n",temp,var_get(name));
		fprintf(fp,"add %s,%s,$sp\n",temp,temp);
		fprintf(fp,"lw %s,0(%s)\n",temp,temp);
	}
	else if(name[0]=='&')
	{
		fprintf(fp,"li %s,%d\n",temp,var_get(name));
	}
	else if(name[0]=='#')
	{
		fprintf(fp,"li %s,%s\n",temp,&name[1]);
	}
	else
	{
		fprintf(fp,"lw %s,%d($sp)\n",temp,var_get(name));
	}
}

//翻译一个函数，两参数左闭右开。
void gen_one_function(code_node* begin,code_node* end);
void gen_dst_code(code_node* h,FILE* f)
{
	head=h;
	fp=f;
	fprintf(fp,"%s\n",".data");
	fprintf(fp,"%s\n","_prompt: .asciiz \"Enter an integer:\"");
	fprintf(fp,"%s\n","_ret: .asciiz \"\\n\"");
	fprintf(fp,"%s\n",".globl main");
	fprintf(fp,"%s\n",".text");
	fprintf(fp,"%s\n","read:");
	fprintf(fp,"%s\n","li $v0, 4");
	fprintf(fp,"%s\n","la $a0, _prompt");
	fprintf(fp,"%s\n","syscall");
	fprintf(fp,"%s\n","li $v0, 5");
	fprintf(fp,"%s\n","syscall");
	fprintf(fp,"%s\n","jr $ra");
	fprintf(fp,"%s\n","write:");
	fprintf(fp,"%s\n","li $v0, 1");
	fprintf(fp,"%s\n","syscall");
	fprintf(fp,"%s\n","li $v0, 4");
	fprintf(fp,"%s\n","la $a0, _ret");
	fprintf(fp,"%s\n","syscall");
	fprintf(fp,"%s\n","move $v0, $0");
	fprintf(fp,"%s\n","jr $ra");
	code_node* p=head,*q;
	do
	{
		q=p->next;
		while(1)
		{
			if(strcmp(q->args[0],"FUNCTION")==0)
				break;
			else if(q==head) break;
			q=q->next;
		}
		gen_one_function(p,q);
		p=q;
	}while(p!=head);
}
void gen_one_function(code_node* begin,code_node* end)
{
	count=0;
	size=0;
	arg_count=0;
	fprintf(fp,"%s:\n",begin->args[1]);
	code_node* p=begin->next;
	while(p!=end)
	{
		switch(p->args_count)
		{
			case 2:
			{
				if(strcmp("GOTO",p->args[0])!=0)
					var_add(p->args[1],4);
				break;
			}
			case 3:
			{
				if(strcmp(p->args[1],":=")==0)
				{
					var_add(p->args[0],4);
					var_add(p->args[2],4);
				}
				else if(strcmp(p->args[0],"DEC")==0)
				{
					int a=strtol(p->args[2],NULL,10);
					var_add(p->args[1],a);
				}
				break;
			}
			case 4:
			{
				var_add(p->args[0],4);
				break;
			}
			case 5:
			{
				var_add(p->args[0],4);
				var_add(p->args[2],4);
				var_add(p->args[4],4);
				break;
			}
			case 6:
			{
				var_add(p->args[1],4);
				var_add(p->args[3],4);
				break;
			}
			default:;
		}
		p=p->next;
	}
	fprintf(fp,"addi $sp, $sp, -%d\n",size);
	p=begin->next;
	while(p!=end)
	{
		/*
		fprintf(fp,"#%s",p->args[0]);
		for(int i=1;i<p->args_count;i++)
			fprintf(fp," %s",p->args[i]);
		fprintf(fp,"\n");
		*/
		switch(p->args_count)
		{
			case 2:
			{
				if(strcmp(p->args[0],"GOTO")==0)
				{
					fprintf(fp,"j %s\n",p->args[1]);
				}
				else if(strcmp(p->args[0],"RETURN")==0)
				{
					prep_reg(p->args[1],0);
					fprintf(fp,"move $v0,$t0\n");
					fprintf(fp,"addi $sp, $sp, %d\n",size);
					fprintf(fp,"jr $ra\n");
				}
				else if(strcmp(p->args[0],"ARG")==0)
				{
					prep_reg(p->args[1],0);
					fprintf(fp,"move $a%d,$t0\n",arg_count);
					arg_count++;
				}
				else if(strcmp(p->args[0],"PARAM")==0)
				{
					int para_count=0;
					code_node* q=p;
					while(strcmp(q->next->args[0],"PARAM")==0)
					{
						q=q->next;
						para_count++;
					}
					fprintf(fp,"sw $a%d,%d($sp)\n",para_count,var_get(p->args[1]));
				}
				else if(strcmp(p->args[0],"READ")==0)
				{
					fprintf(fp,"addi $sp, $sp, -4\n");
					fprintf(fp,"sw $ra, 0($sp)\n");
					fprintf(fp,"jal read\n");
					fprintf(fp,"lw $ra, 0($sp)\n");
					fprintf(fp,"addi $sp, $sp, 4\n");
					if(p->args[1][0]=='*')
					{
						fprintf(fp,"lw $t0, %d($sp)\n",var_get(p->args[1]));
						fprintf(fp,"add $t0, $t0, $sp\n");
						fprintf(fp,"sw $v0, 0($t0)\n");
					}
					else
						fprintf(fp,"sw $v0,%d($sp)\n",var_get(p->args[1]));
				}
				else if(strcmp(p->args[0],"WRITE")==0)
				{
					prep_reg(p->args[1],0);
					fprintf(fp,"move $a0, $t0\n");
					fprintf(fp,"addi $sp, $sp, -4\n");
					fprintf(fp,"sw $ra, 0($sp)\n");
					fprintf(fp,"jal write\n");
					fprintf(fp,"lw $ra, 0($sp)\n");
					fprintf(fp,"addi $sp, $sp, 4\n");
				}
				break;
			}
			case 3:
			{
				if(strcmp(p->args[1],":=")==0)
				{
					prep_reg(p->args[2],0);
					if(p->args[0][0]=='*')
					{
						fprintf(fp,"lw $t1, %d($sp)\n",var_get(p->args[0]));
						fprintf(fp,"add $t1, $t1, $sp\n");
						fprintf(fp,"sw $t0, 0($t1)\n");
					}
					else
					{
						fprintf(fp,"sw $t0, %d($sp)\n",var_get(p->args[0]));
					}
				}
				else if(strcmp(p->args[0],"DEC")!=0)
				{
					fprintf(fp,"%s:\n",p->args[1]);
				}
				break;
			}
			case 4:
			{
				arg_count=0;
				fprintf(fp,"addi $sp, $sp, -4\n");
				fprintf(fp,"sw $ra, 0($sp)\n");
				fprintf(fp,"jal %s\n",p->args[3]);
				fprintf(fp,"lw $ra, 0($sp)\n");
				fprintf(fp,"addi $sp, $sp, 4\n");
				if(p->args[0][0]=='*')
				{
					fprintf(fp,"lw $t0, %d($sp)\n",var_get(p->args[0]));
					fprintf(fp,"add $t0, $t0 ,$sp\n");
					fprintf(fp,"sw $v0, 0($t0)\n");
				}
				else
					fprintf(fp,"sw $v0,%d($sp)\n",var_get(p->args[0]));
				break;
			}
			case 5:
			{
				prep_reg(p->args[2],0);
				prep_reg(p->args[4],1);
				switch(p->args[3][0])
				{
					case '+':
					{
						fprintf(fp,"add $t0, $t0, $t1\n");
						break;
					}
					case '-':
					{
						fprintf(fp,"sub $t0, $t0, $t1\n");
						break;
					}
					case '*':
					{
						fprintf(fp,"mul $t0, $t0, $t1\n");
						break;
					}
					case '/':
					{
						fprintf(fp,"div $t0, $t1\n");
						fprintf(fp,"mflo $t0\n");
						break;
					}
					default:;
				}
				if(p->args[0][0]=='*')
				{
					fprintf(fp,"lw $t1, %d($sp)\n",var_get(p->args[0]));
					fprintf(fp,"add $t1, $t1 ,$sp\n");
					fprintf(fp,"sw $t0, 0($t1)\n");
				}
				else
					fprintf(fp,"sw $t0, %d($sp)\n",var_get(p->args[0]));
				break;
			}
			case 6:
			{
				char temp[4];
				if(strcmp(p->args[2],"==")==0)
					strcpy(temp,"beq");
				else if(strcmp(p->args[2],"!=")==0)
					strcpy(temp,"bne");
				else if(strcmp(p->args[2],">")==0)
					strcpy(temp,"bgt");
				else if(strcmp(p->args[2],"<")==0)
					strcpy(temp,"blt");
				else if(strcmp(p->args[2],">=")==0)
					strcpy(temp,"bge");
				else if(strcmp(p->args[2],"<=")==0)
					strcpy(temp,"ble");
				prep_reg(p->args[1],0);
				prep_reg(p->args[3],1);
				fprintf(fp,"%s $t0,$t1,%s\n",temp,p->args[5]);
				break;
			}
			default:;
		}
		p=p->next;
		//fprintf(fp,"\n");
	}
}