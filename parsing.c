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
	LVAL_ERR,
	LVAL_SYM,
	LVAL_SEXPR
} lval_type_t;

/*lisp值，区分值与错误*/
typedef struct lval {
	union {
		double dou;
		long num;
		//lval_err_t err;
		char *err;
		char *sym;
	} u;
	lval_type_t type;
	int count;
	/* 自身引用需添加struct字段作为前向声明
	 * 自引用struct结构时只能引用指针类型以确定大小
	 * 双指针为了存储lval *列表（数组）
	 */
	struct lval **cell;
} lval;


/*number类型(整型long)构造函数*/
lval* lval_num(long x)
{
	lval *v = malloc(sizeof(lval));
	v->type = LVAL_NUM;
	v->u.num = x;
	return v;
}

/*number类型（浮点型double）构造函数*/
lval* lval_dou(double x)
{
	lval *v = malloc(sizeof(lval));
	v->type = LVAL_DOU;
	v->u.dou = x;
	return v;
}

/*error类型构造函数*/
lval* lval_err(char *m)
{
	lval *v = malloc(sizeof(lval));
	v->type = LVAL_ERR;
	v->u.err = malloc(strlen(m) + 1);
	strcpy(v->u.err,m);
	return v;
}

/*sym类型(符号类型)构造函数*/
lval* lval_sym(char *s)
{
	lval *v = malloc(sizeof(lval));
	v->type = LVAL_SYM;
	v->u.sym = malloc(strlen(s) + 1);
	strcpy(v->u.sym, s);
	return v;
}

/*sexpr s-表达式构造函数*/
lval* lval_sexpr(void)
{
	lval *v = malloc(sizeof(lval));
	v->type = LVAL_SEXPR;
	v->count = 0;
	v->cell = NULL;
	return v;
}

/*析构函数*/
void lval_del(lval *v)
{
	switch (v->type) {
	case LVAL_NUM: break;
	case LVAL_DOU: break;

	case LVAL_ERR: free(v->u.err); break;
	case LVAL_SYM: free(v->u.sym); break;

	case LVAL_SEXPR:
		       for(int i = 0; i < v->count; ++i) {
			       lval_del(v->cell[i]);
		       }
		       free(v->cell);
		       break;
	}
	free(v);
}

/*读取整数*/
lval* lval_read_num(mpc_ast_t *t)
{
	errno = 0;
	long x = strtol(t->contents, NULL, 10);
	if(errno != ERANGE)
		return lval_num(x);
	return lval_err("invalid number");
}

/*读取浮点数*/
lval* lval_read_double(mpc_ast_t *t)
{
	errno = 0;
	double x = strtod(t->contents, NULL);
	if(errno != ERANGE)
		return lval_dou(x);
	return lval_err("invalid double");
}

lval* lval_add(lval *v, lval *x)
{
	v->count++;
	v->cell = realloc(v->cell, sizeof(lval*) *v->count);
	v->cell[v->count - 1] = x;
	return v;
}

lval* lval_read(mpc_ast_t *t)
{
	if(strstr(t->tag, "number")){
		if(strstr(t->contents, "."))
			return lval_read_double(t);
		else
			return lval_read_num(t);
	}
	if(strstr(t->tag, "symbol"))
		return lval_sym(t->contents);

	lval *x = NULL;
	if(strcmp(t->tag, ">") == 0)
		x = lval_sexpr();
	if(strstr(t->tag, "sexpr"))
		x = lval_sexpr();

	for(int i = 0; i < t->children_num; ++i) {
		if(strcmp(t->children[i]->contents, "(") == 0)
			continue;
		if(strcmp(t->children[i]->contents, ")") == 0)
			continue;
		if(strcmp(t->children[i]->tag, "regex") == 0)
			continue;
		x = lval_add(x, lval_read(t->children[i]));
	}
	return x;
}

void lval_expr_print(lval *v, char open, char close);
/*打印lval类型数据*/
void lval_print(lval *v)
{
	switch(v->type) {
	case LVAL_NUM: printf("%li", v->u.num);
		       break;
	case LVAL_DOU: printf("%f",v->u.dou);
		       break;
	case LVAL_ERR: printf("ERROR: %s", v->u.err);
		       break;
	case LVAL_SYM: printf("%s", v->u.sym);
		       break;
	case LVAL_SEXPR: lval_expr_print(v, '(', ')');
			 break;
	}
}

void lval_expr_print(lval *v, char open, char close)
{
	putchar(open);
	for(int i = 0; i < v->count; ++i) {
		lval_print(v->cell[i]);

		if(i != (v->count - 1))
			putchar(' ');
	}
	putchar(close);
}

/*println打印*/
void lval_println(lval *v)
{
	lval_print(v);
	putchar('\n');
}

/*
 * 弹出索引i处元素，并使其余部分后移
 * */
lval* lval_pop(lval *v, int i)
{
	lval *x = v->cell[i];

	memmove(&v->cell[i], &v->cell[i + 1],
		sizeof(lval *) * (v->count - i - 1));
	v->count--;
	v->cell = realloc(v->cell, sizeof(lval*) * (v->count));
	return x;
}

/*
 * 删除索引为i的元素
 * */
lval* lval_take(lval *v, int i)
{
	lval *x = lval_pop(v, i);
	lval_del(v);
	return x;
}

