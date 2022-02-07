#ifndef EQREL2FUNC_HPP 
#define EQREL2FUNC_HPP
#include <vector>
#include "misc.hpp"
#include "base/abc/abc.h"
Abc_Ntk_t* rel2FuncEQ(Abc_Ntk_t*, std::vector<char*>&, std::vector<Abc_Ntk_t*>&);
Abc_Ntk_t* rel2FuncEQ2(Abc_Ntk_t*, std::vector<char*>&, std::vector<Abc_Ntk_t*>&);
Abc_Ntk_t* rel2FuncEQ3(Abc_Ntk_t*, std::vector<char*>&, std::vector<Abc_Ntk_t*>&);
#endif
