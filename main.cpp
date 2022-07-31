//BonSavage (C) 2021-2022
//Todo: long artithmetic, continuations
#include <iostream>
#include <cstring>
#include <vector>
#include <assert.h>
#include <exception>
#include <string>

//Object
enum class lisp_type {nil = 0, broken_heart, cons_cell, fixnum, double_float, bignum, real, boolean, vector, primitive, compound, continuation, symbol};

struct lisp_object
{
    lisp_type tag : 8;
    unsigned id : 24;
};

struct cons_cell
{
    lisp_object car;
    lisp_object cdr;
};

//Debug

class SimpleError : public std::exception
{
public:
    //SimpleError(std::string&& str) : msg{str} {}
    SimpleError(const char* const cstr) : msg{cstr} {}
    const char* what(void) {return msg;};
private:
    const char* msg;
};

#define typep(obj,type) (obj .tag == lisp_type:: type)
#define etypep(obj,type,msg) if (!typep(obj,type)) throw SimpleError(msg);
#define numberp(obj) typep(obj,fixnum)
lisp_object make_lisp_object(lisp_type type,unsigned id)
{
    lisp_object res;
    res.tag = type;
    res.id = id;
    return res;
}

#define make_obj(type,val) make_lisp_object(lisp_type:: type, val)

lisp_object number(unsigned value)
{
    return make_obj(fixnum,value);
}

//Registers

lisp_object val, expr, argl, proc, unev, env;
lisp_object global_environment;

//Obarray
using symbol = const char*;
std::vector<symbol> obarray;

bool same_strings(const char* str1, const char* str2)
{
    while((*str1 != '\0') || (*str2 != '\0'))
        if(*(str1++) != *(str2++))
            return false;
    return (*str1 == *str2);
}

int symbol_id(const char* name)
{

    for(size_t i = 0;i != obarray.size();++i){
        if (name == obarray[i] || same_strings(name,obarray[i]))
            return i;
    }
    return -1;
}

const char* get_symbol(unsigned id)
{
    return obarray.at(id);
}

lisp_object make_symbol(const char* name)
{
    int id = symbol_id(name);
    if(id != -1)
        return make_obj(symbol,id);

    obarray.push_back(name); //Intern unseen symbol
    return make_obj(symbol,obarray.size()-1);
}

//Memory space
using memory_adress = unsigned int;
using memory_cell = cons_cell;
using memory_adress = unsigned int;
const size_t pool_size = 16777216; //2^24 = 16777216
memory_cell first_pool[pool_size],second_pool[pool_size];
memory_cell *working_memory = first_pool;
memory_cell *free_memory = second_pool;
unsigned int working_index = 0, free_index = 0;

class out_of_memory : public std::exception
{
};

unsigned allocate_pair(void)
{
    if (working_index < pool_size) {
        //working_memory[working_index].car = nil;
        //working_memory[working_index].cdr = nil;
        return working_index++;
    } else throw out_of_memory();
}

cons_cell deref_cons(lisp_object obj)
{
    assert(typep(obj,cons_cell) || typep(obj,compound));
    return static_cast<cons_cell>(working_memory[obj.id]);
}

//Constants

const long unsigned max_num = pool_size-1;

//symbols

const lisp_object sym_nil = make_symbol("nil");
const lisp_object sym_lambda = make_symbol("lambda");
const lisp_object sym_quote = make_symbol("quote");
const lisp_object sym_set = make_symbol("set!");
const lisp_object sym_if = make_symbol("if");
const lisp_object sym_define = make_symbol("define");
const lisp_object sym_begin = make_symbol("begin");

//Stack

lisp_object stack[1000];
unsigned stack_pointer = 0;

lisp_object stack_get(unsigned offset)
{
    unsigned index = stack_pointer-offset-1;
    assert(index >= 0 && index < 1000);
    return stack[index];
}
void stack_push(lisp_object val)
{
    stack[stack_pointer++] = val;
    assert(stack_pointer < 1000);
}
lisp_object stack_pop(void)
{
    assert(stack_pointer > 0);
    return stack[--stack_pointer];
}

