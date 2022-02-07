#ifndef BDDREL2FUNC_HPP 
#define BDDREL2FUNC_HPP
#include <vector>
#include "misc.hpp"
#include "base/abc/abc.h"
int long rel2FuncBdd(Abc_Ntk_t*, std::vector<char*>&, std::vector<Abc_Ntk_t*>&, int choice);
#endif
