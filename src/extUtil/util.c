/**CFile****************************************************************
  FileName    [util.c]
  SystemName  [ABC: Logic synthesis and verification system.]
  PackageName []
  Synopsis    [Utility box]
  Author      [Kuan-Hua Tu(isuis3322@gmail.com)]

  Affiliation [NTU]
  Date        [2018.04.24]
***********************************************************************/

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "util.h"
#include "aig/gia/gia.h"
#include "aig/gia/giaAig.h"
#include "bdd/extrab/extraBdd.h" //
#include "proof/cec/cec.h"
#include "proof/dch/dch.h"
#include "utilSat.h"
#include <fcntl.h>
#include <math.h> // floor(), ceil(), log2()
#include <signal.h>
#include <stdarg.h>
#include <sys/wait.h>
#include <unistd.h>

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#ifndef min
static inline int min(int a, int b) { return (a > b) ? b : a; }
#endif

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************
  Synopsis    []
  Description [Create small ntk.
               most signficant bit first.
               ex:
               ab | AND(a,b)
               11 | 1
               10 | 0
               01 | 0
               00 | 0

               *pTruth = "1000"
               ]

  SideEffects []
  SeeAlso     []
***********************************************************************/
Abc_Ntk_t *Util_CreatNtkFromTruthBin(char *pTruth) {
  extern char *Abc_SopFromTruthBin(char *pTruth);
  char *pSopCover;
  if (pTruth == NULL)
    return NULL;
  if (*pTruth == '\0')
    return NULL;
  pSopCover = Abc_SopFromTruthBin(pTruth);
  if (pSopCover == NULL || pSopCover[0] == 0) {
    ABC_FREE(pSopCover);
    return NULL;
  }
  Abc_Ntk_t *pNtk = Abc_NtkCreateWithNode(pSopCover);
  ABC_FREE(pSopCover);
  Abc_Ntk_t *pNtkRet = Abc_NtkStrash(pNtk, 0, 1, 0);
  Abc_NtkDelete(pNtk);
  return pNtkRet;
}

/**Function*************************************************************
  Synopsis    []
  Description [Create ntk with constant function]

  SideEffects []
  SeeAlso     []
***********************************************************************/
Abc_Ntk_t *Util_CreatConstant0Ntk(int nInput) {
  Abc_Ntk_t *pNtk = Util_NtkAllocStrashWithName("const 0");
  Util_NtkCreateMultiPi(pNtk, nInput);
  Abc_Obj_t *Abc_Po = Abc_NtkCreatePo(pNtk);
  Abc_ObjAssignName(Abc_Po, "out", NULL);
  Abc_ObjAddFanin(Abc_Po, Abc_AigConst0(pNtk));
  return pNtk;
}

Abc_Ntk_t *Util_CreatConstant1Ntk(int nInput) {
  Abc_Ntk_t *pNtk = Util_NtkAllocStrashWithName("const 1");
  Util_NtkCreateMultiPi(pNtk, nInput);
  Abc_Obj_t *Abc_Po = Abc_NtkCreatePo(pNtk);
  Abc_ObjAssignName(Abc_Po, "out", NULL);
  Abc_ObjAddFanin(Abc_Po, Abc_AigConst1(pNtk));
  return pNtk;
}

/**Function*************************************************************
  Synopsis    []
  Description [Create empty ntk with name]

  SideEffects []
  SeeAlso     []
***********************************************************************/
Abc_Ntk_t *Util_NtkAllocStrashWithName(char *NtkName) {
  assert(NtkName);
  Abc_Ntk_t *pNtkNew = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
  pNtkNew->pName = Extra_UtilStrsav(NtkName);
  return pNtkNew;
}

/**Function*************************************************************
  Synopsis    []
  Description [Pi[0] is LSB]

  SideEffects []
  SeeAlso     []
***********************************************************************/
Abc_Ntk_t *Util_NtkCreateCounter(int nInput) {
  assert(nInput > 0);
  int nOutput = (nInput == 1) ? 1 : floor(log2(nInput)) + 1;

  Abc_Ntk_t *pNtk = Util_NtkAllocStrashWithName("counter");
  Util_NtkCreateMultiPi(pNtk, nInput);
  assert(Abc_NtkPiNum(pNtk) == nInput);
  Util_NtkCreateMultiPo(pNtk, nOutput);
  assert(Abc_NtkPoNum(pNtk) == nOutput);

  Vec_Ptr_t *pInput = Vec_PtrAlloc(nInput);
  Util_CollectPi(pInput, pNtk, 0, Abc_NtkPiNum(pNtk));
  assert(Vec_PtrSize(pInput) == nInput);
  Vec_Ptr_t *pOutput = Util_AigNtkCounter(pNtk, pInput, 0, nInput);
  assert(Vec_PtrSize(pOutput) == nOutput);
  for (int i = 0; i < nOutput; i++) {
    Abc_ObjAddFanin(Abc_NtkPo(pNtk, i), Vec_PtrEntry(pOutput, i));
  }
  Vec_PtrFree(pOutput);
  Vec_PtrFree(pInput);
  return pNtk;
}

/**Function*************************************************************
  Synopsis    []
  Description []

  SideEffects []
  SeeAlso     []
***********************************************************************/
Abc_Ntk_t *Util_NtkAnd(Abc_Ntk_t *pNtk1, Abc_Ntk_t *pNtk2) {
  assert(pNtk1);
  assert(pNtk2);
  assert(Abc_NtkPiNum(pNtk1) == Abc_NtkPiNum(pNtk2));
  assert(Abc_NtkPoNum(pNtk1) == 1);
  assert(Abc_NtkPoNum(pNtk2) == 1);

  char NtkMiterName[1000];
  sprintf(NtkMiterName, "%s_And", pNtk1->pName);
  Abc_Ntk_t *pNtkMiter = Util_NtkAllocStrashWithName(NtkMiterName);

  Util_NtkCreatePiFrom(pNtkMiter, pNtk1, NULL);
  Util_NtkCreatePoWithName(pNtkMiter, "miter", NULL);

  Abc_Obj_t *pObjFirst = Util_NtkAppendWithPiOrder(pNtkMiter, pNtk1, 0);
  Abc_Obj_t *pObjSecond = Util_NtkAppendWithPiOrder(pNtkMiter, pNtk2, 0);
  Abc_Obj_t *pObjNew = Util_AigNtkAnd(pNtkMiter, pObjFirst, pObjSecond);

  Abc_ObjAddFanin(Abc_NtkPo(pNtkMiter, 0), pObjNew);

  return pNtkMiter;
}

/**Function*************************************************************
  Synopsis    []
  Description []

  SideEffects []
  SeeAlso     []
***********************************************************************/
Abc_Ntk_t *Util_NtkOr(Abc_Ntk_t *pNtk1, Abc_Ntk_t *pNtk2) {
  assert(pNtk1);
  assert(pNtk2);
  assert(Abc_NtkPiNum(pNtk1) == Abc_NtkPiNum(pNtk2));
  assert(Abc_NtkPoNum(pNtk1) == 1);
  assert(Abc_NtkPoNum(pNtk2) == 1);

  char NtkMiterName[1000];
  sprintf(NtkMiterName, "%s_Or", pNtk1->pName);
  Abc_Ntk_t *pNtkMiter = Util_NtkAllocStrashWithName(NtkMiterName);

  Util_NtkCreatePiFrom(pNtkMiter, pNtk1, NULL);
  Util_NtkCreatePoWithName(pNtkMiter, "miter", NULL);

  Abc_Obj_t *pObjFirst = Util_NtkAppendWithPiOrder(pNtkMiter, pNtk1, 0);
  Abc_Obj_t *pObjSecond = Util_NtkAppendWithPiOrder(pNtkMiter, pNtk2, 0);
  Abc_Obj_t *pObjNew = Util_AigNtkOr(pNtkMiter, pObjFirst, pObjSecond);

  Abc_ObjAddFanin(Abc_NtkPo(pNtkMiter, 0), pObjNew);

  return pNtkMiter;
}

/**Function*************************************************************
  Synopsis    []
  Description []

  SideEffects []
  SeeAlso     []
***********************************************************************/
Abc_Ntk_t *Util_NtkMaj(Abc_Ntk_t *pNtk1, Abc_Ntk_t *pNtk2, Abc_Ntk_t *pNtk3) {
  assert(pNtk1);
  assert(pNtk2);
  assert(pNtk3);
  assert(Abc_NtkPiNum(pNtk1) == Abc_NtkPiNum(pNtk2));
  assert(Abc_NtkPiNum(pNtk1) == Abc_NtkPiNum(pNtk3));
  assert(Abc_NtkPoNum(pNtk1) == 1);
  assert(Abc_NtkPoNum(pNtk2) == 1);
  assert(Abc_NtkPoNum(pNtk3) == 1);

  char NtkMiterName[1000];
  sprintf(NtkMiterName, "%s_Maj", pNtk1->pName);
  Abc_Ntk_t *pNtkMiter = Util_NtkAllocStrashWithName(NtkMiterName);

  Util_NtkCreatePiFrom(pNtkMiter, pNtk1, NULL);
  Util_NtkCreatePoWithName(pNtkMiter, "miter", NULL);

  Abc_Obj_t *pPoA = Util_NtkAppendWithPiOrder(pNtkMiter, pNtk1, 0);
  Abc_Obj_t *pPoB = Util_NtkAppendWithPiOrder(pNtkMiter, pNtk2, 0);
  Abc_Obj_t *pPoC = Util_NtkAppendWithPiOrder(pNtkMiter, pNtk3, 0);
  Abc_Obj_t *pObjNew = Util_AigNtkMaj(pNtkMiter, pPoA, pPoB, pPoC);

  Abc_ObjAddFanin(Abc_NtkPo(pNtkMiter, 0), pObjNew);

  return pNtkMiter;
}