lval* builtin_op(lval *a, char *op);
lval* lval_eval(lval *v);
lval* lval_eval_sexpr(lval *v)
{
	for(int i = 0; i < v->count; ++i) {
		v->cell[i] = lval_eval(v->cell[i]);
	}

	for(int i = 0; i < v->count; ++i) {
		if(v->cell[i]->type == LVAL_ERR)
			return lval_take(v, i);
	}

	if(v->count == 0)
		return v;
	if(v->count == 1)
		return lval_take(v, 0);

	lval *f = lval_pop(v, 0);
	if(f->type != LVAL_SYM) {
		lval_del(f);
		lval_del(v);
		return lval_err("S-expression Does not start with symbol!");
	}

	lval *result = builtin_op(v, f->u.sym);
	lval_del(f);
	return result;
}

lval* lval_eval(lval *v)
{
	if(v->type == LVAL_SEXPR)
		return lval_eval_sexpr(v);
	return v;
}

int main(int argc, char** argv)
{
	mpc_parser_t *Number   = mpc_new("number");
	mpc_parser_t *Symbol   = mpc_new("symbol");
	mpc_parser_t *Sexpr    = mpc_new("sexpr");
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
		  symbol : '+' | '-' | '*' | '/' | '%' | '^'	\
		  | \"add\" | \"sub\" | \"mul\"	| \"div\"	\
		  | \"min\" | \"max\";				\
		  sexpr: '(' <expr>* ')';			\
		  expr : <number> | <symbol> | <sexpr>;		\
		  lispy	: /^/ <expr>* /$/;			\
		  ",
		  Number, Symbol, Sexpr, Expr, Lispy);

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
			//lval result = eval(r.output);
			//lval_println(result);
			//mpc_ast_delete(r.output);
			lval *x = lval_eval(lval_read(r.output));
			lval_println(x);
			lval_del(x);
		} else {
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}
		free(input);

	}

	mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Lispy);
	return 0;
}
/*
 * 部分较长的函数
 * */

/*运算逻辑处理*/
lval* builtin_op(lval *a, char *op)
{
	int has_double = 0;
	for(int i = 0; i < a->count; ++i) {
		if(a->cell[i]->type != LVAL_NUM && a->cell[i]->type != LVAL_DOU) {
			lval_del(a);
			return lval_err("Cannot operator on non-number");
		}
		if(a->cell[i]->type == LVAL_DOU)
			has_double = 1;
	}

	if(!has_double){
		/*弹出第一个元素*/
		lval *x = lval_pop(a, 0);

		if(strcmp(op, "-") == 0 && a->count == 0)
			x->u.num = -x->u.num;

		while(a->count > 0) {

			lval *y = lval_pop(a, 0);

			if(strcmp(op, "+") == 0 || strcmp(op, "add") == 0)
				x->u.num += y->u.num;
			if(strcmp(op, "-") == 0 || strcmp(op, "sub") == 0)
				x->u.num -= y->u.num;
			if(strcmp(op, "*") == 0 || strcmp(op, "mul") == 0)
				x->u.num *= y->u.num;
			if(strcmp(op, "^") == 0)
				x->u.num = pow(x->u.num, y->u.num) ;
			if(strcmp(op, "%") == 0)
				x->u.num %= y->u.num;
			if(strcmp(op, "max") == 0) {
				if(x->u.num < y->u.num)
					x->u.num = y->u.num;
			}
			if(strcmp(op, "min") == 0) {
				if(x->u.num > y->u.num)
					x->u.num = y->u.num;
			}
			if(strcmp(op, "/") == 0 || strcmp(op, "div")) {
				if(y->u.num == 0){
					lval_del(x);
					lval_del(y);
					x = lval_err("Division by zero");
					break;
				}
				x->u.num /= y->u.num;
			}

			lval_del(y);
		}
		lval_del(a);
		return x;
	}
	lval *x = lval_pop(a, 0);
	double dx;
	if(x->type == LVAL_DOU)
		dx = x->u.dou;
	else
		dx = (double)x->u.num;
	if((strcmp(op, "-") == 0 || strcmp(op, "sub") == 0) && a->count == 0)
		dx = -dx;

	while(a->count > 0) {
		lval *y = lval_pop(a, 0);
		double dy;
		if(y->type == LVAL_DOU)
			dy = y->u.dou;
		else
			dy = (double)y->u.num;
		if(strcmp(op, "+") == 0 || strcmp(op, "add") == 0)
			dx += dy;
		if(strcmp(op, "-") == 0 || strcmp(op, "sub") == 0)
			dx -= dy;
		if(strcmp(op, "*") == 0 || strcmp(op, "mul") == 0)
			dx *= dy;
		if(strcmp(op, "max") == 0)
			dx = (dx > dy) ? dx : dy;
		if(strcmp(op, "min") == 0)
			dx = (dx < dy) ? dx : dy;
		if(strcmp(op, "/") == 0 || strcmp(op, "div") == 0) {
			if(dy == 0.0){
				lval_del(x);
				lval_del(y);
				lval_del(a);
				return lval_err("Divsion by zero");
			}
			dx /= dy;
		}
		if(strcmp(op, "%") == 0 || strcmp(op, "^") == 0){
			lval_del(x);
			lval_del(y);
			lval_del(a);
			return lval_err("Error operator");
		}
	}
	lval_del(a);
	lval_del(x);
	return lval_dou(dx);
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
