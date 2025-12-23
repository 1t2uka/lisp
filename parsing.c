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
	LVAL_NUM_D,
	LVAL_ERR,
	LVAL_SYM,
	LVAL_SEXPR
} lval_type_t;

/*
 * lisp value,即lisp内部列表结构
 * union内部为数字，符号，或者错误时错误描述字符串
 * 使用完整的标签名（使用匿名标签会导致自引用结构体编译时无法确认类型）
 * 使用二级指针存储lval* 类型的数组（使用实体会导致嵌套声明内存溢出）
 * 二级指针允许动态调整指针数组大小（realloc)
 * */
typedef struct lval {
	union {
		double num_d;
		long num;
		char *err;
		char *sym;
	} u;
	lval_type_t type;
	int count;
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
lval* lval_num_d(double x)
{
	lval *v = malloc(sizeof(lval));
	v->type = LVAL_NUM_D;
	v->u.num_d = x;
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
	case LVAL_NUM_D: break;

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

/*读取整数,并使用该值创立lval类型数据*/
lval* lval_read_num(mpc_ast_t *t)
{
	errno = 0;
	long x = strtol(t->contents, NULL, 10);
	if(errno != ERANGE)
		return lval_num(x);
	return lval_err("invalid number");
}

/*读取浮点数,同上*/
lval* lval_read_double(mpc_ast_t *t)
{
	errno = 0;
	double x = strtod(t->contents, NULL);
	if(errno != ERANGE)
		return lval_num_d(x);
	return lval_err("invalid double");
}

/*添加元素*/
lval* lval_add(lval *v, lval *x)
{
	v->count++;
	v->cell = realloc(v->cell, sizeof(lval*) * v->count);
	v->cell[v->count - 1] = x;
	return v;
}

/*读取抽象语法树ast转化为s表达式*/
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
	case LVAL_NUM_D: printf("%f",v->u.num_d);
		       break;
	case LVAL_ERR: printf("ERROR: %s", v->u.err);
		       break;
	case LVAL_SYM: printf("%s", v->u.sym);
		       break;
	case LVAL_SEXPR: lval_expr_print(v, '(', ')');
			 break;
	}
}

/*s表达式打印*/
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
 * 用[0-i]的内容覆盖[0-i+1]
 * 将计数索引count减1
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
 * 调用pop函数覆盖原数据
 * 调用del释放原先堆
 * */
lval* lval_take(lval *v, int i)
{
	lval *x = lval_pop(v, i);
	lval_del(v);
	return x;
}


lval* builtin_op(lval *a, char *op);
lval* lval_eval(lval *v);
/*
 * 解析s表达式
 * 首先评估所有子表达式
 * 有任何错误子表达式调用lval_take返回第一个错误
 * 没有子项直接返回空结果（）
 * 正确的表达式，使用lval_pop分离第一各元素，并传递给builtin_op计算
 * 第一个元素必须为sym类型
 * */
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

/*将sexpr与其他类型分离*/
lval* lval_eval(lval *v)
{
	if(v->type == LVAL_SEXPR)
		return lval_eval_sexpr(v);
	return v;
}

/*操作符函数指针*/
typedef long (*op_func_t)(long, long);
typedef double (*op_func_double_t)(double, double);

/*整数操作符函数*/
static long op_add(long a, long b) {return a + b;}
static long op_sub(long a, long b) {return a - b;}
static long op_mul(long a, long b) {return a * b;}
static long op_div(long a, long b) {return a / b;}
static long op_mod(long a, long b) {return a % b;}
static long op_pow_long(long a, long b) {return (long)pow(a, b);}
static long op_max(long a, long b) {return a > b ? a : b;}
static long op_min(long a, long b) {return a < b ? a : b;}

/*浮点操作符函数*/
static double op_add_d(double a, double b) {return a + b;}
static double op_sub_d(double a, double b) {return a - b;}
static double op_mul_d(double a, double b) {return a * b;}
static double op_div_d(double a, double b) {return a / b;}
static double op_max_d(double a, double b) {return a > b ? a : b;}
static double op_min_d(double a, double b) {return a < b ? a : b;}