/**Function*************************************************************
  Synopsis    []
  Description []

  SideEffects []
  SeeAlso     []
***********************************************************************/
Abc_Ntk_t *Util_NtkExistQuantify(Abc_Ntk_t *pNtk, int pi) {
  if (Abc_ObjFanoutNum(Abc_NtkPi(pNtk, pi)) == 0)
    return pNtk;
  Abc_Ntk_t *pNtkNew =
      Util_NtkAllocStrashWithName(Abc_ObjName(Abc_NtkCo(pNtk, 0)));
  Util_NtkCreatePiFrom(pNtkNew, pNtk, NULL);
  Util_NtkCreatePoWithName(pNtkNew, "out", NULL);
  Util_NtkConst1SetCopy(pNtk, pNtkNew);
  Util_NtkPiSetCopy(pNtk, pNtkNew, 0);

  Abc_NtkPi(pNtk, pi)->pCopy = Abc_AigConst0(pNtkNew);
  Abc_Obj_t *pPhase0 = Util_NtkAppend(pNtkNew, pNtk);
  Abc_NtkPi(pNtk, pi)->pCopy = Abc_AigConst1(pNtkNew);
  Abc_Obj_t *pPhase1 = Util_NtkAppend(pNtkNew, pNtk);

  Abc_Obj_t *pObj = Util_AigNtkOr(pNtkNew, pPhase0, pPhase1);
  Abc_ObjAddFanin(Abc_NtkPo(pNtkNew, 0), pObj);
  return pNtkNew;
}

/**Function*************************************************************
  Synopsis    []
  Description []

  SideEffects []
  SeeAlso     []
***********************************************************************/
Abc_Ntk_t *Util_NtkExistQuantifyPis(Abc_Ntk_t *pNtk, Vec_Int_t *pPiIndex) {
  Abc_Ntk_t *pNtkTemp;

  int index;
  int entry;
  Vec_IntForEachEntry(pPiIndex, entry, index) {
    pNtk = Util_NtkExistQuantify(pNtkTemp = pNtk, entry);
    if (pNtk != pNtkTemp)
      Abc_NtkDelete(pNtkTemp);
  }
  return pNtk;
}

/**Function*************************************************************
  Synopsis    []
  Description []

  SideEffects []
  SeeAlso     []
***********************************************************************/
DdNode *ExistQuantify(DdManager *dd, DdNode *bFunc, Vec_Int_t *pPiIndex) {
  Cudd_Ref(bFunc);
  int *array = Vec_IntArray(pPiIndex);
  int n = Vec_IntSize(pPiIndex);
  DdNode *cube = Cudd_IndicesToCube(dd, array, n);
  Cudd_Ref(cube);
  DdNode *bRet = Cudd_bddExistAbstract(dd, bFunc, cube);
  Cudd_Ref(bRet);
  Cudd_RecursiveDeref(dd, cube);
  Cudd_RecursiveDeref(dd, bFunc);
  Cudd_Deref(bRet);
  return bRet;
}

Abc_Ntk_t *Util_NtkExistQuantifyPis_BDD(Abc_Ntk_t *pNtk, Vec_Int_t *pPiIndex) {
  if (!Abc_NtkIsBddLogic(pNtk))
    return NULL;
  // Abc_Print( 1, "Util_NtkExistQuantifyPis_BDD\n" );
  int fVerbose = 0;
  int fReorder = 1;
  int fReverse = 0;
  // int fDualRail = 0;
  int fBddSizeMax = ABC_INFINITY;

  pNtk = Abc_NtkStrash(pNtk, 0, 0, 0);
  DdManager *dd = (DdManager *)Abc_NtkBuildGlobalBdds(
      pNtk, fBddSizeMax, 1, fReorder, fReverse, fVerbose);
  // DdManager * dd = (DdManager *)Abc_NtkGlobalBddMan( pNtk );
  // DdManager * dd = (DdManager *)pNtk->pManFunc;
  // Cudd_PrintLinear(dd);
  if (dd == NULL)
    return NULL;

  DdNode *bFunc = (DdNode *)Abc_ObjGlobalBdd(Abc_NtkPo(pNtk, 0));
  bFunc = ExistQuantify(dd, bFunc, pPiIndex);

  // Abc_Print( 1, "Abc_NtkDeriveFromBdd\n" );
  return Abc_NtkDeriveFromBdd(dd, bFunc, NULL, NULL);
}

/**Function*************************************************************
  Synopsis    []
  Description []

  SideEffects []
  SeeAlso     []
***********************************************************************/
Abc_Ntk_t *Util_NtkExistQuantifyPisByBDD(Abc_Ntk_t *pNtk, Vec_Int_t *pPiIndex) {
  int fVerbose = 0;
  int fReorder = 1;
  int fReverse = 0;
  // int fDualRail = 0;
  int fBddSizeMax = ABC_INFINITY;
  DdManager *dd = (DdManager *)Abc_NtkBuildGlobalBdds(
      pNtk, fBddSizeMax, 1, fReorder, fReverse, fVerbose);
  // Abc_Ntk_t * pNtkNew = Abc_NtkCollapse( pNtk, fBddSizeMax, fDualRail,
  // fReorder, fReverse, fVerbose );
  if (dd == NULL) {
    return NULL;
  }
  // Abc_NtkDelete(pNtk);
  // assert( Abc_NtkIsBddLogic(pNtkNew) );
  int *array = Vec_IntArray(pPiIndex);
  int n = Vec_IntSize(pPiIndex);
  DdNode *cube = Cudd_IndicesToCube(dd, array, n);
  Cudd_Ref(cube);
  DdNode *f = (DdNode *)Abc_ObjGlobalBdd(Abc_NtkPo(pNtk, 0));
  DdNode *bFunc = Cudd_bddExistAbstract(dd, f, cube);
  Cudd_Ref(bFunc);
  Cudd_RecursiveDeref(dd, cube);
  Cudd_RecursiveDeref(dd, f);
  Abc_ObjSetGlobalBdd(Abc_NtkPo(pNtk, 0), bFunc);
  // create the new network
  // int fReverse = 0;
  // Abc_Ntk_t * pNtkNew2 = Abc_NtkFromGlobalBdds( pNtk, fReverse );
  Abc_Ntk_t *pNtkNew2 = Abc_NtkDeriveFromBdd(dd, bFunc, NULL, NULL);

  Abc_NtkFreeGlobalBdds(pNtk, 1);
  // Abc_NtkDelete(pNtk);
  if (pNtkNew2 == NULL) {
    return NULL;
  }

  return pNtkNew2;

  // make the network minimum base
  // Abc_NtkMinimumBase2( pNtkNew2 );

  int fAllNodes = 0;
  int fCleanup = 1;
  int fRecord = 0;
  // int fComplOuts= 0;
  Abc_Ntk_t *pNtkNew3 = Abc_NtkStrash(pNtkNew2, fAllNodes, fCleanup, fRecord);
  Abc_NtkFreeGlobalBdds(pNtkNew2, 1);
  Abc_NtkDelete(pNtkNew2);
  if (pNtkNew3 == NULL)
    return NULL;

  Abc_NtkDelete(pNtk);
  return pNtkNew3;
}

/**Function*************************************************************
  Synopsis    []
  Description []

  SideEffects []
  SeeAlso     []
***********************************************************************/
void Util_NtkCreateMultiPi(Abc_Ntk_t *pNtk, int n) {
  static char Buffer[1000];
  assert(pNtk);
  for (int i = 0; i < n; i++) {
    Abc_Obj_t *pPi = Abc_NtkCreatePi(pNtk);
    sprintf(Buffer, "pi%d", i);
    Abc_ObjAssignName(pPi, Buffer, NULL);
  }
}

/**Function*************************************************************
  Synopsis    []
  Description []

  SideEffects []
  SeeAlso     []
***********************************************************************/
void Util_NtkCreateMultiPiWithPrefix(Abc_Ntk_t *pNtk, int n, char *prefix) {
  static char Buffer[1000];
  for (int i = 0; i < n; i++) {
    Abc_Obj_t *pObjNew = Abc_NtkCreatePi(pNtk);
    sprintf(Buffer, "%s%d", (prefix) ? prefix : "", i);
    Abc_ObjAssignName(pObjNew, Buffer, NULL);
  }
}

/**Function*************************************************************
  Synopsis    []
  Description []

  SideEffects []
  SeeAlso     []
***********************************************************************/
void Util_NtkCreateMultiPo(Abc_Ntk_t *pNtk, int n) {
  static char Buffer[1000];
  assert(pNtk);
  for (int i = 0; i < n; i++) {
    Abc_Obj_t *pPo = Abc_NtkCreatePo(pNtk);
    sprintf(Buffer, "po%d", i);
    Abc_ObjAssignName(pPo, Buffer, NULL);
  }
}

