#ifndef INTREL2FUNC_HPP 
#define INTREL2FUNC_HPP
#include <vector>
#include "misc.hpp"
#include "base/abc/abc.h"
Abc_Ntk_t* rel2FuncInt(Abc_Ntk_t* pNtk, std::vector<char*> & vNameY, std::vector<Abc_Ntk_t*> &vFunc);
Abc_Ntk_t* singleRel2FuncInt(Abc_Ntk_t* pNtk, char* yName);
#endif