void stack_drop(unsigned count)
{
    stack_pointer -= count;
    assert(stack_pointer >= 0);
}

void stack_set(unsigned count)
{
    stack_pointer = 1;
}

#define push(place) stack_push(place)
#define pop(place) place=stack_pop()

//Built-ins

lisp_object nil = {lisp_type::nil,0};
lisp_object val_true = {lisp_type::boolean,1};
lisp_object val_false = {lisp_type::boolean,0};

bool eq(lisp_object o1,lisp_object o2)
{
    return o1.tag == o2.tag && o1.id == o2.id;
}

lisp_object add(lisp_object a1,lisp_object a2)
{
    if(!(numberp (a1) && numberp(a2)))
        throw SimpleError("+: args must be numbers");
    return number(a1.id+a2.id);
}

lisp_object sub(lisp_object a1,lisp_object a2)
{
    if(!(numberp (a1) && numberp(a2)))
        throw SimpleError("-: args must be numbers");
    return number(a1.id-a2.id);
}

lisp_object mul(lisp_object a1,lisp_object a2)
{
    if (!(numberp (a1) && numberp(a2)))
        throw SimpleError("*: args must be numbers");
    return number(a1.id*a2.id);
}

lisp_object div(lisp_object a1,lisp_object a2)
{
    if(!(numberp (a1) && numberp(a2)))
        throw SimpleError("/: args must be numbers");
    return number(a1.id/a2.id);
}

void collect_garbage(void);

lisp_object cons(lisp_object car,lisp_object cdr)
{
    cons_cell* ptr = nullptr;
    try {
        ptr = working_memory + allocate_pair();
    } catch (out_of_memory ex) {
        push(car);
        push(cdr);
        collect_garbage();
        pop(cdr);
        pop(car);
        ptr = working_memory + allocate_pair();
    }
    ptr->car = car;
    ptr->cdr = cdr;
    return make_obj(cons_cell, ptr-working_memory);
}

bool null(lisp_object obj)
{
    return eq(obj,nil);
}

lisp_object car(memory_adress cell)
{
    //assert(typep(pair,cons))
    return working_memory[cell].car;
}

lisp_object cdr(memory_adress cell)
{
    //assert(typep(pair,cons));
    return working_memory[cell].cdr;
}

void set_car(memory_adress cell,lisp_object val)
{
    working_memory[cell].car = val;
}

void set_cdr(memory_adress cell,lisp_object val)
{
    working_memory[cell].cdr = val;
}

unsigned length(lisp_object lst)
{
    unsigned res = 0;
    while(!null(lst)) {
        ++res;
        lst = deref_cons(lst).cdr;
    }
    return res;
}

//Memory

lisp_object gc_trace(lisp_object);

lisp_object broken_heart = make_obj(broken_heart,0);

bool broken_heartp(memory_adress cell)
{
    return typep(working_memory[cell].car,broken_heart);
}

void set_broken_heart(memory_adress cell,memory_adress new_adress)
{
    working_memory[cell].car = broken_heart;
    working_memory[cell].cdr = number(new_adress);
}

lisp_object trace_cell(const lisp_object obj) //Trace memory cell
{
    lisp_object res = obj;
    lisp_object* root = &res;
    while(!null(*root)) {
        printf("Tracing ID %lu \n",root->id);
        if(broken_heartp(root->id)) {
            printf("Found broken heart\n");
            root->id = deref_cons(*root).cdr.id;
            break;
        }

        const cons_cell old_pair = deref_cons(*root);
        const memory_adress new_adress = free_index++;
        set_broken_heart(root->id,new_adress); //Must be before tracing to avoid infinite recursion
        printf("New adress: %lu\n",deref_cons(*root).cdr.id);
        root->id = new_adress;

        lisp_object new_car = gc_trace(old_pair.car);
        free_memory[new_adress].car = new_car;
        free_memory[new_adress].cdr = old_pair.cdr;
        root = &(free_memory[new_adress].cdr);

        if(!typep((*root),cons_cell)) {
            *root = gc_trace(*root);
            break;
        }
    }
    return res;
}