/**Function*************************************************************
  Synopsis    []
  Description []

  SideEffects []
  SeeAlso     []
***********************************************************************/
void Util_NtkCreatePiFrom(Abc_Ntk_t *pNtkDest, Abc_Ntk_t *pNtkOri,
                          char *pSuffix) {
  assert(pNtkDest);
  assert(pNtkOri);
  int i;
  Abc_Obj_t *pObj;
  Abc_NtkForEachPi(pNtkOri, pObj, i) {
    Abc_Obj_t *pObjNew = Abc_NtkCreatePi(pNtkDest);
    Abc_ObjAssignName(pObjNew, Abc_ObjName(pObj), pSuffix);
  }
}

Abc_Obj_t* Util_NtkCreateMultiOr(Abc_Ntk_t *pNtk, Vec_Ptr_t *pVec) {
  int size = Vec_PtrSize(pVec);
  while (size > 1) {
    int half = size / 2;
    if (size % 2 == 0) {
      for (int i = 0; i < half; i++) {
        pVec->pArray[i] = (void*)Abc_AigOr((Abc_Aig_t *)pNtk->pManFunc,
                               (Abc_Obj_t*)pVec->pArray[2 * i],
                               (Abc_Obj_t*)pVec->pArray[2 * i + 1]);
      }
      size = half;
    } else {
      for (int i = 0; i < half; i++) {
        pVec->pArray[i] = (void*)Abc_AigOr((Abc_Aig_t *)pNtk->pManFunc,
                               (Abc_Obj_t*)pVec->pArray[2 * i],
                               (Abc_Obj_t*)pVec->pArray[2 * i + 1]);
      }
      pVec->pArray[half] = pVec->pArray[size - 1];
      size = half + 1;
    }
  }
  return (Abc_Obj_t*)pVec->pArray[0];
}

Abc_Obj_t* Util_NtkCreateMultiAnd(Abc_Ntk_t *pNtk, Vec_Ptr_t *pVec) {
  int size = Vec_PtrSize(pVec);
  while (size > 1) {
    int half = size / 2;
    if (size % 2 == 0) {
      for (int i = 0; i < half; i++) {
        pVec->pArray[i] = (void*)Abc_AigAnd((Abc_Aig_t *)pNtk->pManFunc,
                               (Abc_Obj_t*)pVec->pArray[2 * i],
                               (Abc_Obj_t*)pVec->pArray[2 * i + 1]);
      }
      size = half;
    } else {
      for (int i = 0; i < half; i++) {
        pVec->pArray[i] = (void*)Abc_AigAnd((Abc_Aig_t *)pNtk->pManFunc,
                               (Abc_Obj_t*)pVec->pArray[2 * i],
                               (Abc_Obj_t*)pVec->pArray[2 * i + 1]);
      }
      pVec->pArray[half] = pVec->pArray[size - 1];
      size = half + 1;
    }
  }
  return pVec->pArray[0];
}

/**Function*************************************************************
  Synopsis    []
  Description []

  SideEffects []
  SeeAlso     []
***********************************************************************/
void Util_NtkConst1SetCopy(Abc_Ntk_t *pNtk, Abc_Ntk_t *pNtk3opy) {
  Abc_ObjSetCopy(Abc_AigConst1(pNtk), Abc_AigConst1(pNtk3opy));
}

/**Function*************************************************************
  Synopsis    []
  Description []

  SideEffects []
  SeeAlso     []
***********************************************************************/
void Util_NtkPiSetCopy(Abc_Ntk_t *pNtk, Abc_Ntk_t *pNtk3opy, int start_index) {
  assert(pNtk);
  assert(pNtk3opy);
  assert(start_index + Abc_NtkPiNum(pNtk) <= Abc_NtkPiNum(pNtk3opy));
  int i;
  Abc_Obj_t *pObj;
  Abc_NtkForEachPi(pNtk, pObj, i) {
    Abc_ObjSetCopy(pObj, Abc_NtkPi(pNtk3opy, start_index + i));
  }
}

/**Function*************************************************************
  Synopsis    []
  Description [Create CountingCircuit in ntk
              out[0] is LSB]

  SideEffects []
  SeeAlso     []
***********************************************************************/
Vec_Ptr_t *Util_AigNtkCounter(Abc_Ntk_t *pNtk, Vec_Ptr_t *pIn, int index_begin,
                              int index_end) {
  assert(pNtk);
  assert(pIn);
  int nPi = index_end - index_begin;

  assert(nPi >= 0);
  if (nPi == 0) {
    Vec_Ptr_t *pOut = Vec_PtrAlloc(0);
    Vec_PtrPush(pOut, Abc_AigConst0(pNtk));
    return pOut;
  } else if (nPi == 1) {
    Vec_Ptr_t *pOut = Vec_PtrAlloc(0);
    Vec_PtrPush(pOut, Vec_PtrEntry(pIn, index_begin));
    return pOut;
  } else if (nPi == 2) {
    Vec_Ptr_t *pOut = Vec_PtrAlloc(0);
    Abc_Obj_t *pObj0 = Vec_PtrEntry(pIn, index_begin);
    Abc_Obj_t *pObj1 = Vec_PtrEntry(pIn, index_begin + 1);
    Abc_Obj_t *pObjSum = Util_AigNtkXor(pNtk, pObj0, pObj1);
    Vec_PtrPush(pOut, pObjSum);
    Abc_Obj_t *pObjCarry = Util_AigNtkAnd(pNtk, pObj0, pObj1);
    Vec_PtrPush(pOut, pObjCarry);
    return pOut;
  } else if (nPi == 3) {
    Vec_Ptr_t *pOut = Vec_PtrAlloc(0);
    Abc_Obj_t *pObj0 = Vec_PtrEntry(pIn, index_begin);
    Abc_Obj_t *pObj1 = Vec_PtrEntry(pIn, index_begin + 1);
    Abc_Obj_t *pObj2 = Vec_PtrEntry(pIn, index_begin + 2);
    Abc_Obj_t *pObjSum = Util_AigNtkAdderSum(pNtk, pObj0, pObj1, pObj2);
    Vec_PtrPush(pOut, pObjSum);
    Abc_Obj_t *pObjCarry = Util_AigNtkAdderCarry(pNtk, pObj0, pObj1, pObj2);
    Vec_PtrPush(pOut, pObjCarry);
    return pOut;
  } else {
  }
  // nPi >= 4
  int nSubPo = floor(log2(nPi));
  int nSubPi = (1 << nSubPo) - 1;
  // generate sub1
  int nSub1_index_start = index_begin;
  int nSub1_index_end = index_begin + nSubPi;
  Vec_Ptr_t *pSubOut_1 =
      Util_AigNtkCounter(pNtk, pIn, nSub1_index_start, nSub1_index_end);
  assert(Vec_PtrSize(pSubOut_1) <= nSubPo);
  // generate sub2
  int nSub2_index_start = index_begin + nSubPi;
  int nSub2_index_end = min(index_begin + 2 * nSubPi, Vec_PtrSize(pIn));
  Vec_Ptr_t *pSubOut_2 =
      Util_AigNtkCounter(pNtk, pIn, nSub2_index_start, nSub2_index_end);
  assert(Vec_PtrSize(pSubOut_2) <= nSubPo);
  // Add subout
  Vec_Ptr_t *pAddOut = Util_AigNtkAdder(pNtk, pSubOut_1, pSubOut_2);
  // memory free
  Vec_PtrFree(pSubOut_1);
  Vec_PtrFree(pSubOut_2);

  return pAddOut;
}

/**Function*************************************************************
  Synopsis    []
  Description []

  SideEffects []
  SeeAlso     []
***********************************************************************/
void Util_NtkModelAlloc(Abc_Ntk_t *pNtk) {
  if (!pNtk->pModel) {
    pNtk->pModel = ABC_CALLOC(int, Abc_NtkPiNum(pNtk) + 1);
  }
}

void Util_SetCiModelAs0(Abc_Ntk_t *pNtk) {
  Abc_Obj_t *pNode;
  int i;
  Abc_NtkForEachPi(pNtk, pNode, i) pNtk->pModel[i] = 0;
}

void Util_SetCiModelAs1(Abc_Ntk_t *pNtk) {
  Abc_Obj_t *pNode;
  int i;
  Abc_NtkForEachPi(pNtk, pNode, i) pNtk->pModel[i] = (~0);
}

/**Function*************************************************************
  Synopsis    []
  Description [ Need to set pNtk->pModel[] before function called.
                Only return the simulated value of the 0-th po. ]

  SideEffects []
  SeeAlso     []
***********************************************************************/
static inline unsigned int Abc_ObjFaninC0_S(Abc_Obj_t *pObj) {
  return Abc_ObjFaninC0(pObj) ? (~0) : 0;
}
static inline unsigned int Abc_ObjFaninC1_S(Abc_Obj_t *pObj) {
  return Abc_ObjFaninC1(pObj) ? (~0) : 0;
}
unsigned int Util_NtkSimulate(Abc_Ntk_t *pNtk) {
  assert(Abc_NtkIsStrash(pNtk));
  Abc_Obj_t *pNode;
  int i;
  Abc_AigConst1(pNtk)->iTemp = (~0);
  Abc_NtkForEachCi(pNtk, pNode, i) pNode->iTemp = pNtk->pModel[i];
  Abc_NtkForEachNode(pNtk, pNode, i) {
    unsigned int v0 = Abc_ObjFanin0(pNode)->iTemp ^ Abc_ObjFaninC0_S(pNode);
    unsigned int v1 = Abc_ObjFanin1(pNode)->iTemp ^ Abc_ObjFaninC1_S(pNode);
    pNode->iTemp = v0 & v1;
  }
  Abc_Obj_t *pPo = Abc_NtkPo(pNtk, 0);
  Abc_NtkForEachPo(pNtk, pPo, i) {
    pPo->iTemp = Abc_ObjFanin0(pPo)->iTemp ^ Abc_ObjFaninC0_S(pPo);
  }

  unsigned int value = Abc_NtkPo(pNtk, 0)->iTemp;
  return value;
}

