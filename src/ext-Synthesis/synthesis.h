#ifndef ABC__ext__synthesis_h
#define ABC__ext__synthesis_h

// #include "extSsatelim/ssatelim.h"
#include "base/main/main.h"

ABC_NAMESPACE_HEADER_START

// cadetTest.cpp
extern int cadetTest(char* pFileName, int fVerbose);
// cadetExist.cpp
extern Abc_Ntk_t* cadetExistsElim(Abc_Ntk_t* pNtk, Vec_Int_t* pScope);
extern Abc_Ntk_t* applyExists(Abc_Ntk_t* pNtk, Abc_Ntk_t* pExist);

// manthanTest.cpp
extern int manthanTest(char* pFileName, int fVerbose);
// manthanExist.cpp
extern Abc_Ntk_t* manthanExistsElim(Abc_Ntk_t* pNtk, Vec_Int_t* pScope, int fVerbose, char* pName);
extern Abc_Ntk_t* applySkolem(Abc_Ntk_t *pNtk, Abc_Ntk_t *pSkolem,
                              Vec_Int_t *vForalls, Vec_Int_t *vExists);
extern Abc_Ntk_t* r2fExistElim(Abc_Ntk_t* pNtk, Vec_Int_t* pScope, int fVerbose);
extern int r2fTest(char *pFileName, int fVerbose);

ABC_NAMESPACE_HEADER_END
#endif