lisp_object trace_linear(const lisp_object obj) //Trace list of atoms (bignum, real)
{
    lisp_object res = obj;
    lisp_object* root = &res;
    while(!null(*root)) {
        if (broken_heartp(root->id)) {
            root->id = deref_cons(*root).cdr.id;
            break;
        }

        const cons_cell old_pair = deref_cons(*root);
        const memory_adress new_adress = free_index++;
        set_broken_heart(root->id,new_adress);

        free_memory[new_adress].car = old_pair.car;
        free_memory[new_adress].cdr = old_pair.cdr;
        root = &(free_memory[new_adress].cdr);
    }
    return res;
}

lisp_object gc_trace(lisp_object obj)
{
    if(typep(obj,cons_cell) || typep(obj,compound)) {
        return trace_cell(obj);
    } else if (typep(obj,bignum) || typep(obj,real)) {
        return trace_linear(obj);
    } {return obj;}
}

void gc_trace_stack(void)
{
    for(size_t i = 0;i < stack_pointer;++i)
        stack[i] = gc_trace(stack[i]);
}

void gc_start(void)
{
    free_index = 0;
}

void gc_end(void)
{
    memory_cell* temp = working_memory;
    working_memory = free_memory;
    free_memory = temp;
    working_index = free_index;
    free_index = 0;
    printf("val: %lu, expr: %lu, env: %lu, global_env %lu",val.id,expr.id,env.id,global_environment.id);
}

void collect_garbage(void)
{
    gc_start();
    val = gc_trace(val);
    expr = gc_trace(expr);
    env = gc_trace(env);
    argl = gc_trace(argl);
    unev = gc_trace(unev);
    proc = gc_trace(proc);
    global_environment = gc_trace(global_environment); //Must be always broken-heart
    gc_trace_stack();
    gc_end();
}

//Primitives

using primitive_procedure = void(*)(unsigned);

#define assert_count(n,name) if (num != n) throw SimpleError( #name ": invalid call");

void prim_gc(unsigned num)
{
    assert_count(0,gc);
    collect_garbage();
    val = nil;
}

void prim_eq(unsigned num)
{
    assert_count(2,eq);
    val = make_obj(boolean,static_cast<unsigned>(eq(stack_get(0),stack_get(1))));
}

void prim_cons(unsigned num)
{
    assert_count(2,cons);
    val = cons(stack_get(1),stack_get(0));
}

void prim_add(unsigned num)
{
    val = number(0);
    for(int i = num-1; i >= 0; --i)
        val = add(val,stack_get(i));
}

void prim_sub(unsigned num)
{
    if(num == 0){
            val = number(0);
            return;
    } else val = stack_get(0);

    for(int i = num-1; i > 0;--i)
        val = sub(stack_get(i),val);
}

void prim_mul(unsigned num)
{
    val = number(1);
    for(int i = num-1; i >= 0;--i)
        val = mul(val,stack_get(i));
}

void prim_div(unsigned num)
{
    if(num == 0)
        throw SimpleError("Too few args for division");
    val = stack_get(num-1);
    for(int i = num-2; i >= 0;--i)
        val = div(val,stack_get(i));
}

void prim_car(unsigned num)
{
    if ((num != 1) || !typep(stack_get(0),cons_cell))
        throw SimpleError("car: one arg of type pair awaited");
    val = car(stack_get(0).id);
}

void prim_cdr(unsigned num)
{
    if ((num != 1) || !(typep(stack_get(0),cons_cell)))
        throw SimpleError("cdr: one arg of type pair awaited");
    val = cdr(stack_get(0).id);
}

void prim_null(unsigned num)
{
    if (num != 1)
        throw SimpleError("null?: invalid args count");
    val = make_obj(boolean,static_cast<unsigned>(null(stack_get(0))));
}

void prim_list(unsigned num)
{
    val = nil;
    for(unsigned i = 0;i < num;++i)
        val = cons(stack_get(i),val);
}

void prim_set_car(unsigned num)
{
    if ((num != 2) || !typep(stack_get(1),cons_cell))
        throw SimpleError("set-car: invalid call");
    set_car(stack_get(1).id,stack_get(0));
}

