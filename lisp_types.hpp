#ifndef LISP_TYPES_HPP_INCLUDED
#define LISP_TYPES_HPP_INCLUDED

#include <exception>

//Object
enum class lisp_type {nil = 0, broken_heart, cons_cell, lisp_string, character, lisp_vector, fixnum, double_float, bignum, real, boolean, vector, primitive, compound, continuation, symbol};

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

#define typep(obj,type) (obj .tag == lisp_type:: type)
#define etypep(obj,type,msg) if (!typep(obj,type)) throw SimpleError(msg);
#define numberp(obj) typep(obj,fixnum)

//Common definitions

using byte_t = unsigned char;

//Memory

using memory_cell = cons_cell;

class out_of_memory : public std::exception
{
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


#endif // LISP_TYPES_HPP_INCLUDED
