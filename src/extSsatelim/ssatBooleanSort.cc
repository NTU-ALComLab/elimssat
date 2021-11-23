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
#include <vector>
#include <algorithm>

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////
extern "C" DdNode * ExistQuantify(DdManager * dd, DdNode * bFunc, Vec_Int_t * pPiIndex);
DdNode * sort(DdManager ** dd, DdNode * bFunc, Vec_Int_t * pScope);

// use poinrter to pointer of DdManager because sorting function would change
// the reference pointer
extern "C" DdNode * RandomQuantify(DdManager ** dd, DdNode * bFunc, Vec_Int_t * pScope)
{
  Cudd_AutodynDisable(*dd);
  return sort(dd, bFunc, pScope);
}

/**Function*************************************************************
  Synopsis    []
  Description []
               
  SideEffects []
  SeeAlso     []
***********************************************************************/
Abc_Ntk_t * random_eliminate_reverse_one_pi(Abc_Ntk_t * pNtk, Vec_Int_t * pScope, int index_var)
{
  Abc_Ntk_t * pNtkNew = Util_NtkAllocStrashWithName("ssat");
  Util_NtkCreatePiFrom( pNtkNew, pNtk, NULL );
  Util_NtkCreatePoWithName( pNtkNew, "out", NULL );
  Util_NtkConst1SetCopy( pNtk, pNtkNew );
  Util_NtkPiSetCopy    ( pNtk, pNtkNew, 0 );

  Abc_Obj_t * pPhasePositive = Util_NtkAppend(pNtkNew, pNtk);
  int entry = Vec_IntEntry(pScope, index_var);
  Abc_NtkPi(pNtk, entry)->pCopy = Abc_ObjNot( Abc_NtkPi(pNtkNew, entry) );
  Abc_Obj_t * pPhaseNegative = Util_NtkAppend(pNtkNew, pNtk);

  Abc_Obj_t * pObjAnd = Util_AigNtkAnd(pNtkNew, pPhasePositive, pPhaseNegative);
              pObjAnd = Util_AigNtkAnd(pNtkNew, pObjAnd,  Abc_ObjNot(Abc_NtkPi(pNtkNew, entry)) );

  Abc_Obj_t * pObjOr = Util_AigNtkOr(pNtkNew, pPhasePositive, pPhaseNegative);
              pObjOr = Util_AigNtkAnd(pNtkNew, pObjOr, Abc_NtkPi(pNtkNew, entry));

  Abc_Obj_t * pObj = Util_AigNtkOr(pNtkNew, pObjAnd, pObjOr);
  Abc_ObjAddFanin(  Abc_NtkPo( pNtkNew, 0 ), pObj );

  return pNtkNew;
}

Abc_Ntk_t * random_eliminate_reverse_all_bit(Abc_Ntk_t * pNtk, Vec_Int_t * pScopeReverse)
{
  Abc_Ntk_t * pNtkNew = Util_NtkAllocStrashWithName("ssat");
  Util_NtkCreatePiFrom( pNtkNew, pNtk, NULL );
  Util_NtkCreatePoWithName( pNtkNew, "out", NULL );
  Util_NtkConst1SetCopy( pNtk, pNtkNew );
  Util_NtkPiSetCopy    ( pNtk, pNtkNew, 0 );
  
  Abc_Obj_t * pPhasePositive = Util_NtkAppend(pNtkNew, pNtk);
  int index;
  int entry;
  Vec_IntForEachEntry( pScopeReverse, entry, index )
  {
    Abc_NtkPi(pNtk, entry)->pCopy = Abc_ObjNot( Abc_NtkPi(pNtkNew, entry) );
  }
  Abc_Obj_t * pPhaseNegative = Util_NtkAppend(pNtkNew, pNtk);

  int pi_index = Vec_IntEntryLast(pScopeReverse);
  Abc_Obj_t * pObjAnd = Util_AigNtkAnd(pNtkNew, pPhasePositive, pPhaseNegative);
              pObjAnd = Util_AigNtkAnd(pNtkNew, pObjAnd, Abc_ObjNot(Abc_NtkPi(pNtkNew, pi_index)) );

  Abc_Obj_t * pObjOr = Util_AigNtkOr(pNtkNew, pPhasePositive, pPhaseNegative);
              pObjOr = Util_AigNtkAnd(pNtkNew, pObjOr, Abc_NtkPi(pNtkNew, pi_index));

  Abc_Obj_t * pObj = Util_AigNtkOr(pNtkNew, pObjAnd, pObjOr);
  Abc_ObjAddFanin(  Abc_NtkPo( pNtkNew, 0 ), pObj );

  return pNtkNew;
}

