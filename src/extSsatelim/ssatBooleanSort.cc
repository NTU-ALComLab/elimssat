/**HFile****************************************************************
  FileName    [ssatBooleanSort.c]
  SystemName  []
  PackageName []
  Synopsis    []
  Author      [Kuan-Hua Tu]
  
  Affiliation []
  Date        [Sat Apr 11 00:20:25 CST 2020]
***********************************************************************/

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "ssatBooleanSort.h"
#include "extUtil/util.h"
#include "bdd/extrab/extraBdd.h" // 
#include "ext-Synthesis/synthesis.h"
#include <vector>
#include <algorithm>

ABC_NAMESPACE_IMPL_START

extern "C" {
int Abc_NtkDarCec(Abc_Ntk_t* pNtk1, Abc_Ntk_t* pNtk2, int nConfLimit,
                  int fPartition, int fVerbose);
}

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////
static DdNode * random_eliminate_reverse_one_pi_BDD(DdManager * dd, DdNode * bFunc, Vec_Int_t * pScope, int index_var);
static DdNode * random_eliminate_reverse_all_bit_BDD(DdManager * dd, DdNode * bFunc, Vec_Int_t * pScopeReverse);

DdNode * Util_sortBitonicBDD(DdManager * dd, DdNode * bFunc, Vec_Int_t * pScopeReverse)
{
  DdNode * ptemp;

  Cudd_Ref( bFunc );
  ptemp = bFunc;
  bFunc = random_eliminate_reverse_all_bit_BDD(dd, bFunc, pScopeReverse);
  Cudd_Ref( bFunc );
  Cudd_RecursiveDeref( dd, ptemp );

  int index;
  int entry;
  Vec_IntForEachEntryReverse( pScopeReverse, entry, index )
  {
    if (index == 0)
      break;
    ptemp = bFunc;
    bFunc = random_eliminate_reverse_one_pi_BDD(dd, bFunc, pScopeReverse, index - 1);
    Cudd_Ref( bFunc );
    Cudd_RecursiveDeref( dd, ptemp );
  }
  Cudd_Deref( bFunc );
  return bFunc;
}

Abc_Ntk_t * random_eliminate_reverse_one_pi2(Abc_Ntk_t * pNtk, Vec_Int_t * pScope, int index_var)
{
  Abc_Ntk_t * pNtkNew = Util_NtkAllocStrashWithName("ssat");
  Util_NtkCreatePiFrom( pNtkNew, pNtk, NULL );
  Util_NtkCreatePoWithName( pNtkNew, "out", NULL );
  Util_NtkConst1SetCopy( pNtk, pNtkNew );
  Util_NtkPiSetCopy    ( pNtk, pNtkNew, 0 );


  int entry = Vec_IntEntry(pScope, index_var);
  Abc_NtkPi(pNtk, entry)->pCopy = Abc_AigConst0(pNtkNew);
  Abc_Obj_t * pPhasePositive = Util_NtkAppend(pNtkNew, pNtk);
  Abc_NtkPi(pNtk, entry)->pCopy = Abc_AigConst1(pNtkNew);
  Abc_Obj_t * pPhaseNegative = Util_NtkAppend(pNtkNew, pNtk);

  // Abc_Ntk_t * pExist = Abc_NtkMiterQuantify(pNtk, entry, false);
  Abc_Obj_t * pObjAnd = Util_AigNtkAnd(pNtkNew, pPhasePositive, pPhaseNegative);
              pObjAnd = Util_AigNtkAnd(pNtkNew, pObjAnd,  Abc_ObjNot(Abc_NtkPi(pNtkNew, entry)) );

  // Abc_Ntk_t * pForall = Abc_NtkMiterQuantify(pNtk, entry, true);
  Abc_Obj_t * pObjOr = Util_AigNtkOr(pNtkNew, pPhasePositive, pPhaseNegative);
              pObjOr = Util_AigNtkAnd(pNtkNew, pObjOr, Abc_NtkPi(pNtkNew, entry));

  Abc_Obj_t * pObj = Util_AigNtkOr(pNtkNew, pObjAnd, pObjOr);
  Abc_ObjAddFanin(  Abc_NtkPo( pNtkNew, 0 ), pObj );

  return pNtkNew;
}