void prim_set_cdr(unsigned num)
{
    if ((num != 2) || !typep(stack_get(1),cons_cell))
        throw SimpleError("set-cdr: invalid call");
    set_cdr(stack_get(1).id,stack_get(0));
}

struct built_in
{
    lisp_object symbol;
    primitive_procedure adress;
};

#define prim_proc(name,adress) {make_symbol(name),adress}

built_in primitive_procedures[] =
    {prim_proc("car",prim_car),
    prim_proc("cdr",prim_cdr),
    prim_proc("cons",prim_cons),
    prim_proc("+",prim_add),
    prim_proc("-",prim_sub),
    prim_proc("*",prim_mul),
    prim_proc("/",prim_div),
    prim_proc("eq?",prim_eq),
    prim_proc("null?",prim_null),
    prim_proc("list",prim_list),
    prim_proc("gc",prim_gc),
    prim_proc("set-car!",prim_set_car),
    prim_proc("set-cdr!",prim_set_cdr)};


primitive_procedure primitive_adress(lisp_object proc)
{
    assert(typep(proc,primitive));
    return primitive_procedures[proc.id].adress;
}

//Environment

lisp_object assoc(const lisp_object sym,const lisp_object alist)
{
    lisp_object current = alist;
    while(!null(current)){
        lisp_object match = car(car(current.id).id);
        if(eq(sym,match))
            return (car(current.id));
        current = cdr(current.id);
    }
    return nil;
}

void extend_environment(lisp_object sym,lisp_object value)
{
    lisp_object slot = cons(sym,value);
    lisp_object binding = assoc(sym,car(env.id)); //Be careful with GC!!!
    if (!null(binding)) {
            set_cdr(binding.id,value);
    } else {
        lisp_object bindings = cons(slot,car(env.id));
        set_car(env.id,bindings);
    }
}

void extend_environment_list(unsigned argc)
{
    env = cons(nil,env);
    while (argc) {
        if(null(unev)) {
            throw SimpleError("Too many args given for application");
        }
        lisp_object binding = cons(car(unev.id),stack_get(--argc));
        //printf("EXTEND ENV: Tag: %lu ID %lu",car(unev.id).tag,car(unev.id).id);
        set_car(env.id,cons(binding,car(env.id)));
        unev = cdr(unev.id);
    }
    if(!null(unev))
        throw SimpleError("Too few args given for application");
}

lisp_object find_binding(lisp_object sym)
{
    lisp_object current_env = env;
    while(!null(current_env)) {
            lisp_object pair = assoc(sym,car(current_env.id));
            if (!null(pair))
                return pair;
            current_env = cdr(current_env.id);
    }
    return nil;
}

void find_var(lisp_object sym)
{
    lisp_object binding = find_binding(sym);
    if (null(binding)) {
        //printf("Value is null!\n");
        //val = nil;
        throw SimpleError("unbound variable");
    } else {val = cdr(binding.id);}
}

//Compound procedure

lisp_object proc_body(cons_cell lst)
{
    return car(lst.cdr.id);
}

lisp_object proc_env(cons_cell lst)
{
    return cdr(lst.cdr.id);
}

lisp_object proc_params(cons_cell lst)
{
    return lst.car;
}

//Application

lisp_object get_args(lisp_object lst)
{
    return cdr(lst.id);
}

lisp_object get_procedure(lisp_object lst)
{
    return car(lst.id);
}

//Conditional

lisp_object if_precond(lisp_object expr)
{
    return car(cdr(expr.id).id);
}

lisp_object if_then(lisp_object expr)
{
    return car(cdr(cdr(expr.id).id).id);
}

lisp_object if_else(lisp_object expr)
{
    return car(cdr(cdr(cdr(expr.id).id).id).id);
}

//Evaluator

void eval(void);
void eval_apply(void);
void apply_compound(unsigned);
void apply_primitive(unsigned);

void eval_lambda(void)
{
    lisp_object tail = cons(cdr(expr.id),env);
    lisp_object lst = cons(car(expr.id),tail);
    val = make_obj(compound,lst.id);
}

bool truep(lisp_object expr)
{
    return !eq(expr,val_false);
}