Abc_Ntk_t * Util_function_sort_Bitonic(Abc_Ntk_t * pNtk, Vec_Int_t * pScopeReverse)
{
  Abc_Ntk_t * pNtkOld = pNtk;
  Abc_Ntk_t * pNtkNew = pNtk;
  pNtkNew = random_eliminate_reverse_all_bit(pNtkOld, pScopeReverse);
  Abc_NtkDelete(pNtkOld);
  pNtkOld = pNtkNew;
  int index;
  int entry;
  Vec_IntForEachEntryReverse( pScopeReverse, entry, index )
  {
    if (index == 0)
      break;
    pNtkNew = random_eliminate_reverse_one_pi(pNtkOld, pScopeReverse, index - 1);
    Abc_NtkDelete(pNtkOld);
    pNtkOld = pNtkNew;
  }
  return pNtkNew;
}

Abc_Ntk_t * Util_FunctionSort_1(Abc_Ntk_t * pNtk, Vec_Int_t * pScope)
{
    Vec_Int_t * pScopeReverse = Vec_IntAlloc(0);
    int index;
    int entry;
    Vec_IntForEachEntryReverse( pScope, entry, index )
    {
        Vec_IntPush(pScopeReverse, entry);
        pNtk = Util_function_sort_Bitonic(pNtk, pScopeReverse);
    }
    return pNtk;
}

//=================================================
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
  pNtkNew = Util_NtkDc2(pNtkNew, 1);
  pNtkNew = Util_NtkResyn2(pNtkNew, 1);

  return pNtkNew;
}

// Xv * f(Xv = 0) * f(Xv = 1) + (-Xv) * ( f(Xv = 0) + f(Xv = 1) )
DdNode * random_eliminate_reverse_one_pi2_BDD(DdManager * dd, DdNode * bFunc, Vec_Int_t * pScope, int index_var)
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