/**Function*************************************************************
  Synopsis    []
  Description [ Assume that pNtk is sorted function.
                At least f(000) should be 0.
                EX:
                000 0
                001 0
                010 0  <- max input where output is 0
                011 1
                100 1
                101 1
                110 1
                111 1
                pNtk->pModel will be 010
              ]

  SideEffects [pNtk->pModel will be changed.]
  SeeAlso     []
***********************************************************************/
void Util_MaxInputPatternWithOuputAs0(Abc_Ntk_t *pNtk, Vec_Int_t *pCaredPi) {
  assert(pCaredPi);
  Util_SetCiModelAs0(pNtk);
  int index;
  int entry;
  Vec_IntForEachEntry(pCaredPi, entry, index) {
    pNtk->pModel[entry] = 1;
    const int simulated_result = Util_NtkSimulate(pNtk) & (int)01;
    if (simulated_result != 0) {
      pNtk->pModel[entry] = 0;
    }
  }
}

/**Function*************************************************************
  Synopsis    []
  Description []

  SideEffects []
  SeeAlso     []
***********************************************************************/
static void printMinterm(Abc_Ntk_t *pNtk, unsigned int simulatedResult) {
  Abc_Obj_t *pNode;
  int i;
  Abc_NtkForEachCi(pNtk, pNode, i) {
    Abc_Print(1, "%d", (pNtk->pModel[i] != 0));
  }
  Abc_Print(1, "  %d\n", (simulatedResult != 0));
}

static void printSimulatedResult(Abc_Ntk_t *pNtk) {
  Abc_Obj_t *pNode;
  int i;
  Abc_NtkForEachCi(pNtk, pNode, i) {
    Abc_Print(1, "%d", (pNtk->pModel[i] != 0));
  }
  Abc_Print(1, "  ");
  Abc_NtkForEachCo(pNtk, pNode, i) { Abc_Print(1, "%d", (pNode->iTemp != 0)); }
  Abc_Print(1, "\n");
}

#define Util_NtkForEachCiRevere(pNtk, pCi, i)                                  \
  for (i = Abc_NtkCiNum(pNtk) - 1;                                             \
       ((i >= 0)) && (((pCi) = Abc_NtkCi(pNtk, i)), 1); i--)

static int increaseModelValue(Abc_Ntk_t *pNtk) {
  Abc_Obj_t *pNode;
  int i;
  int isOverflow = 1;
  Util_NtkForEachCiRevere(pNtk, pNode, i) {
    if (pNtk->pModel[i] == 0) {
      pNtk->pModel[i] = (~0);
      isOverflow = 0;
      break;
    }
    if (pNtk->pModel[i] != 0) {
      pNtk->pModel[i] = 0;
    }
  }
  return isOverflow;
}

void Util_PrintTruthtable(Abc_Ntk_t *pNtk) {
  ABC_FREE(pNtk->pModel);
  pNtk->pModel = ABC_CALLOC(int, Abc_NtkPiNum(pNtk) + 1);
  Abc_Obj_t *pNode;
  int i;
  Abc_NtkForEachCi(pNtk, pNode, i) pNtk->pModel[i] = 0;

  // unsigned long count = 0;
  while (1) {
    unsigned int simulatedResult = Util_NtkSimulate(pNtk);
    printSimulatedResult(pNtk);
    // if (simulatedResult)
    //   count++;
    int isOverflow = increaseModelValue(pNtk);
    if (isOverflow) {
      // Abc_Print( 1, " # of 1: %d\n", count );
      return;
    }
  }
}

void Util_NtkResub(Abc_Ntk_t *pNtk, int nCutsMax, int nNodesMax) {
  extern int Abc_NtkResubstitute( Abc_Ntk_t * pNtk, int nCutsMax, int nNodesMax, int nLevelsOdc, int fUpdateLevel, int fVerbose, int fVeryVerbose );
  assert(nCutsMax > 0);
  assert(nNodesMax > 0);
  int nLevelsOdc = 0;
  int fUpdateLevel = 1;
  int fVerbose = 0;
  int fVeryVerbose = 0;
  Abc_NtkResubstitute(pNtk, nCutsMax, nNodesMax, nLevelsOdc, fUpdateLevel,
                      fVerbose, fVeryVerbose);
}


/**Function*************************************************************
  Synopsis    []
  Description []

  SideEffects []
  SeeAlso     []
***********************************************************************/
Abc_Ntk_t *Util_NtkBalance(Abc_Ntk_t *pNtk, int fDelete) {
  int fDuplicate = 0;
  int fSelective = 0;
  int fUpdateLevel = 1;
  // int fExor        = 0;
  // int fVerbose     = 0;
  Abc_Ntk_t *pNtkNew =
      Abc_NtkBalance(pNtk, fDuplicate, fSelective, fUpdateLevel);
  if (pNtkNew && fDelete)
    Abc_NtkDelete(pNtk);
  return pNtkNew;
}

Abc_Ntk_t *Util_NtkResyn2(Abc_Ntk_t *pNtk, int fDelete) {
  // b; rw; rf; b; rw; rwz; b; rfz; rwz; b
  Abc_Ntk_t *pNtkNew = Util_NtkBalance(pNtk, fDelete);
  Util_NtkRewrite(pNtkNew);
  Util_NtkRefactor(pNtkNew);
  pNtkNew = Util_NtkBalance(pNtkNew, 1);
  Util_NtkRewrite(pNtkNew);
  Util_NtkRewriteZ(pNtkNew);
  pNtkNew = Util_NtkBalance(pNtkNew, 1);
  Util_NtkRefactorZ(pNtkNew);
  Util_NtkRewriteZ(pNtkNew);
  pNtkNew = Util_NtkBalance(pNtkNew, 1);
  return pNtkNew;
}

Abc_Ntk_t *Util_NtkResyn2rs(Abc_Ntk_t *pNtk, int fDelete) {
  // b; rs -K 6; rw; rs -K 6 -N 2; rf; rs -K 8; b; rs -K 8 -N 2;
  Abc_Ntk_t *pNtkNew = Util_NtkBalance(pNtk, fDelete);
  Util_NtkResub(pNtkNew, 6, 1);
  Util_NtkRewrite(pNtkNew);
  Util_NtkResub(pNtkNew, 6, 2);
  Util_NtkRefactor(pNtkNew);
  Util_NtkResub(pNtkNew, 8, 1);
  pNtkNew = Util_NtkBalance(pNtkNew, fDelete);
  Util_NtkResub(pNtkNew, 8, 2);
  // rw; rs -K 10; rwz; rs -K 10 -N 2; b; rs -K 12; rfz; rs -K 12 -N 2; rwz; b
  Util_NtkRewrite(pNtkNew);
  Util_NtkResub(pNtkNew, 10, 1);
  Util_NtkRewriteZ(pNtkNew);
  Util_NtkResub(pNtkNew, 10, 2);
  pNtkNew = Util_NtkBalance(pNtkNew, fDelete);
  Util_NtkResub(pNtkNew, 12, 1);
  Util_NtkRefactorZ(pNtkNew);
  Util_NtkResub(pNtkNew, 12, 2);
  Util_NtkRewriteZ(pNtkNew);
  pNtkNew = Util_NtkBalance(pNtkNew, fDelete);
  return pNtkNew;
}

Abc_Ntk_t *Util_NtkDc2(Abc_Ntk_t *pNtk, int fDelete) {
  extern Abc_Ntk_t *Abc_NtkDC2(Abc_Ntk_t * pNtk, int fBalance, int fUpdateLevel,
                               int fFanout, int fPower, int fVerbose);
  int fBalance = 0;
  int fVerbose = 0;
  int fUpdateLevel = 0;
  int fFanout = 1;
  int fPower = 0;
  Abc_Ntk_t *pNtkNew =
      Abc_NtkDC2(pNtk, fBalance, fUpdateLevel, fFanout, fPower, fVerbose);
  if (pNtkNew && fDelete)
    Abc_NtkDelete(pNtk);
  return pNtkNew;
}

Abc_Ntk_t *Util_NtkDrwsat(Abc_Ntk_t *pNtk, int fDelete) {
  extern Abc_Ntk_t *Abc_NtkDrwsat(Abc_Ntk_t * pNtk, int fBalance, int fVerbose);
  int fBalance = 0;
  int fVerbose = 0;
  // int fUpdateLevel = 0;
  // int fFanout      = 1;
  // int fPower       = 0;
  Abc_Ntk_t *pNtkNew = Abc_NtkDrwsat(pNtk, fBalance, fVerbose);
  if (pNtkNew && fDelete)
    Abc_NtkDelete(pNtk);
  return pNtkNew;
}

