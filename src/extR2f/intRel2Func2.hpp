#ifndef INTREL2FUNC2_HPP 
#define INTREL2FUNC2_HPP
#include <vector>
#include "misc.hpp"
#include "base/abc/abc.h"
Abc_Ntk_t* rel2FuncInt2(Abc_Ntk_t* pNtk, std::vector<char*> & vNameY, std::vector<Abc_Ntk_t*> &vFunc, bool fVerbose = false);
#endif
