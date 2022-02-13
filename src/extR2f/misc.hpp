#ifndef MISC_HPP
#define MISC_HPP

#define MAX_AIG_NODE 30000
#include <vector>
#include "base/abc/abc.h"
#include "IntSolver.hpp"
extern "C" Abc_Ntk_t * Abc_NtkMulti( Abc_Ntk_t * pNtk, int nThresh, int nFaninMax, int fCnf, int fMulti, int fSimple, int fFactor );
extern "C" void Abc_NtkShow( Abc_Ntk_t * pNtk, int fGateNames, int fSeq, int fUseReverse );
extern "C" void Abc_NtkSynthesize( Abc_Ntk_t ** ppNtk, int fMoreEffort );
extern "C" Abc_Ntk_t * Abc_NtkIvyFraig( Abc_Ntk_t * pNtk, int nConfLimit, int fDoSparse, int fProve, int fTransfer, int fVerbose );
extern "C" int Abc_NtkDsdLocal( Abc_Ntk_t * pNtk, bool fVerbose, bool fRecursive );
extern "C" Abc_Ntk_t * Abc_NtkDsdGlobal( Abc_Ntk_t * pNtk, bool fVerbose, bool fPrint, bool fShort);
extern "C" void Abc_NtkCecSat( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int nConfLimit, int nInsLimit );
Vec_Ptr_t* ntkBfs(Abc_Ntk_t*);

void printLits(const vec<Lit> &lits);
Abc_Ntk_t* resyn(Abc_Ntk_t* pNtk);
void ntkAddOne( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkMiter );
Abc_Ntk_t* getEquIntNtk(Abc_Ntk_t*, int);
Abc_Ntk_t* EQ(Abc_Ntk_t* pNtk, char* yName);
Abc_Ntk_t* EQInt(Abc_Ntk_t* pNtk, char* yName);
Abc_Ntk_t* replaceX(Abc_Ntk_t* pNtk, Abc_Ntk_t* pNtkFunc, char* yName);
Abc_Ntk_t* partialClp(Abc_Ntk_t* pNtk, int nLevel, int levelSize);
bool checkRelFunc(Abc_Ntk_t* pNtkA, Abc_Ntk_t* pNtkB);
Abc_Ntk_t * globalBdd2Ntk( Abc_Ntk_t * pNtk );
Abc_Ntk_t* createRelNtkAnd(Abc_Ntk_t*);
Abc_Ntk_t* createRelNtkOr(Abc_Ntk_t*);
Abc_Ntk_t* createRelNtk(Abc_Ntk_t*);
Abc_Ntk_t * ntkTransRel( Abc_Ntk_t * pNtk, int fInputs, int fVerbose );
Abc_Ntk_t * createCubeNtk(Abc_Ntk_t*, int);
Abc_Ntk_t * createNtkUni(Abc_Ntk_t*, Abc_Ntk_t*);
void insertDC(Abc_Ntk_t*, int);
void orReplace(Abc_Ntk_t *pNtk, Abc_Obj_t *pObj);
int checkMem();
#endif
