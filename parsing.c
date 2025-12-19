#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"

//linux __LINUX__
//mac TARGET_OS_MAC
#ifdef _WIN32
#include <string.h>

static char buffer[2048];

char* readline(char* prompt){
	fputs(prompt, stdout);
	fgets(buffer, 2048, stdin);
	char* cpy = malloc(strlen(buffer)+1);
	strcpy(cpy, buffer);
	cpy[strlen(cpy-1)] = '\0';
	return cpy;
}

void add_history(char* unused) {}

#else
#include <editline/readline.h>
#include <editline/history.h>
#endif
/*Bonus Marks函数测试前向声明*/
#if 1
int leaves(mpc_ast_t *t);


#endif

long eval_op(long x, char *op, long y) {
	if(strcmp(op, "+") == 0)
		return x + y;
	if(strcmp(op, "-") == 0){
		//if(long y )
		return x - y;
	}
	if(strcmp(op, "*") == 0)
		return x * y;
	if(strcmp(op, "/") == 0)
		return x / y;
	if(strcmp(op, "%") == 0)
		return x % y;
	if(strcmp(op, "^") == 0)
		return pow(x, y);
	if(strcmp(op, "min") == 0)
		return x < y ? x : y;
	if(strcmp(op, "max") == 0)
		return x > y ? x : y;
	return 0;
}

long eval_unary(char *op, long x) {
	if(strcmp(op, "-") == 0)
		return -x;
	if(strcmp(op, "+") == 0)
		return +x;
	return 0;
}

long eval(mpc_ast_t *t) {
	/*标记为数字则将起转换为 int 类型直接返回*/
	if(strstr(t->tag, "number")) {
		return atoi(t->contents);
	}
	/*
	 * expr : <number> | '(' <operator> <expr>+ ')';
	 * lispy: /^/ <operator> <expr>+ /$/;
	 * 非数字的表达式，符号总是在第二个孩子节点,第一个子节点始终为（
	 * */
	char *op = t->children[1]->contents;

	/*解析第一个表达式*/
	long x = eval(t->children[2]);

	if(!strstr(t->children[3]->tag, "expr"))
		return eval_unary(op, x);

	/*迭代剩余子节点组合*/
	int i = 3;
	while (strstr(t->children[i]->tag, "expr")) {
		x = eval_op(x, op, eval(t->children[i]));
		++i;
	}
	return x;
}

int main(int argc, char** argv) {
	mpc_parser_t *Number   = mpc_new("number");
	mpc_parser_t *Operator = mpc_new("operator");
	mpc_parser_t *Expr     = mpc_new("expr");
	mpc_parser_t *Lispy    = mpc_new("lispy");

	/*
	 * 一般输入为字面量
	 * //内包裹正则表达式
	 * <>内包裹规则引用
	 * */
	mpca_lang(MPCA_LANG_DEFAULT,
		  "						\
		  number : /-?[0-9]+(\\.[0-9]+)?/;		\
		  operator : '+' | '-' | '*' | '/' | '%' | '^'	\
		  | \"add\" | \"sub\" | \"mul\"	| \"div\"	\
		  | \"min\" | \"max\";						\
		  expr : <number> | '(' <operator> <expr>+ ')';	\
		  lispy	: /^/ <operator> <expr>+ /$/;		\
		  ",
		  Number, Operator, Expr, Lispy);

	//仅输出字符串，且自动添加换行符,不支持格式化
	puts("Lispy Version 0.0.0.0.2");

	puts("Press Ctrl+c to Exit\n");

	while(1) {
		//readline会调用malloc分配新的内存
		char* input = readline("lisp> ");
		add_history(input);

		//printf("No you're a %s\n", input);

		mpc_result_t r;
		if (mpc_parse("<stdin>", input, Lispy, &r)) {
		//	mpc_ast_print(r.output);
			long result = eval(r.output);
			printf("%li\n", result);
		//	int num_leaf = leaves(r.output);
		//	printf("leaves: %d\n",num_leaf);
#if 0
			//load ast from output
			mpc_ast_t* a = r.output;
			printf("Tag: %s\n", a->tag);
			printf("Contents: %s\n", a->contents);
			printf("Number of children: %i\n", a->children_num);

			/*GET First Child*/
			mpc_ast_t* c0 = a->children[0];
			printf("First Child Tag: %s\n", c0->tag);
			printf("First Child Contents: %s\n", c0->contents);
			printf("First Child Number of Children: %i\n", c0->children_num);
#endif
			mpc_ast_delete(r.output);
		} else {
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}
		free(input);

	}

	mpc_cleanup(5, Number, Operator, Expr, Lispy);
	return 0;
}
/*Bonus Marks部分附加函数*/

/*递归计算树的叶子数量*/
int leaves(mpc_ast_t *t) {
	if(t->children_num == 0)
		return 1;
	int count = 0;
	for(int i = 0; i < t->children_num; ++i) {
		count += leaves(t->children[i]);
	}
	return count;
}

/*递归计算分支数同上（有多少叶子节点便有多少分支）*/

/*
 * 递归计算一个分支延伸出的最大子节点数（即最大深度）
 * 与上计算叶子节点的不同是：
 * 在遍历解空间树时，leaves函数只需将到达的叶子节点与当前叶子数相加
 * 而depth需要保存当前深度与下一分支深度比较求最大值
 * */
int depth(mpc_ast_t *t) {
	if(t->children_num == 0)
		return 1;
	int max = 0;
	for(int i = 0; i < t->children_num; ++i) {
		int deep = depth(t->children[i]);
		if(deep > max)
			max = deep;
	}
	return max + 1;
}