Abc_Ntk_t * Util_FunctionSort_1bit( Abc_Ntk_t * pNtk, Vec_Int_t * pScope, int index_var )
{
  if (!Abc_NtkIsBddLogic(pNtk))
    return NULL;
  int fVerbose = 0;
  int fReorder = 1;
  int fReverse = 0;
  // int fDualRail = 0;
  int fBddSizeMax = ABC_INFINITY;

  pNtk = Abc_NtkStrash( pNtk, 0, 0, 0 );
  DdManager * dd = (DdManager *)Abc_NtkBuildGlobalBdds(pNtk, fBddSizeMax, 1, fReorder, fReverse, fVerbose) ;
  DdNode * bFunc = (DdNode *)Abc_ObjGlobalBdd(Abc_NtkPo(pNtk, 0));

  bFunc = random_eliminate_reverse_one_pi2_BDD(dd, bFunc, pScope, index_var);
  return Abc_NtkDeriveFromBdd( dd, bFunc, NULL, NULL );
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

DdNode * random_eliminate_reverse_all_bit2_BDD(DdManager * dd, DdNode * bFunc, Vec_Int_t * pScopeReverse)
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

Abc_Ntk_t * Util_FunctionSort_allbit( Abc_Ntk_t * pNtk, Vec_Int_t * pScope)
{
  if (!Abc_NtkIsBddLogic(pNtk))
    return NULL;

  Vec_Int_t * pScopeReverse = Vec_IntAlloc(0);
  int index;
  int entry;
  Vec_IntForEachEntryReverse( pScope, entry, index )
  {
    Vec_IntPush(pScopeReverse, entry);
  }

  int fVerbose = 0;
  int fReorder = 1;
  int fReverse = 0;
  // int fDualRail = 0;
  int fBddSizeMax = ABC_INFINITY;

  pNtk = Abc_NtkStrash( pNtk, 0, 0, 0 );
  DdManager * dd = (DdManager *)Abc_NtkBuildGlobalBdds(pNtk, fBddSizeMax, 1, fReorder, fReverse, fVerbose) ;
  DdNode * bFunc = (DdNode *)Abc_ObjGlobalBdd(Abc_NtkPo(pNtk, 0));

  bFunc = random_eliminate_reverse_all_bit2_BDD(dd, bFunc, pScopeReverse);
  Vec_IntFree(pScopeReverse);
  return Abc_NtkDeriveFromBdd( dd, bFunc, NULL, NULL );
}

Abc_Ntk_t * Util_function_sort_Bitonic2(Abc_Ntk_t * pNtk, Vec_Int_t * pScopeReverse)
{
  Abc_Ntk_t * pNtkOld = pNtk;
  Abc_Ntk_t * pNtkNew = pNtk;
  pNtkNew = random_eliminate_reverse_all_bit2(pNtkOld, pScopeReverse);
  Abc_NtkDelete(pNtkOld);
  pNtkOld = pNtkNew;
  int index;
  int entry;
  Vec_IntForEachEntryReverse( pScopeReverse, entry, index )
  {
    Abc_Print(1, "[DEBUG] working on %d/%d variables\n", index+1, pScopeReverse->nSize);
    fflush(stdout);
    if (index == 0)
      break;
    pNtkNew = random_eliminate_reverse_one_pi2(pNtkOld, pScopeReverse, index - 1);
    Abc_NtkDelete(pNtkOld);
    Abc_Print(1, "[DEBUG] Object Number of current network: %d\n", Abc_NtkNodeNum(pNtkNew));
    pNtkOld = pNtkNew;
  }
  Abc_Print(1, "\n");
  return pNtkNew;
}

DdNode * block(DdManager * dd, DdNode * bFunc, Vec_Int_t * pScopeReverse)
{
  DdNode * ptemp;

  Cudd_Ref( bFunc );
  ptemp = bFunc;
  bFunc = random_eliminate_reverse_all_bit2_BDD(dd, bFunc, pScopeReverse);
  Cudd_Ref( bFunc );
  Cudd_RecursiveDeref( dd, ptemp );

  int index;
  int entry;
  Vec_IntForEachEntryReverse( pScopeReverse, entry, index )
  {
    if (index == 0)
      break;
    ptemp = bFunc;
    bFunc = random_eliminate_reverse_one_pi2_BDD(dd, bFunc, pScopeReverse, index - 1);
    Cudd_Ref( bFunc );
    Cudd_RecursiveDeref( dd, ptemp );
  }
  Cudd_Deref( bFunc );
  return bFunc;
}

Abc_Ntk_t * Util_FunctionSort_block( Abc_Ntk_t * pNtk, Vec_Int_t * pScope )
{
  if (!Abc_NtkIsBddLogic(pNtk))
    return NULL;

  Vec_Int_t * pScopeReverse = Vec_IntAlloc(0);
  int index;
  int entry;
  Vec_IntForEachEntryReverse( pScope, entry, index )
  {
    Vec_IntPush(pScopeReverse, entry);
  }

  int fVerbose = 0;
  int fReorder = 1;
  int fReverse = 0;
  // int fDualRail = 0;
  int fBddSizeMax = ABC_INFINITY;

  pNtk = Abc_NtkStrash( pNtk, 0, 0, 0 );
  
  DdManager * dd = (DdManager *)Abc_NtkBuildGlobalBdds(pNtk, fBddSizeMax, 1, fReorder, fReverse, fVerbose);
  // DdManager * dd = (DdManager *)Abc_NtkGlobalBdd(pNtk);
  DdNode * bFunc = (DdNode *)Abc_ObjGlobalBdd(Abc_NtkPo(pNtk, 0));

  bFunc = block(dd, bFunc, pScopeReverse);
  Vec_IntFree(pScopeReverse);
  return Abc_NtkDeriveFromBdd( dd, bFunc, NULL, NULL );
}

Abc_Ntk_t * Util_FunctionSort_2(Abc_Ntk_t * pNtk, Vec_Int_t * pScope)
{
    Vec_Int_t * pScopeReverse = Vec_IntAlloc(0);
    int index;
    int entry;
    Vec_IntForEachEntryReverse( pScope, entry, index )
    {
        Vec_IntPush(pScopeReverse, entry);
        pNtk = Util_function_sort_Bitonic2(pNtk, pScopeReverse);
    }
    return pNtk;
}

static DdNode *sorting_reorder(DdManager *ddNew, DdManager *dd, DdNode * bFunc, int target_var, int target_level) {
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
  Cudd_Quit(dd);
  Cudd_ReduceHeap(ddNew, CUDD_REORDER_EXACT, 10000);
  return bFuncNew;
}

DdNode * sort(DdManager ** dd, DdNode * bFunc, Vec_Int_t * pScope)
{
  DdNode * ptemp;
  Cudd_Ref( bFunc );
  Vec_Int_t * pScopeReverse = Vec_IntAlloc(0);
  int index;
  int entry;
  int target_level = 0;
  Vec_IntForEachEntryReverse( pScope, entry, index )
  {
      Abc_Print(1, "[DEBUG] working on %d/%d variables\n", index+1, pScope->nSize);
      Vec_IntPush(pScopeReverse, entry);
      ptemp = bFunc;
      bFunc = block(*dd, bFunc, pScopeReverse);
      Cudd_Ref( bFunc );
      Cudd_RecursiveDeref( *dd, ptemp );
      int lev = Cudd_ReadPerm(*dd, entry);
      Abc_Print(1, "[DEBUG] Before => The level for variable %d: %d\n", entry, lev);
      Abc_Print(1, "[DEBUG] Before => Object Number of dd manager: %d\n", Cudd_ReadNodeCount(*dd));
      Abc_Print(1, "[DEBUG] Before => Object Number of current network: %d\n", Cudd_DagSize(bFunc));
      DdManager *ddNew = Cudd_Init( (*dd)->size, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
      bFunc = sorting_reorder(ddNew, *dd, bFunc, entry, target_level);
      target_level++;
      *dd = ddNew;
      lev = Cudd_ReadPerm(*dd, entry);
      Abc_Print(1, "[DEBUG] After => The level for variable %d: %d\n", entry, lev);
      Abc_Print(1, "[DEBUG] After => Object Number of dd manager: %d\n", Cudd_ReadNodeCount(*dd));
      Abc_Print(1, "[DEBUG] After => Object Number of current network: %d\n", Cudd_DagSize(bFunc));
  }
  Vec_IntFree(pScopeReverse);
  Cudd_Deref( bFunc );
  return bFunc;
}

Abc_Ntk_t * Util_NtkRandomQuantifyPis_BDD( Abc_Ntk_t * pNtk, Vec_Int_t * pScope )
{
  if (!Abc_NtkIsBddLogic(pNtk))
    return NULL;
  // Abc_Print( 1, "Util_NtkRandomQuantifyPis_BDD\n" ); 
  int fVerbose = 0;
  int fReorder = 1;
  int fReverse = 0;
  // int fDualRail = 0;
  int fBddSizeMax = ABC_INFINITY;

  pNtk = Abc_NtkStrash( pNtk, 0, 0, 0 );
  DdManager * dd = (DdManager *)Abc_NtkBuildGlobalBdds(pNtk, fBddSizeMax, 1, fReorder, fReverse, fVerbose) ;
  DdNode * bFunc = (DdNode *)Abc_ObjGlobalBdd(Abc_NtkPo(pNtk, 0));

  bFunc = sort(&dd, bFunc, pScope);

  // Abc_Print( 1, "Util_NtkRandomQuantifyPis_BDD\n" ); 
  return Abc_NtkDeriveFromBdd( dd, bFunc, NULL, NULL );
}

extern "C" DdNode * RandomQuantifyReverse(DdManager ** dd, DdNode * bFunc, Vec_Int_t * pScopeReverse)
{
  Vec_Int_t * pScope = Vec_IntAlloc(0);
  int index;
  int entry;
  Vec_IntForEachEntryReverse( pScopeReverse, entry, index )
  {
      Vec_IntPush(pScope, entry);
  }
  bFunc = RandomQuantify(dd, bFunc, pScope);
  Vec_IntFree(pScope);
  return bFunc;
}

//=================================================
Abc_Ntk_t * Util_FunctionSortSeq(Abc_Ntk_t * pNtk, Vec_Int_t * pScopeReverse)
{
    return Util_FunctionSortSeq_Select(pNtk, pScopeReverse, 2);
}

Abc_Ntk_t * Util_FunctionSort(Abc_Ntk_t * pNtk, Vec_Int_t * pScope)
{
    return Util_FunctionSort_Select(pNtk, pScope, 2);
}

Abc_Ntk_t * Util_FunctionSortSeq_Select(Abc_Ntk_t * pNtk, Vec_Int_t * pScopeReverse, int method)
{
    if (method == 1)
        return Util_function_sort_Bitonic(pNtk, pScopeReverse);
    if (method == 2)
        return Util_function_sort_Bitonic2(pNtk, pScopeReverse);
    
    Abc_Print( 1, "method %d is not existed\n", method );
    return NULL;
}

Abc_Ntk_t * Util_FunctionSort_Select(Abc_Ntk_t * pNtk, Vec_Int_t * pScopeReverse, int method)
{
    if (method == 1)
        return Util_FunctionSort_1(pNtk, pScopeReverse);
    if (method == 2)
        return Util_FunctionSort_2(pNtk, pScopeReverse);

    Abc_Print( 1, "method %d is not existed\n", method );
    return NULL;
}

ABC_NAMESPACE_IMPL_END
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