void eval_if(void)
{
    push(expr);
    expr = if_precond(expr);
    push(env);
    eval();
    pop(env);
    pop(expr);
    if (truep(val)) {
        expr = if_then(expr);
        eval(); //Should be goto
    } else {
        expr = if_else(expr);
        eval(); //Should be goto
    }
}

void eval_set(void)
{
    push(expr);
    expr = car(cdr(cdr(expr.id).id).id);
    push(env);
    eval();
    pop(env);
    pop(expr);
    lisp_object binding = find_binding(car(cdr(expr.id).id));
    if(!null(binding))
        set_cdr(binding.id,val);
}

void eval_defun(void)
{
    push(expr);
    expr = cons(cdr(car(cdr(expr.id).id).id),cdr(cdr(expr.id).id));
    eval_lambda();
    pop(expr);
    extend_environment(car(car(cdr(expr.id).id).id),val);
}

bool consp(lisp_object);

void eval_define(void)
{
    if(consp(car(cdr(expr.id).id))) {
        eval_defun();
        return;
    }
    push(expr);
    expr = car(cdr(cdr(expr.id).id).id);
    push(env);
    eval();
    pop(env);
    pop(expr);
    if (!(typep(car(cdr(expr.id).id),symbol)))
        throw SimpleError("Bad define form");
    extend_environment(car(cdr(expr.id).id),val);
}

void eval_block(void)
{
    push(unev);
    unev = expr;
    while(!null(cdr(unev.id))) {
        expr = car(unev.id);
        push(env);
        eval();
        pop(env);
        unev = cdr(unev.id);
    }
    expr = car(unev.id);
    pop(unev);
    eval();
}

void eval_quote(void)
{
    val = deref_cons(expr).cdr;
}

bool consp(lisp_object expr)
{
    return typep(expr,cons_cell);
}

bool lambdap(lisp_object expr)
{
    return eq(car(expr.id),sym_lambda);
}

bool ifp(lisp_object expr)
{
    return eq(car(expr.id),sym_if);
}

bool setp(lisp_object expr)
{
    return eq(car(expr.id),sym_set);
}

bool definep(lisp_object expr)
{
    return eq(car(expr.id),sym_define);
}

bool quotep(lisp_object expr)
{
    return eq(car(expr.id),sym_quote);
}

bool variablep(lisp_object expr)
{
    return typep(expr,symbol);
}

bool blockp(lisp_object expr)
{
    return eq(car(expr.id),sym_begin);
}

void eval(void) //Mutates val register
{
    if(consp(expr)) {
            if(lambdap(expr)) {
                expr = cdr(expr.id);
                eval_lambda();
            } else if (ifp(expr)) {
                eval_if();
            } else if (setp(expr)) {
                eval_set();
            } else if (definep(expr)) {
                eval_define();
            } else if (quotep(expr)) {
                eval_quote();
             } else if(blockp(expr)){
                expr = cdr(expr.id);
                eval_block();
            } else {
                eval_apply();
            }
    } else if (variablep(expr)) {
        find_var(expr);
    } else { //self-evaluating
        val = expr;
    }
}

void eval_apply(void)
{
    printf("Apply: Stack size is %u\n",stack_pointer);
    push(unev); //REMEMBER ABOUT IT
    unev = get_args(expr);
    unsigned argc = 0;
    expr = get_procedure(expr);
    eval();
    proc = val;
    while (!null(unev)) {
        expr = deref_cons(unev).car;
        push(env);
        push(proc);
        eval();
        pop(proc);
        pop(env);
        push(val);
        ++argc;
        unev = deref_cons(unev).cdr;
    }

    if (!(typep(proc,compound) || typep(proc,primitive)))
        throw SimpleError("Cannot find procedure for application");
    if (typep(proc,compound)){
        apply_compound(argc);
    } else if (typep(proc,primitive)){
        apply_primitive(argc);
    }
    //pop(unev) - Must be NOT here for tail call optimization!!!
}

void apply_compound(unsigned argc)
{
    env = proc_env(deref_cons(proc));
    unev = proc_params(deref_cons(proc));
    extend_environment_list(argc);
    stack_drop(argc);
    pop(unev);
    expr = proc_body(deref_cons(proc));
    eval_block(); //Must be goto instead!
}

