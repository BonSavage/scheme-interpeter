//BonSavage (C) 2021-2023
//Todo: long artithmetic, continuations
#include <iostream>
#include <cstring>
#include <vector>
#include <assert.h>
#include <exception>
#include <string>
#include "lisp.hpp"

//Reader
/*
    Expression = Atom | SExp
    SExp = Expression * Expression
    Atom = Number | Symbol | String
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

void read_string(void)
{
    unsigned len = 0;
    char* str = new char[128];
    while((read_char = getchar()) != '"' && len != 128) {
        str[len++] = read_char;
    }
    read_char = getchar();
    val = make_string(len,str);
    delete[] str;
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
    delete[] name;
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
    } else if (read_char == '"') {
        read_string();
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
        int flag = val.id&fixnum_negative_flag;
        int value = (val.id^(-fixnum_negative_flag))*(flag ? -1 : 1);
        printf("%d",val.id);
    } else if (typep(val,cons_cell)) {
        print_cons(val);
    } else if (typep(val,symbol)) {
            printf("%s",get_symbol(val.id));
    } else if (typep(val,boolean)) {
            if(val.id) {
                printf("#t");
            } else printf("#f");
    } else if(typep(val,character)) {
        printf("#\\%c",(unsigned char)val.id);
    } else if(typep(val,lisp_string)) {
        char* str = reinterpret_cast<char*>(deref_string(val));
        putchar('"');
        for(size_t i = 0;i < vector_length(val);++i) {
            putchar(str[i]);
        }
        putchar('"');
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
