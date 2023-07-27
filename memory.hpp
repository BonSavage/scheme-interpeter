#ifndef MEMORY_HPP_INCLUDED
#define MEMORY_HPP_INCLUDED
#include "lisp_types.hpp"

//Memory space
using memory_adress = unsigned int;
using memory_adress = unsigned int;

unsigned allocate_pair(void);
unsigned allocate_byte_vector(unsigned);

cons_cell deref_cons(lisp_object);
byte_t* deref_string(lisp_object);

lisp_object make_string(const unsigned,const char* const);

unsigned vector_length(lisp_object);

#endif // MEMORY_HPP_INCLUDED
