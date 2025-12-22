#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
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

/*枚举可能的lval类型*/
typedef enum {
	LVAL_NUM,
	LVAL_DOU,
	LVAL_ERR
} lval_type_t;

/* 创建可能的错误类型枚举
 * @1除0
 * @2未知运算符
 * @3数值过大
 * @4浮点数过大
 * */
typedef enum {
	LVAL_ERR_DIV_ZERO,
	LVAL_ERR_BAD_OP,
	LVAL_ERR_BAD_NUM,
	LVAL_ERR_FLOAT_BAD_OP
} lval_err_t;

/*lisp值，区分值与错误*/
typedef struct {
	union {
		double dou;
		long num;
		lval_err_t err;
	} u;
	lval_type_t type;
} lval;


/*创建新的number类型(整型long)*/
lval lval_num(long x)
{
	lval v;
	v.type = LVAL_NUM;
	v.u.num = x;
	return v;
}

/*创建新的number类型（浮点型double）*/
lval lval_dou(double x)
{
	lval v;
	v.type = LVAL_DOU;
	v.u.dou = x;
	return v;
}

/*创建新的error类型lval数据*/
lval lval_err(int x)
{
	lval v;
	v.type = LVAL_ERR;
	v.u.err = x;
	return v;
}

/*打印lval类型数据*/
void lval_print(lval v)
{
	switch(v.type) {
	case LVAL_NUM: printf("%li", v.u.num);
		       break;
	case LVAL_DOU: printf("%f",v.u.dou);
		       break;
	case LVAL_ERR:
		       if(v.u.err == LVAL_ERR_DIV_ZERO)
			       printf("Error: Divsion by zero!");
		       if(v.u.err == LVAL_ERR_BAD_OP)
			       printf("Error: Invalid operator!");
		       if(v.u.err == LVAL_ERR_BAD_NUM)
			       printf("Error: Invalid Number!");
		       break;
	}
}

/*println打印*/
void lval_println(lval v)
{
	lval_print(v);
	putchar('\n');
}

lval eval_op(lval x, char *op, lval y)
{
	if(x.type == LVAL_ERR) return x;
	if(y.type == LVAL_ERR) return y;

	double x_val;
	double y_val;
	int is_double = 0;
	if(x.type == LVAL_DOU || y.type == LVAL_DOU) {
		is_double = 1;
		if(x.type == LVAL_DOU) {
			x_val = x.u.dou;
		} else
			x_val = (double)x.u.num;
		if(y.type == LVAL_DOU) {
			y_val = y.u.dou;
		} else
			y_val = (double)y.u.num;

	}
	if(!is_double) {
		if(strcmp(op, "+") == 0)
			return lval_num(x.u.num + y.u.num);
		if(strcmp(op, "-") == 0)
			return lval_num(x.u.num - y.u.num);
		if(strcmp(op, "*") == 0)
			return lval_num(x.u.num * y.u.num);
		if(strcmp(op, "/") == 0) {
			if(y.u.num == 0)
				return lval_err(LVAL_ERR_DIV_ZERO);
			return lval_num(x.u.num / y.u.num);
		}
		if(strcmp(op, "%") == 0)
			return lval_num(x.u.num % y.u.num);
		if(strcmp(op, "^") == 0)
			return lval_num(x.u.num ^ y.u.num);
		if(strcmp(op, "min") == 0)
			return lval_num(x.u.num < y.u.num ? x.u.num : y.u.num);
		if(strcmp(op, "max") == 0)
			return lval_num(x.u.num > y.u.num ? x.u.num : y.u.num);
	} else {
		if(strcmp(op, "+") == 0)
			return lval_dou(x_val + y_val);
		if(strcmp(op, "-") == 0)
			return lval_dou(x_val - y_val);
		if(strcmp(op, "*") == 0)
			return lval_dou(x_val * y_val);
		if(strcmp(op, "/") == 0) {
			if(y_val == 0)
				return lval_err(LVAL_ERR_DIV_ZERO);
			return lval_dou(x_val / y_val);
		}
		if(strcmp(op, "%") == 0)
			return lval_err(LVAL_ERR_FLOAT_BAD_OP);
		if(strcmp(op, "^") == 0)
			return lval_err(LVAL_ERR_FLOAT_BAD_OP);
		if(strcmp(op, "min") == 0)
			return lval_dou(x_val < y_val ? x.u.num : y_val);
		if(strcmp(op, "max") == 0)
			return lval_dou(x_val > y_val ? x.u.num : y_val);

	}
	return lval_err(LVAL_ERR_BAD_OP);
}

lval eval_unary(char *op, lval x)
{
	if(strcmp(op, "-") == 0)
		return lval_num(-x.u.num);
	if(strcmp(op, "+") == 0)
		return lval_num(-x.u.num);
	return lval_err(LVAL_ERR_BAD_OP);
}

lval eval(mpc_ast_t *t)
{
	/*标记为数字则将起转换为 int 类型直接返回*/
	if(strstr(t->tag, "number")) {
		errno = 0;
		long x_long = strtol(t->contents, NULL, 10);
		if(errno == ERANGE)
			return lval_err(LVAL_ERR_BAD_NUM);
		//浮点数判断
		if(strchr(t->contents, '.') != NULL) {
			double x_double = strtod(t->contents, NULL);
			return lval_dou(x_double);
		}
		return lval_num(x_long);

	}
	/*
	 * expr : <number> | '(' <operator> <expr>+ ')';
	 * lispy: /^/ <operator> <expr>+ /$/;
	 * 非数字的表达式，符号总是在第二个孩子节点,第一个子节点始终为（
	 * */
	char *op = t->children[1]->contents;

	/*解析第一个表达式*/
	lval x = eval(t->children[2]);

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

int main(int argc, char** argv)
{
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
			lval result = eval(r.output);
			lval_println(result);
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
int leaves(mpc_ast_t *t)
{
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
int depth(mpc_ast_t *t)
{
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