void print(void);

void apply_primitive(unsigned argc)
{

    primitive_adress(proc)(argc);
    stack_drop(argc);
    pop(unev);
}

void add_var(const char* const name,lisp_object val)
{
    lisp_object temp = cons(make_symbol(name),val);
    lisp_object bindings = cons(temp,car(global_environment.id));
    set_car(global_environment.id,bindings);
}

void init_global_env(void)
{
    global_environment = cons(nil,nil);
    add_var("nil",nil);
    for(size_t i = 0;i != (sizeof(primitive_procedures)/sizeof(built_in));++i) {
        lisp_object temp = cons(primitive_procedures[i].symbol,make_obj(primitive,i));
        printf("Adding symbol to global env: %u \n",primitive_procedures[i].symbol.id);
        lisp_object bindings = cons(temp,car(global_environment.id));
        set_car(global_environment.id,bindings);
    }
    env = global_environment;
}

//Reader
/*
    Expression = Atom | SExp
    SExp = Expression * Expression
    Atom = Number | Symbol
*/

char read_char;

void pass_space(void)
{
    while(isspace(read_char))
        read_char = getchar();
}

void read_number(void)
{
    unsigned num = 0;
    while(isdigit(read_char)) {
        num = num*10 + (read_char - '0');
        read_char = getchar();
    }
    val = number(num);
}

void read_symbol(void)
{
    char* name = new char[64];
    unsigned len = 0;
    while(read_char != '(' && read_char != ')' && !isspace(read_char) && read_char != EOF && len < 63)  {
        name[len++] = read_char;
        read_char = getchar();
    }
    name[len] = '\0';

    int id = symbol_id(name);

    if(symbol_id(name) != -1) {
        val = make_obj(symbol,id);
    } else {
        char* new_location = new char[len+1];
        strcpy(new_location,name);
        val = make_symbol(new_location);
    }
    delete name;
}

void read_expr(void);

void read_sexpr(void)
{
    pass_space();
    if(read_char == ')') {
        val = nil;
        read_char = getchar();
    } else {
        read_expr(); //First val
        push(val); //Save first val
        read_sexpr(); //Second val is changed
        pop(expr); //Restore first val. Now expr is first_val, val is second_val
        val = cons(expr,val); //Pair of first val and second val
    }
}

void read_expr(void)
{
    pass_space();

    if (read_char == '(') {
        read_char = getchar();
        read_sexpr();
    }  else if (isdigit(read_char)) {
        read_number();
    } else if (read_char == '\'') {
        read_char = getchar();
        read_expr();
        val = cons(sym_quote,val);
    } else {
        read_symbol();
    }
}

void read(void)
{
    read_char = getchar();
    read_expr();
    expr = val;
    val = nil;
}

void print_obj(lisp_object);

void print_cons(lisp_object pair)
{
    putchar('(');
    while(typep(pair,cons_cell)) {
        print_obj(car(pair.id));
        if (typep(cdr(pair.id),cons_cell))
             putchar(' ');
        pair = cdr(pair.id);
    }
    if (!null(pair)) {
        printf(" . ");
        print_obj(pair);
    }

    putchar(')');
}

void print_obj(lisp_object val)
{
    if(typep(val,fixnum)){
        printf("%d",val.id);
    } else if (typep(val,cons_cell)) {
        print_cons(val);
    } else if (typep(val,symbol)) {
            printf("%s",get_symbol(val.id));
    } else if (typep(val,boolean)) {
            if(val.id) {
                printf("#t");
            } else printf("#f");
    } else {
        printf("Type: %lu, ID: %lu",val.tag,val.id);
    }
}

void print(void)
{
    print_obj(val);
    putchar('\n');
}

int main()
{
    init_global_env();
    while(true) {
        printf("LISP REPL>");
        read();
        push(env);
        try{
        eval();
        } catch(SimpleError(tr)) {
            printf("Runtime error: %s\n",tr.what());
            stack_set(1);
            pop(env);
            continue;
        }
        pop(env);
        print();
    }
    return 0;
}