Abc_Ntk_t *Util_NtkIFraig(Abc_Ntk_t *pNtk, int fDelete) {
  extern Abc_Ntk_t *Abc_NtkIvyFraig(Abc_Ntk_t * pNtk, int nConfLimit,
                                    int fDoSparse, int fProve, int fTransfer,
                                    int fVerbose);
  // int nPartSize    = 0;
  // int nLevelMax    = 0;
  int nConfLimit = 100;
  int fDoSparse = 0;
  int fProve = 0;
  int fVerbose = 0;
  Abc_Ntk_t *pNtkNew =
      Abc_NtkIvyFraig(pNtk, nConfLimit, fDoSparse, fProve, 0, fVerbose);
  if (pNtkNew && fDelete)
    Abc_NtkDelete(pNtk);
  return pNtkNew;
}

Abc_Ntk_t *Util_NtkDFraig(Abc_Ntk_t *pNtk, int fDelete, int fDoSparse) {
  extern Abc_Ntk_t *Abc_NtkDarFraig(
      Abc_Ntk_t * pNtk, int nConfLimit, int fDoSparse, int fProve,
      int fTransfer, int fSpeculate, int fChoicing, int fVerbose);
  int nConfLimit = 100;
  // int fDoSparse    = 0;
  int fProve = 0;
  int fSpeculate = 0;
  int fChoicing = 0;
  int fVerbose = 0;
  Abc_Ntk_t *pNtkNew = Abc_NtkDarFraig(pNtk, nConfLimit, fDoSparse, fProve, 0,
                                       fSpeculate, fChoicing, fVerbose);
  if (pNtkNew && fDelete)
    Abc_NtkDelete(pNtk);
  return pNtkNew;
}

Abc_Ntk_t *Util_NtkGiaSweep(Abc_Ntk_t *pNtk, int fDelete) {
  extern Aig_Man_t *Abc_NtkToDar(Abc_Ntk_t * pNtk, int fExors, int fRegisters);
  extern Abc_Ntk_t *Abc_NtkFromAigPhase(Aig_Man_t * pMan);
  Abc_Ntk_t *pStrash;
  Aig_Man_t *pAig;
  Gia_Man_t *pGia, *pTemp;
  Dch_Pars_t Pars, *pPars = &Pars;
  Dch_ManSetDefaultParams(pPars);
  // pPars->nBTLimit = 500;
  // pPars->nSatVarMax = 2500;
  char *pInits;
  int fVerbose = 0;
  pStrash = Abc_NtkStrash(pNtk, 0, 1, 0);
  pAig = Abc_NtkToDar(pStrash, 0, 0);
  Abc_NtkDelete(pStrash);
  pGia = Gia_ManFromAig(pAig);
  Aig_ManStop(pAig);
  // perform undc/zero
  pInits = Abc_NtkCollectLatchValuesStr(pNtk);
  pGia = Gia_ManDupZeroUndc(pTemp = pGia, pInits, 0, 0, fVerbose);
  Gia_ManStop(pTemp);
  // pTemp = Cec_ManSatSweeping( pGia, pPars, 0 );
  pTemp = Gia_ManFraigSweepSimple(pGia, pPars);
  ABC_FREE(pInits);
  pAig = Gia_ManToAig(pTemp, 0);
  Gia_ManStop(pTemp);
  Abc_Ntk_t *pNtkNew = Abc_NtkFromAigPhase(pAig);
  pNtkNew->pName = Extra_UtilStrsav(pAig->pName);
  Aig_ManStop(pAig);
  if (pNtkNew && fDelete) {
    Abc_NtkDelete(pNtk);
  }
  return pNtkNew;
}

Abc_Ntk_t *Util_NtkGiaFraig(Abc_Ntk_t *pNtk, int fDelete) {
  extern void Cec4_ManSetParams(Cec_ParFra_t * pPars);
  extern Aig_Man_t *Abc_NtkToDar(Abc_Ntk_t * pNtk, int fExors, int fRegisters);
  extern Abc_Ntk_t *Abc_NtkFromAigPhase(Aig_Man_t * pMan);
  extern Gia_Man_t *Cec2_ManSimulateTest(Gia_Man_t * p, Cec_ParFra_t * pPars);
  extern Gia_Man_t *Cec3_ManSimulateTest(Gia_Man_t * p, Cec_ParFra_t * pPars);
  extern Gia_Man_t *Cec4_ManSimulateTest(Gia_Man_t * p, Cec_ParFra_t * pPars);
  Abc_Ntk_t *pStrash;
  Aig_Man_t *pAig;
  Gia_Man_t *pGia, *pTemp;
  Cec_ParFra_t ParsFra, *pPars = &ParsFra;
  Cec4_ManSetParams(pPars);
  char *pInits;
  int fVerbose = 0;
  pStrash = Abc_NtkStrash(pNtk, 0, 1, 0);
  pAig = Abc_NtkToDar(pStrash, 0, 0);
  Abc_NtkDelete(pStrash);
  pGia = Gia_ManFromAig(pAig);
  Aig_ManStop(pAig);
  // perform undc/zero
  pInits = Abc_NtkCollectLatchValuesStr(pNtk);
  pGia = Gia_ManDupZeroUndc(pTemp = pGia, pInits, 0, 0, fVerbose);
  Gia_ManStop(pTemp);
  pTemp = Cec_ManSatSweeping(pGia, pPars, 0);
  // pTemp = Cec4_ManSimulateTest( pGia, pPars );
  ABC_FREE(pInits);
  pAig = Gia_ManToAig(pTemp, 0);
  Gia_ManStop(pTemp);
  Abc_Ntk_t *pNtkNew = Abc_NtkFromAigPhase(pAig);
  pNtkNew->pName = Extra_UtilStrsav(pAig->pName);
  Aig_ManStop(pAig);
  if (pNtkNew && fDelete) {
    Abc_NtkDelete(pNtk);
  }
  return pNtkNew;
}

Abc_Ntk_t *Util_NtkCollapse(Abc_Ntk_t *pNtk, int fBddSizeMax, int fDelete) {
  int fVerbose = 0;
  int fReorder = 1;
  int fReverse = 0;
  int fDumpOrder = 0;
  int fDualRail = 0;
  Abc_Ntk_t *pNtkNew = Abc_NtkCollapse(pNtk, fBddSizeMax, fDualRail, fReorder,
                                       fReverse, fDumpOrder, fVerbose);
  if (pNtkNew && fDelete)
    Abc_NtkDelete(pNtk);
  return pNtkNew;
}

Abc_Ntk_t *Util_NtkStrash(Abc_Ntk_t *pNtk, int fDelete) {
  int fAllNodes = 0;
  int fCleanup = 1;
  int fRecord = 0;
  // int fComplOuts= 0;
  Abc_Ntk_t *pNtkNew = Abc_NtkStrash(pNtk, fAllNodes, fCleanup, fRecord);
  if (pNtkNew && fDelete)
    Abc_NtkDelete(pNtk);
  return pNtkNew;
}

Abc_Ntk_t *Util_NtkInverse(Abc_Ntk_t *pNtk, int fDelete) {
  Abc_Ntk_t *pNtkNew = Abc_NtkDup(pNtk);
  Abc_Obj_t *pObj = Abc_NtkPo(pNtkNew, 0);
  Abc_ObjXorFaninC(pObj, 0);
  if (pNtkNew && fDelete)
    Abc_NtkDelete(pNtk);
  return pNtkNew;
}

Abc_Ntk_t *Util_NtkInversePi(Abc_Ntk_t *pNtk, int index, int fDelete) {
  Abc_Ntk_t *pNtkNew = Abc_NtkStartFrom(pNtk, pNtk->ntkType, pNtk->ntkFunc);
  Abc_Obj_t *pObj;
  int i;
  Abc_NtkForEachPi(pNtk, pObj, i) {
    if (index == i) {
      pObj->pCopy = Abc_ObjNot(Abc_NtkPi(pNtkNew, index));
    } else {
      pObj->pCopy = Abc_NtkPi(pNtkNew, i);
    }
  }
  Abc_NtkForEachCo(pNtk, pObj, i) { pObj->pCopy = Abc_NtkPo(pNtkNew, i); }
  Abc_AigForEachAnd(pNtk, pObj, i) {
    pObj->pCopy = Abc_AigAnd((Abc_Aig_t *)pNtkNew->pManFunc,
                             Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj));
  }
  // relink the CO nodes
  Abc_NtkForEachCo(pNtk, pObj, i) {
    Abc_ObjAddFanin(pObj->pCopy, Abc_ObjChild0Copy(pObj));
  }
  return pNtkNew;
}

void Util_NtkRewrite(Abc_Ntk_t *pNtk) {
  int fUpdateLevel = 1;
  // int fPrecompute  = 0;
  int fUseZeros = 0;
  int fVerbose = 0;
  int fVeryVerbose = 0;
  int fPlaceEnable = 0;
  Abc_NtkRewrite(pNtk, fUpdateLevel, fUseZeros, fVerbose, fVeryVerbose,
                 fPlaceEnable);
}

void Util_NtkRewriteZ(Abc_Ntk_t *pNtk) {
  int fUpdateLevel = 1;
  // int fPrecompute  = 0;
  int fUseZeros = 1;
  int fVerbose = 0;
  int fVeryVerbose = 0;
  int fPlaceEnable = 0;
  Abc_NtkRewrite(pNtk, fUpdateLevel, fUseZeros, fVerbose, fVeryVerbose,
                 fPlaceEnable);
}