Abc_Ntk_t * random_eliminate_reverse_one_pi3(Abc_Ntk_t * pNtk, Vec_Int_t * pScope, int index_var)
{
  Abc_Ntk_t * pNtkNew = Util_NtkAllocStrashWithName("ssat");
  Util_NtkCreatePiFrom( pNtkNew, pNtk, NULL );
  Util_NtkCreatePoWithName( pNtkNew, "out", NULL );

  int entry = Vec_IntEntry(pScope, index_var);
  Abc_Obj_t *pRoot;

  Vec_Int_t *pScopeNew = Vec_IntAlloc(0);
  Vec_IntPush(pScopeNew, Abc_NtkPi(pNtk, entry)->Id);

  Abc_Ntk_t * pNtkRel1 = Abc_NtkMiterQuantify(pNtk, entry, 1);
  Util_NtkConst1SetCopy(pNtkRel1, pNtkNew);
  Util_NtkPiSetCopy(pNtkRel1, pNtkNew, 0);
  pRoot = Abc_NtkCo( pNtkRel1, 0 );
  Abc_NtkMiterAddCone( pNtkRel1, pNtkNew, pRoot );
  Abc_Obj_t *pExist = Abc_ObjNotCond( Abc_ObjFanin0(pRoot)->pCopy, Abc_ObjFaninC0(pRoot) );
  Abc_NtkDelete(pNtkRel1);

  Abc_Ntk_t * pNtkRel2 = Abc_NtkMiterQuantify(pNtk, entry, 0);
  Util_NtkConst1SetCopy(pNtkRel2, pNtkNew);
  Util_NtkPiSetCopy(pNtkRel2, pNtkNew, 0);
  pRoot = Abc_NtkCo( pNtkRel2, 0 );
  Abc_NtkMiterAddCone( pNtkRel2, pNtkNew, pRoot );
  Abc_Obj_t *pForall = Abc_ObjNotCond(Abc_ObjFanin0(pRoot)->pCopy, Abc_ObjFaninC0(pRoot));
  Abc_NtkDelete(pNtkRel2);

  Abc_Obj_t* pObj1 = Util_AigNtkAnd(pNtkNew, pExist, Abc_NtkPi(pNtkNew, entry));
  Abc_Obj_t* pObj2 = Util_AigNtkAnd(pNtkNew, pForall, Abc_ObjNot(Abc_NtkPi(pNtkNew, entry)));

  Abc_Obj_t * pObj = Util_AigNtkOr(pNtkNew, pObj1, pObj2);
  Abc_ObjAddFanin(  Abc_NtkPo( pNtkNew, 0 ), pObj );

  return pNtkNew;
}

// Xv * f(Xv = 0) * f(Xv = 1) + (-Xv) * ( f(Xv = 0) + f(Xv = 1) )
static DdNode * random_eliminate_reverse_one_pi_BDD(DdManager * dd, DdNode * bFunc, Vec_Int_t * pScope, int index_var)
{
  DdNode * pTemp;
  Cudd_Ref( bFunc );
  int var_index = Vec_IntEntry(pScope, index_var);
  DdNode * var = Cudd_bddIthVar(dd, var_index);
  Cudd_Ref(var);
  Cudd_Ref(Cudd_Not(var));

  DdNode * pForall = Cudd_bddUnivAbstract( dd, bFunc, var);
           Cudd_Ref(pForall);
           pTemp = pForall;
           pForall = Cudd_bddAnd(dd, pForall, Cudd_Not(var));
           Cudd_Ref(pForall);
           Cudd_RecursiveDeref( dd, pTemp);

  DdNode * pExist = Cudd_bddExistAbstract( dd, bFunc, var);
           Cudd_Ref(pExist);
           pTemp = pExist;
           pExist = Cudd_bddAnd(dd, pExist, var);
           Cudd_Ref(pExist);
           Cudd_RecursiveDeref( dd, pTemp);

  pTemp = pExist;
  pExist = Cudd_bddOr(dd, pForall, pExist); 
  Cudd_Ref(pExist);
  Cudd_RecursiveDeref( dd, pTemp);

  Cudd_RecursiveDeref( dd, pForall);
  Cudd_RecursiveDeref( dd, bFunc);
  Cudd_RecursiveDeref( dd, var);
  Cudd_RecursiveDeref( dd, Cudd_Not(var));
  Cudd_Deref(pExist);
  return pExist;
}

