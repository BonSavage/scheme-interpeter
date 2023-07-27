#ifndef LISP_HPP_INCLUDED
#define LISP_HPP_INCLUDED

#include <vector>
#include "lisp_types.hpp"
#include "memory.hpp"

lisp_object make_lisp_object(lisp_type type,unsigned id);

#define make_obj(type,val) make_lisp_object(lisp_type:: type, val)

lisp_object number(unsigned);

//Registers

extern lisp_object val, expr, argl, proc, unev, env;
extern lisp_object global_environment;

//Obarray
using symbol = const char*;

bool same_strings(const char* str1, const char* str2);

int symbol_id(const char* name);

const char* get_symbol(unsigned id);

lisp_object make_symbol(const char* name);

//Constants

extern const long unsigned max_num;

//symbols

extern const lisp_object sym_nil;
extern const lisp_object sym_lambda;
extern const lisp_object sym_quote;
extern const lisp_object sym_set;
extern const lisp_object sym_if;
extern const lisp_object sym_define;
extern const lisp_object sym_begin;

extern const unsigned fixnum_negative_flag;

//Stack

lisp_object stack_get(unsigned offset);

void stack_push(lisp_object val);

lisp_object stack_pop(void);

void stack_drop(unsigned count);

void stack_set(unsigned count);

#define push(place) stack_push(place)
#define pop(place) place=stack_pop()

//Built-ins

extern lisp_object nil;
extern lisp_object val_true;
extern lisp_object val_false;

bool eq(lisp_object o1,lisp_object o2);

lisp_object add(lisp_object a1,lisp_object a2);

lisp_object sub(lisp_object a1,lisp_object a2);

lisp_object mul(lisp_object a1,lisp_object a2);

lisp_object div(lisp_object a1,lisp_object a2);

void collect_garbage(void);

lisp_object cons(lisp_object car,lisp_object cdr);

bool null(lisp_object obj);

lisp_object car(memory_adress cell);

lisp_object cdr(memory_adress cell);

void set_car(memory_adress cell,lisp_object val);

void set_cdr(memory_adress cell,lisp_object val);

unsigned length(lisp_object lst);

//Memory

void collect_garbage(void);
//Primitives

using primitive_procedure = void(*)(unsigned);

void prim_gc(unsigned num);

void prim_eq(unsigned num);

void prim_cons(unsigned num);

void prim_add(unsigned num);

void prim_sub(unsigned num);

void prim_mul(unsigned num);

void prim_div(unsigned num);

void prim_car(unsigned num);

void prim_cdr(unsigned num);

void prim_null(unsigned num);

void prim_list(unsigned num);

void prim_set_car(unsigned num);

void prim_set_cdr(unsigned num);

struct built_in
{
    lisp_object symbol;
    primitive_procedure adress;
};

primitive_procedure primitive_adress(lisp_object proc);

//Environment

lisp_object assoc(const lisp_object sym,const lisp_object alist);

void extend_environment(lisp_object sym,lisp_object value);

void find_var(lisp_object sym);

//Evaluator

bool variablep(lisp_object);

void eval(void);

void print(void);

//Env

void init_global_env(void);

#endif // LISP_HPP_INCLUDED