void Util_NtkRefactor(Abc_Ntk_t *pNtk) {
  int nNodeSizeMax = 10;
  int nConeSizeMax = 16;
  int fUpdateLevel = 1;
  int fUseZeros = 0;
  int fUseDcs = 0;
  int fVerbose = 0;
  Abc_NtkRefactor(pNtk, nNodeSizeMax, nConeSizeMax, fUpdateLevel, fUseZeros,
                  fUseDcs, fVerbose);
}

void Util_NtkRefactorZ(Abc_Ntk_t *pNtk) {
  int nNodeSizeMax = 10;
  int nConeSizeMax = 16;
  int fUpdateLevel = 1;
  int fUseZeros = 1;
  int fUseDcs = 0;
  int fVerbose = 0;
  Abc_NtkRefactor(pNtk, nNodeSizeMax, nConeSizeMax, fUpdateLevel, fUseZeros,
                  fUseDcs, fVerbose);
}

/**Function*************************************************************
  Synopsis    []
  Description []

  SideEffects []
  SeeAlso     []
***********************************************************************/
Abc_Obj_t *Util_NtkCreatePoWithName(Abc_Ntk_t *pNtk, char *PoName,
                                    char *pSuffix) {
  Abc_Obj_t *pPo = Abc_NtkCreatePo(pNtk);
  Abc_ObjAssignName(pPo, PoName, pSuffix);
  return pPo;
}

/**Function*************************************************************
  Synopsis    []
  Description [Used on Strash network only.]

  SideEffects [Precondition: input with fanout should has valid pCopy
reference.] SeeAlso     []
***********************************************************************/
Abc_Obj_t *Util_NtkAppend(Abc_Ntk_t *pNtkDes, Abc_Ntk_t *pNtkSource) {
  Abc_Obj_t *pObj;
  int i;
  Abc_AigForEachAnd(pNtkSource, pObj, i) pObj->pCopy =
      Abc_AigAnd((Abc_Aig_t *)pNtkDes->pManFunc, Abc_ObjChild0Copy(pObj),
                 Abc_ObjChild1Copy(pObj));

  pObj = Abc_ObjChild0Copy(Abc_NtkPo(pNtkSource, 0));
  return pObj;
}

/**Function*************************************************************
  Synopsis    []
  Description []

  SideEffects []
  SeeAlso     []
***********************************************************************/
Abc_Obj_t *Util_NtkAppendWithPiOrder(Abc_Ntk_t *pNtkDes, Abc_Ntk_t *pNtkSource,
                                     int start_index) {
  Util_NtkConst1SetCopy(pNtkSource, pNtkDes);
  Util_NtkPiSetCopy(pNtkSource, pNtkDes, start_index);
  Abc_Obj_t *pObjOut = Util_NtkAppend(pNtkDes, pNtkSource);
  return pObjOut;
}

/**Function*************************************************************
  Synopsis    []
  Description []

  SideEffects []
  SeeAlso     []
***********************************************************************/
Abc_Obj_t *Util_AigNtkMaj(Abc_Ntk_t *pNtk, Abc_Obj_t *p0, Abc_Obj_t *p1,
                          Abc_Obj_t *p2) {
  assert(Abc_NtkIsStrash(pNtk));
  Abc_Aig_t *pMan = (Abc_Aig_t *)pNtk->pManFunc;
  Abc_Obj_t *pObj = Util_AigMaj(pMan, p0, p1, p2);
  return pObj;
}

Abc_Obj_t *Util_AigNtkAnd(Abc_Ntk_t *pNtk, Abc_Obj_t *p0, Abc_Obj_t *p1) {
  assert(Abc_NtkIsStrash(pNtk));
  Abc_Aig_t *pMan = (Abc_Aig_t *)pNtk->pManFunc;
  Abc_Obj_t *pObj = Abc_AigAnd(pMan, p0, p1);
  return pObj;
}

Abc_Obj_t *Util_AigNtkOr(Abc_Ntk_t *pNtk, Abc_Obj_t *p0, Abc_Obj_t *p1) {
  assert(Abc_NtkIsStrash(pNtk));
  Abc_Aig_t *pMan = (Abc_Aig_t *)pNtk->pManFunc;
  Abc_Obj_t *pObj = Abc_AigOr(pMan, p0, p1);
  return pObj;
}

Abc_Obj_t *Util_AigNtkXor(Abc_Ntk_t *pNtk, Abc_Obj_t *p0, Abc_Obj_t *p1) {
  assert(Abc_NtkIsStrash(pNtk));
  Abc_Aig_t *pMan = (Abc_Aig_t *)pNtk->pManFunc;
  Abc_Obj_t *pObj = Abc_AigXor(pMan, p0, p1);
  return pObj;
}

Abc_Obj_t *Util_AigNtkEqual(Abc_Ntk_t *pNtk, Abc_Obj_t *p0, Abc_Obj_t *p1) {
  assert(Abc_NtkIsStrash(pNtk));
  Abc_Aig_t *pMan = (Abc_Aig_t *)pNtk->pManFunc;
  Abc_Obj_t *pObj = Abc_ObjNot(Abc_AigXor(pMan, p0, p1));
  return pObj;
}

Abc_Obj_t *Util_AigNtkImply(Abc_Ntk_t *pNtk, Abc_Obj_t *p0, Abc_Obj_t *p1) {
  assert(Abc_NtkIsStrash(pNtk));
  Abc_Aig_t *pMan = (Abc_Aig_t *)pNtk->pManFunc;
  Abc_Obj_t *pObj = Abc_AigOr(pMan, Abc_ObjNot(p0), p1);
  return pObj;
}

Abc_Obj_t *Util_AigNtkAdderSum(Abc_Ntk_t *pNtk, Abc_Obj_t *p0, Abc_Obj_t *p1,
                               Abc_Obj_t *c) {
  assert(Abc_NtkIsStrash(pNtk));
  Abc_Obj_t *pObj = Util_AigNtkXor(pNtk, p0, p1);
  pObj = Util_AigNtkXor(pNtk, pObj, c);
  return pObj;
}

Abc_Obj_t *Util_AigNtkAdderCarry(Abc_Ntk_t *pNtk, Abc_Obj_t *p0, Abc_Obj_t *p1,
                                 Abc_Obj_t *c) {
  assert(Abc_NtkIsStrash(pNtk));
  Abc_Obj_t *pObj_1 = Util_AigNtkAnd(pNtk, p0, p1);
  Abc_Obj_t *pObj_2 = Util_AigNtkAnd(pNtk, p1, c);
  Abc_Obj_t *pObj_3 = Util_AigNtkAnd(pNtk, p0, c);
  Abc_Obj_t *pObj = Util_AigNtkOr(pNtk, pObj_1, pObj_2);
  pObj = Util_AigNtkOr(pNtk, pObj, pObj_3);
  return pObj;
}

Vec_Ptr_t *Util_AigNtkAdder(Abc_Ntk_t *pNtk, Vec_Ptr_t *p1, Vec_Ptr_t *p2) {
  Vec_Ptr_t *pOut = Vec_PtrAlloc(0);
  Abc_Obj_t *pObjCarry = Abc_AigConst0(pNtk);
  for (int i = 0; i < Abc_MaxInt(Vec_PtrSize(p1), Vec_PtrSize(p2)); i++) {
    Abc_Obj_t *pObjIn_1 =
        (i < Vec_PtrSize(p1)) ? Abc_AigConst0(pNtk) : Vec_PtrEntry(p1, i);
    Abc_Obj_t *pObjIn_2 =
        (i < Vec_PtrSize(p2)) ? Abc_AigConst0(pNtk) : Vec_PtrEntry(p2, i);

    Abc_Obj_t *pObjSum =
        Util_AigNtkAdderSum(pNtk, pObjIn_1, pObjIn_2, pObjCarry);
    Vec_PtrPush(pOut, pObjSum);
    pObjCarry = Util_AigNtkAdderCarry(pNtk, pObjIn_1, pObjIn_2, pObjCarry);
  }
  Vec_PtrPush(pOut, pObjCarry);
  return pOut;
}

/**Function*************************************************************
  Synopsis    []
  Description []

  SideEffects []
  SeeAlso     []
***********************************************************************/
Abc_Obj_t *Abc_AigConst0(Abc_Ntk_t *pNtk) {
  return Abc_ObjNot(Abc_AigConst1(pNtk));
}

/**Function*************************************************************
  Synopsis    []
  Description []

  SideEffects []
  SeeAlso     []
***********************************************************************/
Abc_Obj_t *Util_AigMaj(Abc_Aig_t *pMan, Abc_Obj_t *p0, Abc_Obj_t *p1,
                       Abc_Obj_t *p2) {
  Abc_Obj_t *pObjAnd1 = Abc_AigAnd(pMan, p0, p1);
  Abc_Obj_t *pObjAnd2 = Abc_AigAnd(pMan, p0, p2);
  Abc_Obj_t *pObjAnd3 = Abc_AigAnd(pMan, p1, p2);

  Abc_Obj_t *pObjOr1 = Abc_AigOr(pMan, pObjAnd1, pObjAnd2);
  Abc_Obj_t *pObjOr2 = Abc_AigOr(pMan, pObjOr1, pObjAnd3);
  return pObjOr2;
}