Abc_Ntk_t * random_eliminate_reverse_all_bit2(Abc_Ntk_t * pNtk, Vec_Int_t * pScopeReverse)
{
  Abc_Ntk_t * pNtkNew = Util_NtkAllocStrashWithName("ssat");
  Util_NtkCreatePiFrom( pNtkNew, pNtk, NULL );
  Util_NtkCreatePoWithName( pNtkNew, "out", NULL );
  Util_NtkConst1SetCopy( pNtk, pNtkNew );
  Util_NtkPiSetCopy    ( pNtk, pNtkNew, 0 );

  int last_var = Vec_IntEntryLast(pScopeReverse);
  Abc_NtkPi(pNtk, last_var)->pCopy = Abc_AigConst0(pNtkNew);
  Abc_Obj_t * pPhasePositive = Util_NtkAppend(pNtkNew, pNtk);
  Abc_NtkPi(pNtk, last_var)->pCopy = Abc_AigConst1(pNtkNew);
  Abc_Obj_t * pPhaseNegative = Util_NtkAppend(pNtkNew, pNtk);
  int index;
  int entry;
  Vec_IntForEachEntry( pScopeReverse, entry, index )
  {
    Abc_NtkPi(pNtk, entry)->pCopy = Abc_ObjNot( Abc_NtkPi(pNtkNew, entry) );
  }
  Abc_NtkPi(pNtk, last_var)->pCopy = Abc_AigConst1(pNtkNew);
  Abc_Obj_t * pPhaseReverseNegative = Util_NtkAppend(pNtkNew, pNtk);
  Abc_NtkPi(pNtk, last_var)->pCopy = Abc_AigConst0(pNtkNew);
  Abc_Obj_t * pPhaseReversePositive = Util_NtkAppend(pNtkNew, pNtk);

  Abc_Obj_t * pObjAnd = Util_AigNtkAnd(pNtkNew, pPhasePositive, pPhaseReverseNegative);
              pObjAnd = Util_AigNtkAnd(pNtkNew, pObjAnd, Abc_ObjNot(Abc_NtkPi(pNtkNew, last_var)) );

  Abc_Obj_t * pObjOr = Util_AigNtkOr(pNtkNew, pPhaseNegative, pPhaseReversePositive);
              pObjOr = Util_AigNtkAnd(pNtkNew, pObjOr, Abc_NtkPi(pNtkNew, last_var));

  Abc_Obj_t * pObj = Util_AigNtkOr(pNtkNew, pObjAnd, pObjOr);
  Abc_ObjAddFanin(  Abc_NtkPo( pNtkNew, 0 ), pObj );

  return pNtkNew;
}

DdNode * random_eliminate_reverse_all_bit_BDD(DdManager * dd, DdNode * bFunc, Vec_Int_t * pScopeReverse)
{
  DdNode * ptemp;

  Cudd_Ref( bFunc );
  int last_var_index = Vec_IntEntryLast(pScopeReverse);
  DdNode * last_var = Cudd_bddIthVar(dd, last_var_index);
  Cudd_Ref( last_var );
  Cudd_Ref( Cudd_Not(last_var) );

  DdNode * pPhasePositive = Cudd_Cofactor(dd, bFunc, Cudd_Not(last_var));
  Cudd_Ref( pPhasePositive );
  DdNode * pPhaseNegative = Cudd_Cofactor(dd, bFunc, last_var);
  Cudd_Ref( pPhaseNegative );

  DdNode * pPhaseReversePositive = pPhasePositive;
  Cudd_Ref( pPhaseReversePositive );
  int index;
  int entry;
  Vec_IntForEachEntry( pScopeReverse, entry, index )
  {
    if (entry == last_var_index)
      continue;
    DdNode * IthVar = Cudd_bddIthVar(dd, entry);
    Cudd_Ref( IthVar );
    Cudd_Ref( Cudd_Not(IthVar) );
    ptemp = pPhaseReversePositive;
    pPhaseReversePositive = Cudd_bddCompose(dd, pPhaseReversePositive, Cudd_Not(IthVar), entry);
    Cudd_Ref( pPhaseReversePositive );
    Cudd_RecursiveDeref( dd, IthVar );
    Cudd_RecursiveDeref( dd, Cudd_Not(IthVar) );
    Cudd_RecursiveDeref( dd, ptemp );
  }

  DdNode * pPhaseReverseNegative = pPhaseNegative;
  Cudd_Ref( pPhaseReverseNegative );
  Vec_IntForEachEntry( pScopeReverse, entry, index )
  {
    if (entry == last_var_index)
      continue;
    DdNode * IthVar = Cudd_bddIthVar(dd, entry);
    Cudd_Ref( IthVar );
    Cudd_Ref( Cudd_Not(IthVar) );
    ptemp = pPhaseReverseNegative;
    pPhaseReverseNegative = Cudd_bddCompose(dd, pPhaseReverseNegative, Cudd_Not(IthVar), entry);
    Cudd_Ref( pPhaseReverseNegative );
    Cudd_RecursiveDeref( dd, IthVar );
    Cudd_RecursiveDeref( dd, Cudd_Not(IthVar) );
    Cudd_RecursiveDeref( dd, ptemp );
  }

  DdNode * pObjAnd = Cudd_bddAnd(dd, pPhasePositive, pPhaseReverseNegative);
           Cudd_Ref( pObjAnd );
           ptemp = pObjAnd;
           pObjAnd = Cudd_bddAnd(dd, pObjAnd, Cudd_Not(last_var));
           Cudd_Ref( pObjAnd );
           Cudd_RecursiveDeref( dd, ptemp );
  DdNode * pObjOr = Cudd_bddOr(dd, pPhaseNegative, pPhaseReversePositive);
           Cudd_Ref( pObjOr );
           ptemp = pObjOr;
           pObjOr = Cudd_bddAnd(dd, pObjOr, last_var);
           Cudd_Ref( pObjOr );
           Cudd_RecursiveDeref( dd, ptemp );

  ptemp = pObjOr;
  pObjOr = Cudd_bddOr(dd, pObjAnd, pObjOr); 
  Cudd_Ref( pObjOr );
  Cudd_RecursiveDeref( dd, ptemp );

  Cudd_RecursiveDeref( dd, pPhasePositive);
  Cudd_RecursiveDeref( dd, pPhaseNegative);
  Cudd_RecursiveDeref( dd, pPhaseReverseNegative);
  Cudd_RecursiveDeref( dd, pPhaseReversePositive);
  Cudd_RecursiveDeref( dd, bFunc);
  Cudd_RecursiveDeref( dd, pObjAnd);
  Cudd_RecursiveDeref( dd, last_var);
  Cudd_RecursiveDeref( dd, Cudd_Not(last_var));
  Cudd_Deref( pObjOr );
  return pObjOr;
}

