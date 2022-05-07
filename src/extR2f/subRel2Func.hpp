#ifndef SUBREL2FUNC_HPP 
#define SUBREL2FUNC_HPP
#include <vector>
#include "base/abc/abc.h"
void rel2FuncSub(Abc_Ntk_t*, std::vector<char*>&, std::vector<Abc_Ntk_t*>&);
Abc_Ntk_t* singleRel2Func(Abc_Ntk_t* pNtk, char* yName);
#endif