/**Function*************************************************************
  Synopsis    []
  Description []

  SideEffects []
  SeeAlso     []
***********************************************************************/
static char *OBJTYPE[] = {
    "ABC_OBJ_NONE",     //  0:  unknown
    "ABC_OBJ_CONST1",   //  1:  constant 1 node (AIG only)
    "ABC_OBJ_PI",       //  2:  primary input terminal
    "ABC_OBJ_PO",       //  3:  primary output terminal
    "ABC_OBJ_BI",       //  4:  box input terminal
    "ABC_OBJ_BO",       //  5:  box output terminal
    "ABC_OBJ_NET",      //  6:  net
    "ABC_OBJ_NODE",     //  7:  node
    "ABC_OBJ_LATCH",    //  8:  latch
    "ABC_OBJ_WHITEBOX", //  9:  box with known contents
    "ABC_OBJ_BLACKBOX", // 10:  box with unknown contents
    "ABC_OBJ_NUMBER"    // 11:  unused
};

void Util_ObjPrint(Abc_Obj_t *pObj) {
  Abc_Obj_t *pObjReg = Abc_ObjRegular(pObj);
  Abc_Print(1, "%c%3d: %s\n", Abc_ObjIsComplement(pObj) ? '-' : ' ',
            Abc_ObjId(pObjReg), OBJTYPE[Abc_ObjType(pObjReg)]);
  if (Abc_ObjFaninNum(pObjReg)) {
    Abc_Print(1, "    fanin:");
    Abc_Obj_t *pFanin;
    int i;
    Abc_ObjForEachFanin(pObjReg, pFanin, i) {
      Abc_Print(1, " %c%d", (i < 2 && Abc_ObjFaninC(pObjReg, i)) ? '-' : ' ',
                Abc_ObjFaninId(pObjReg, i));
    }
    Abc_Print(1, "\n");
  }
}

/**Function*************************************************************
  Synopsis    []
  Description []

  SideEffects []
  SeeAlso     []
***********************************************************************/
void Util_CollectPi(Vec_Ptr_t *vPi, Abc_Ntk_t *pNtk, int index_start,
                    int index_end) {
  assert(vPi);
  assert(pNtk);
  assert(index_start <= Abc_NtkPiNum(pNtk));
  assert(index_end <= Abc_NtkPiNum(pNtk));
  int i;
  for (i = index_start; i < index_end; ++i) {
    Vec_PtrPush(vPi, Abc_NtkPi(pNtk, i));
  }
}

/**Function*************************************************************
  Synopsis    []
  Description [Return value  0: SAT
                             1: UNSAT
                            -1: UNDECIDED ]

  SideEffects []
  SeeAlso     []
***********************************************************************/
int Util_NtkSat(Abc_Ntk_t *pNtk, int fVerbose) {
  extern int Abc_NtkDSat(Abc_Ntk_t * pNtk, ABC_INT64_T nConfLimit,
                         ABC_INT64_T nInsLimit, int nLearnedStart,
                         int nLearnedDelta, int nLearnedPerce, int fAlignPol,
                         int fAndOuts, int fNewSolver, int fVerbose);
  assert(pNtk);
  int nConfLimit = 0;
  int nInsLimit = 0;
  int nLearnedStart = 0;
  int nLearnedDelta = 0;
  int nLearnedPerce = 0;
  int fAlignPol = 0;
  int fAndOuts = 0;
  int fNewSolver = 0;
  // int RetValue = Abc_NtkMiterSat( pNtkMiter, (ABC_INT64_T)nConfLimit,
  // (ABC_INT64_T)nInsLimit, fVerbose, NULL, NULL );
  if (fVerbose) {
    printf("#PI : %d\n", Abc_NtkPiNum(pNtk));
    printf("#PO : %d\n", Abc_NtkPoNum(pNtk));
    printf("#AND: %d\n", Abc_NtkNodeNum(pNtk));
  }
  int RetValue = Abc_NtkDSat(
      pNtk, (ABC_INT64_T)nConfLimit, (ABC_INT64_T)nInsLimit, nLearnedStart,
      nLearnedDelta, nLearnedPerce, fAlignPol, fAndOuts, fNewSolver, fVerbose);

  return RetValue;
}

/**Function*************************************************************
  Synopsis    []
  Description [Convert RetValue of SAT to bool]

  SideEffects []
  SeeAlso     []
***********************************************************************/
int Util_SatResultToBool(int SatResult) {
  if (SatResult == -1)
    return -1;
  else if (SatResult == 0)
    return 1;
  else if (SatResult == 1)
    return 0;
  else
    return SatResult;
}

/**Function*************************************************************
  Synopsis    []
  Description [Return 1: Equal
                      0: Non-equal
                     -1: UNDECIDED ]

  SideEffects []
  SeeAlso     []
***********************************************************************/
int Util_NtkIsEqual(Abc_Ntk_t *pNtk1, Abc_Ntk_t *pNtk2) {
  assert(pNtk1);
  assert(pNtk2);
  assert(Abc_NtkIsStrash(pNtk1));
  assert(Abc_NtkIsStrash(pNtk2));
  assert(Abc_NtkPiNum(pNtk1) == Abc_NtkPiNum(pNtk2));
  assert(Abc_NtkPoNum(pNtk1) == 1);
  assert(Abc_NtkPoNum(pNtk2) == 1);
  Abc_Ntk_t *pNtkMiter;
  Abc_Obj_t *pObj;
  Abc_Obj_t *pObjNew;
  char Buffer[1000];
  int i;

  // Allocate network
  pNtkMiter = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
  sprintf(Buffer, "%s_miter", pNtk1->pName);
  pNtkMiter->pName = Extra_UtilStrsav(Buffer);
  Abc_AigConst1(pNtk1)->pCopy = Abc_AigConst1(pNtkMiter);
  Abc_AigConst1(pNtk2)->pCopy = Abc_AigConst1(pNtkMiter);

  // Create PI
  Vec_Ptr_t *vI = Vec_PtrAlloc(0); // input

  Abc_NtkForEachPi(pNtk1, pObj, i) {
    pObjNew = Abc_NtkCreatePi(pNtkMiter);
    Abc_ObjAssignName(pObjNew, Abc_ObjName(pObj), NULL);
    Vec_PtrPush(vI, pObjNew);
  }

  // Create PO
  pObjNew = Abc_NtkCreatePo(pNtkMiter);
  Abc_ObjAssignName(pObjNew, "miter", NULL);

  // copy Ntk1
  Abc_NtkForEachPi(pNtk1, pObj, i) { pObj->pCopy = Vec_PtrEntry(vI, i); }
  Abc_Obj_t *pObjFirst = Util_NtkAppend(pNtkMiter, pNtk1);

  // copy -Ntk2
  Abc_NtkForEachPi(pNtk2, pObj, i) { pObj->pCopy = Vec_PtrEntry(vI, i); }
  Abc_Obj_t *pObjSecond = Util_NtkAppend(pNtkMiter, pNtk2);

  // XOR copy
  pObjNew = Abc_AigXor((Abc_Aig_t *)pNtkMiter->pManFunc, pObjFirst, pObjSecond);

  // connect PO
  Abc_ObjAddFanin(Abc_NtkPo(pNtkMiter, 0), pObjNew);

  // SAT
  int fVerbose = 0;
  int RetValue = Util_NtkSat(pNtkMiter, fVerbose);

  // memory release
  Vec_PtrFree(vI);
  Abc_NtkDelete(pNtkMiter);

  return RetValue;
}

/**Function*************************************************************
  Synopsis    []
  Description []

  SideEffects []
  SeeAlso     []
***********************************************************************/
int sat_solver_add_dual_conditional_equal(sat_solver *pSat, int iVar, int iVar2,
                                          int iVarCond1, int iVarCond2) {
  lit Lits[4];
  int Cid;
  assert(iVar >= 0);
  assert(iVar2 >= 0);
  assert(iVarCond1 >= 0);
  assert(iVarCond2 >= 0);

  Lits[0] = toLitCond(iVarCond1, 1);
  Lits[1] = toLitCond(iVarCond2, 1);
  Lits[2] = toLitCond(iVar, 1);
  Lits[3] = toLitCond(iVar2, 0);
  Cid = sat_solver_addclause(pSat, Lits, Lits + 3);
  assert(Cid);

  Lits[0] = toLitCond(iVarCond1, 1);
  Lits[1] = toLitCond(iVarCond2, 1);
  Lits[2] = toLitCond(iVar, 0);
  Lits[3] = toLitCond(iVar2, 1);
  Cid = sat_solver_addclause(pSat, Lits, Lits + 3);
  assert(Cid);

  return 2;
}

/**Function*************************************************************
  Synopsis    []
  Description []

  SideEffects []
  SeeAlso     []
***********************************************************************/
int sat_solver_add_conditional_equal(sat_solver *pSat, int iVar, int iVar2,
                                     int iVarCond) {
  lit Lits[3];
  int Cid;
  assert(iVar >= 0);
  assert(iVar2 >= 0);
  assert(iVarCond >= 0);

  Lits[0] = toLitCond(iVarCond, 1);
  Lits[1] = toLitCond(iVar, 1);
  Lits[2] = toLitCond(iVar2, 0);
  Cid = sat_solver_addclause(pSat, Lits, Lits + 3);
  assert(Cid);

  Lits[0] = toLitCond(iVarCond, 1);
  Lits[1] = toLitCond(iVar, 0);
  Lits[2] = toLitCond(iVar2, 1);
  Cid = sat_solver_addclause(pSat, Lits, Lits + 3);
  assert(Cid);

  return 2;
}

