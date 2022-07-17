#ifndef ABC__ext__unate_h
#define ABC__ext__unate_h

#include "base/main/main.h"
#include "aig/aig/aig.h"
#include "base/abc/abc.h"

extern void unateDetection(Abc_Ntk_t *, Vec_Int_t *, Vec_Ptr_t *, Vec_Ptr_t *);
extern Abc_Ntk_t* unatePreprocess(Abc_Ntk_t* pNtk, Vec_Int_t **pScope, bool verbose);

#endif