/*操作符表项*/
typedef struct {
	const char *name; //操作符名称
	op_func_t func;		//整数操作函数指针
	op_func_double_t func_d;	//浮点数操作函数指针
	int needs_check;		//是否需要检查
} op_entry_t;

/*操作符映射表*/
const op_entry_t op_table[] = {
	{"+",	op_add,		op_add_d,	0},
	{"-",	op_sub,		op_sub_d,	0},
	{"*",	op_mul,		op_mul_d,	0},
	{"/",	op_div,		op_div_d,	1},
	{"%",	op_mod,		NULL,		0},
	{"^",	op_pow_long,	NULL,		0},
	{"add",	op_add,		op_add_d,	0},
	{"sub",	op_sub,		op_sub_d,	0},
	{"mul",	op_mul,		op_mul_d,	0},
	{"div",	op_div,		op_div_d,	1},
	{"min",	op_min,		op_min_d,	0},
	{"max",	op_max,		op_max_d,	0},
	{NULL,	NULL,		NULL,		0}//终止符

};

/*查找操作符函数*/
op_entry_t* find_op(const char *op_name)
{
	for(int i = 0; op_table[i].name != NULL; ++i) {
		if(strcmp(op_table[i].name, op_name) == 0)
			return (op_entry_t *)&op_table[i];
	}
	return NULL;
}

lval* builtin_op(lval *a, char *op)
{
	int has_double = 0;
	for(int i = 0; i < a->count; ++i) {
		int is_num = a->cell[i]->type == LVAL_NUM;
		int is_dou = a->cell[i]->type == LVAL_NUM_D;
		if(!is_num && !is_dou) {
			lval_del(a);
			return lval_err("Cannot operatot on non-number");
		}
		if(a->cell[i]->type == LVAL_NUM_D)
			has_double = 1;
	}

	op_entry_t *op_entry = find_op(op);
	if(!op_entry) {
		lval_del(a);
		return lval_err("Unknown operator");
	}

	if(!has_double) {
		lval *x = lval_pop(a, 0);

		if(strcmp(op, "-") == 0 && a->count == 0) {
			x->u.num = -x->u.num;
			lval_del(a);
			return x;
		}

		while (a->count > 0) {
			lval *y = lval_pop(a, 0);
			int check = op_entry->needs_check;
			int is_op_div = strcmp(op, "/") == 0;
			int is_op_div_s = strcmp(op, "div") == 0;
			int is_num_0 = y->u.num == 0;
			if(check && (is_op_div_s || is_op_div) && is_num_0) {
				lval_del(x);
				lval_del(y);
				lval_del(a);
				return lval_err("Division by zero");
			}

			x->u.num = op_entry->func(x->u.num, y->u.num);
			lval_del(y);
		}
		lval_del(a);
		return x;
	}

	lval *x = lval_pop(a, 0);
	double dx;
	if(x->type == LVAL_NUM_D)
		dx = x->u.num_d;
	else
		dx = (double)x->u.num;

	if(strcmp(op, "-") == 0 && a->count == 0) {
		dx = -dx;
		lval_del(a);
		lval_del(x);
		return lval_num_d(dx);
	}

	if(!op_entry->func_d) {
		lval_del(x);
		lval_del(a);
		return lval_err("Operator not supported for doubles");
	}

	while(a->count > 0) {
		lval *y = lval_pop(a, 0);
		double dy = y->type == LVAL_NUM_D ? y->u.num_d : (double)y->u.num;
		int check = op_entry->needs_check;
		int is_op_div = strcmp(op, "/") == 0;
		int is_op_div_s = strcmp(op, "div") == 0;
		if(check && (is_op_div || is_op_div_s) && dy == 0.0) {
			lval_del(x);
			lval_del(y);
			lval_del(a);
			return lval_err("Division by zero");
		}
		dx = op_entry->func_d(dx, dy);
		lval_del(y);
	}
	lval_del(a);
	lval_del(x);
	return lval_num_d(dx);
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
	puts("Lispy Version 0.0.1");

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