/**Function*************************************************************
  Synopsis    []
  Description []

  SideEffects []
  SeeAlso     []
***********************************************************************/
int sat_solver_add_conditional_nonequal(sat_solver *pSat, int iVar, int iVar2,
                                        int iVarCond) {
  lit Lits[3];
  int Cid;
  assert(iVar >= 0);
  assert(iVar2 >= 0);
  assert(iVarCond >= 0);

  Lits[0] = toLitCond(iVarCond, 1);
  Lits[1] = toLitCond(iVar, 0);
  Lits[2] = toLitCond(iVar2, 0);
  Cid = sat_solver_addclause(pSat, Lits, Lits + 3);
  assert(Cid);

  Lits[0] = toLitCond(iVarCond, 1);
  Lits[1] = toLitCond(iVar, 1);
  Lits[2] = toLitCond(iVar2, 1);
  Cid = sat_solver_addclause(pSat, Lits, Lits + 3);
  assert(Cid);

  return 2;
}

/**Function*************************************************************
  Synopsis    []
  Description []

  SideEffects []
  SeeAlso     []
***********************************************************************/
int sat_solver_add_equal(sat_solver *pSat, int iVar, int iVar2) {
  lit Lits[2];
  int Cid;
  assert(iVar >= 0);
  assert(iVar2 >= 0);

  Lits[0] = toLitCond(iVar, 1);
  Lits[1] = toLitCond(iVar2, 0);
  Cid = sat_solver_addclause(pSat, Lits, Lits + 2);
  assert(Cid);

  Lits[0] = toLitCond(iVar, 0);
  Lits[1] = toLitCond(iVar2, 1);
  Cid = sat_solver_addclause(pSat, Lits, Lits + 2);
  assert(Cid);

  return 2;
}

/**Function*************************************************************
  Synopsis    []
  Description []

  SideEffects []
  SeeAlso     []
***********************************************************************/
int sat_solver_add_dual_clause(sat_solver *pSat, lit litA, lit litB) {
  lit Lits[2];
  int Cid;
  assert(litA >= 0 && litB >= 0);
  Lits[0] = litA;
  Lits[1] = litB;
  Cid = sat_solver_addclause(pSat, Lits, Lits + 2);
  if (Cid == 0)
    return 0;
  assert(Cid);
  return 1;
}

/**Function*************************************************************
  Synopsis    []
  Description []

  SideEffects []
  SeeAlso     []
***********************************************************************/
int sat_solver_addclause_from(sat_solver *pSat, Cnf_Dat_t *pCnf) {
  for (int i = 0; i < pCnf->nClauses; i++) {
    if (!sat_solver_addclause(pSat, pCnf->pClauses[i], pCnf->pClauses[i + 1]))
      return false;
  }
  return true;
}

/**Function*************************************************************
  Synopsis    []
  Description [This procedure by default does not print binary clauses. To
enable printing of binary clauses, please set fUseBinaryClauses to 0 in the body
of procedure sat_solver_clause_new(sat_solver* s, lit* begin, lit* end, int
learnt) in file "satSolver.c".]

  SideEffects []
  SeeAlso     []
***********************************************************************/
void sat_solver_print(sat_solver *pSat, int fDimacs) {
  Sat_Mem_t *pMem = &pSat->Mem;
  clause *c;
  int i, k, nUnits;

  // count the number of unit clauses
  nUnits = 0;
  for (i = 0; i < pSat->size; i++)
    if (pSat->levels[i] == 0 && pSat->assigns[i] != 3)
      nUnits++;

  //    fprintf( pFile, "c CNF generated by ABC on %s\n", Extra_TimeStamp() );
  printf("p cnf %d %d\n", pSat->size,
         Sat_MemEntryNum(&pSat->Mem, 0) - 1 + Sat_MemEntryNum(&pSat->Mem, 1) +
             nUnits);

  // write the original clauses
  Sat_MemForEachClause(pMem, c, i, k) {
    int i;
    for (i = 0; i < (int)c->size; i++)
      printf("%s%d ", (lit_sign(c->lits[i]) ? "-" : ""),
             lit_var(c->lits[i]) + (fDimacs > 0));
    if (fDimacs)
      printf("0");
    printf("\n");
  }

  // write the learned clauses
  //    Sat_MemForEachLearned( pMem, c, i, k )
  //        Sat_SolverClauseWriteDimacs( pFile, c, fDimacs );

  // write zero-level assertions
  for (i = 0; i < pSat->size; i++)
    if (pSat->levels[i] == 0 && pSat->assigns[i] != 3) // varX
      printf("%s%d%s\n",
             (pSat->assigns[i] == 1) ? "-" : "", // var0
             i + (int)(fDimacs > 0), (fDimacs) ? " 0" : "");

  printf("\n");
}

static pid_t C_PID;
static void (*orig_sighandler)(int) = NULL;
void sigintHandler(int signal_number) {
  kill(C_PID, SIGTERM);
  orig_sighandler(signal_number);
}
void sigalrmHandler(int signal_number) {
  printf("timeout!!\n");
  kill(C_PID, SIGTERM);
}
int Util_CallProcess(char *command, int timeout, int fVerbose,
                     char *exec_command, ...) {
  int status;
  struct sigaction orig_sig;
  sigaction(SIGINT, NULL, &orig_sig);
  orig_sighandler = orig_sig.sa_handler;
  signal(SIGINT, sigintHandler);
  signal(SIGALRM, sigalrmHandler);
  va_list ap;
  char *args[256];
  // parse variable length arguments
  int i = 0;
  args[i] = exec_command;
  va_start(ap, exec_command);
  do {
    i++;
    args[i] = va_arg(ap, char *);
    // printf("%s\n", args[i]);
  } while (args[i] != NULL);

  C_PID = fork();
  if (C_PID == -1) {
    perror("fork()");
    exit(-1);
  } else if (C_PID == 0) { // child process
    if (!fVerbose) {
      int fd = open("/dev/null", O_WRONLY);
      dup2(fd, 1);
      dup2(fd, 2);
      close(fd);
    }
    execvp(command, args);
  } else {
    alarm(timeout);
    waitpid(C_PID, &status, 0);
    signal(SIGINT, orig_sighandler);
    signal(SIGTERM, orig_sighandler);
    signal(SIGSEGV, orig_sighandler);
    if (!WIFEXITED(status)) {
      printf("forked process execution failed\n");
      return 0;
    } else if (WIFSIGNALED(status)) {
      // possibly caught by alarm, reset signal
      signal(SIGALRM, SIG_DFL);
      printf("forked process terminated by signal\n");
      return 0;
    } else {
      if (WEXITSTATUS(status) != 1) { // some process exit abnormally...
        printf("child process exit safe\n");
        alarm(0);
        return 1;
      } else {
        signal(SIGALRM, SIG_DFL);
        return 0;
      }
    }
    signal(SIGINT, SIG_DFL);
  }
  return 1;
}

/**Function*************************************************************
  Synopsis    []
  Description []

  SideEffects []
  SeeAlso     []
***********************************************************************/
void Vec_IntPtrFree(Vec_Ptr_t *vVecInts) {
  int i;
  Vec_Int_t *p;
  Vec_PtrForEachEntry(Vec_Int_t *, vVecInts, p, i) { Vec_IntFree(p); }
  Vec_PtrFree(vVecInts);
}

/**Function*************************************************************
  Synopsis    []
  Description []

  SideEffects []
  SeeAlso     []
***********************************************************************/
static const unsigned int BitSetPosFilter[32] = {
    0b10000000000000000000000000000000, 0b01000000000000000000000000000000,
    0b00100000000000000000000000000000, 0b00010000000000000000000000000000,
    0b00001000000000000000000000000000, 0b00000100000000000000000000000000,
    0b00000010000000000000000000000000, 0b00000001000000000000000000000000,
    0b00000000100000000000000000000000, 0b00000000010000000000000000000000,
    0b00000000001000000000000000000000, 0b00000000000100000000000000000000,
    0b00000000000010000000000000000000, 0b00000000000001000000000000000000,
    0b00000000000000100000000000000000, 0b00000000000000010000000000000000,
    0b00000000000000001000000000000000, 0b00000000000000000100000000000000,
    0b00000000000000000010000000000000, 0b00000000000000000001000000000000,
    0b00000000000000000000100000000000, 0b00000000000000000000010000000000,
    0b00000000000000000000001000000000, 0b00000000000000000000000100000000,
    0b00000000000000000000000010000000, 0b00000000000000000000000001000000,
    0b00000000000000000000000000100000, 0b00000000000000000000000000010000,
    0b00000000000000000000000000001000, 0b00000000000000000000000000000100,
    0b00000000000000000000000000000010, 0b00000000000000000000000000000001};

void Util_BitSet_setValue(unsigned int *bitset, int index, Bool value) {
  assert(bitset);
  assert(0 <= index && index <= 31);
  assert(value == 0 || value == 1);

  const unsigned int filter = BitSetPosFilter[index];
  if (value)
    (*bitset) = (*bitset) | filter;
  else
    (*bitset) = (*bitset) & (~filter);
}

Bool Util_BitSet_getValue(unsigned int *bitset, int index) {
  const unsigned int filter = BitSetPosFilter[index];
  return ((*bitset) & filter) != 0;
}

void Util_BitSet_print(unsigned int bitset) {
  for (int i = 0; i < 32; i++) {
    Abc_Print(1, "%d", Util_BitSet_getValue(&bitset, i));
  }
  Abc_Print(1, "\n");
}

ABC_NAMESPACE_IMPL_END
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