Abc_Ntk_t * Util_sortBitonic(Abc_Ntk_t * pNtk, Vec_Int_t * pScopeReverse)
{
  Abc_Ntk_t * pNtkOld = pNtk;
  Abc_Ntk_t * pNtkNew = pNtk;
  int node_count = Abc_NtkNodeNum(pNtkOld);
  pNtkNew = random_eliminate_reverse_all_bit2(pNtkOld, pScopeReverse);
  pNtkNew = Util_NtkDFraig(pNtkNew, 1, 0);
  pNtkNew = Util_NtkDc2(pNtkNew, 1);
  pNtkNew = Util_NtkResyn2rs(pNtkNew, 1);
  if (Abc_NtkNodeNum(pNtkNew) > node_count * 1.2) {
    pNtkNew = Util_NtkDFraig(pNtkNew, 1, 0);
    // pNtkNew = Util_NtkResyn2rs(pNtkNew, 1);
  }
  Abc_NtkDelete(pNtkOld);
  pNtkOld = pNtkNew;
  int index;
  int entry;
  Vec_IntForEachEntryReverse( pScopeReverse, entry, index )
  {
    if (index == 0)
      break;
    // pNtkNew  = random_eliminate_reverse_one_pi2(pNtkOld, pScopeReverse, index - 1);
    node_count = Abc_NtkNodeNum(pNtkOld);
    pNtkNew = random_eliminate_reverse_one_pi3(pNtkOld, pScopeReverse, index - 1);
    // Abc_Ntk_t *pNtkNew2 = random_eliminate_reverse_one_pi3(Abc_NtkDup(pNtkOld), pScopeReverse, index - 1);
    // Abc_NtkDarCec(pNtkNew, pNtkNew2, 100000, 0, 0);
    if (Abc_NtkNodeNum(pNtkNew) > node_count * 1.2) {
      pNtkNew = Util_NtkDFraig(pNtkNew, 1, 0);
      pNtkNew = Util_NtkDc2(pNtkNew, 1);
      pNtkNew = Util_NtkResyn2rs(pNtkNew, 1);
    }
    Abc_NtkDelete(pNtkOld);
    pNtkOld = pNtkNew;
  }
  return pNtkNew;
}


DdNode *ssat_BDDReorder(DdManager *ddNew, DdManager *dd, DdNode * bFunc, int target_var, int target_level) {
  int bdd_perm[dd->size];
  int lev = Cudd_ReadPerm(dd, target_var);
  if (target_level < lev) {
    for (int i = 0; i < dd->size; i++) {
      int var_level = Cudd_ReadPerm(dd, i);
      if (target_level <= var_level && var_level < lev) {
        bdd_perm[var_level+1] = i;
      } else {
        bdd_perm[var_level] = i;
      }
    }
  } else {
    for (int i = 0; i < dd->size; i++) {
      int var_level = Cudd_ReadPerm(dd, i);
      if (target_level >= var_level && var_level > lev) {
        bdd_perm[var_level-1] = i;
      } else {
        bdd_perm[var_level] = i;
      }
    }
  }
  bdd_perm[target_level] = target_var;
  target_level++;
  Cudd_ShuffleHeap( ddNew, bdd_perm );
  DdNode *bFuncNew = Cudd_bddTransfer( dd, ddNew, bFunc );
  Cudd_Ref( bFuncNew );
  Cudd_RecursiveDeref(dd, bFunc);
  Cudd_Quit(dd);
  Cudd_ReduceHeap(ddNew, CUDD_REORDER_SIFT, 5000);
  Cudd_ReduceHeap(ddNew, CUDD_REORDER_RANDOM, 5000);
  Cudd_ReduceHeap(ddNew, CUDD_REORDER_GROUP_SIFT, 5000);
  return bFuncNew;
}

ABC_NAMESPACE_IMPL_END
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
