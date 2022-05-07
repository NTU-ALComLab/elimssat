#include "intRel2Func2.hpp"
#include "base/abc/abc.h"
#include "extUtil/util.h"
#include "subRel2Func.hpp"
#include <algorithm>
#include <iostream>

#define MAX_AIG_NODE 50000

//#define _DEBUG
using namespace std;
extern "C" {
Abc_Ntk_t *Abc_NtkInter(Abc_Ntk_t *pNtkOn, Abc_Ntk_t *pNtkOff, int fRelation,
                        int fVerbose);
Abc_Ntk_t *Abc_NtkMulti(Abc_Ntk_t *pNtk, int nThresh, int nFaninMax, int fCnf,
                        int fMulti, int fSimple, int fFactor);
}

Abc_Ntk_t *replaceX(Abc_Ntk_t *pNtk, Abc_Ntk_t *pNtkFunc, char *yName) {
  Abc_Ntk_t *pNtkNew = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
  Abc_Obj_t *pObj, *pObjNew;
  int i;

  // cerr << "start replace" << endl;
  assert(Abc_NtkFindCi(pNtkFunc, yName) == NULL);
  // assert(Abc_NtkPiNum(pNtk) >= Abc_NtkPiNum(pNtkFunc) + 1);
  // assert(Abc_NtkFindCi(pNtk, yName) != NULL);
  if (Abc_NtkFindCi(pNtk, yName) == NULL)
    return pNtk;

  Abc_AigConst1(pNtkFunc)->pCopy = Abc_AigConst1(pNtkNew);
  Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNew);
  Abc_NtkForEachPi(pNtkFunc, pObj, i) {
    assert(strcmp(Abc_ObjName(pObj), yName) != 0);
    pObjNew = Abc_NtkCreatePi(pNtkNew);
    // add name
    Abc_ObjAssignName(pObjNew, Abc_ObjName(pObj), NULL);

    // remember this PI in the old PIs
    pObj->pCopy = pObjNew;
  }

  Util_NtkAppend(pNtkNew, pNtkFunc);
  Abc_Obj_t *pObj1 = Abc_ObjChild0Copy(Abc_NtkPo(pNtkFunc, 0));

  Abc_NtkForEachPi(pNtk, pObj, i) {
    if (strcmp(Abc_ObjName(pObj), yName) == 0) {
      pObjNew = pObj1;
    } else {
      pObjNew = Abc_NtkFindCi(pNtkNew, Abc_ObjName(pObj));
      if (pObjNew == NULL) {
        pObjNew = Abc_NtkCreatePi(pNtkNew);
        // add name
        Abc_ObjAssignName(pObjNew, Abc_ObjName(pObj), NULL);
      }
    }
    assert(pObjNew != NULL);
    // remember this PI in the old PIs
    pObj->pCopy = pObjNew;
  }
  Util_NtkAppend(pNtkNew, pNtk);

  Abc_Obj_t *pObj2 = Abc_ObjChild0Copy(Abc_NtkPo(pNtk, 0));

  Abc_Obj_t *pObjOut = Abc_NtkCreatePo(pNtkNew);
  char name[10];
  sprintf(name, "%s", "test");
  Abc_ObjAssignName(pObjOut, name, NULL);
  Abc_ObjAddFanin(pObjOut, pObj2);

  Abc_NtkDelete(pNtk);

  // pNtkNew = resyn(pNtkNew);
  // pNtkNew = resyn(pNtkNew);

  return pNtkNew;
}

// original
Abc_Ntk_t *singleRel2FuncInt2(Abc_Ntk_t *pNtk, char *yName,
                              vector<char *> &vNameY) {

  // cerr << "start sinRel2funcInt" << endl;
  Abc_Ntk_t *pNtkNewA = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
  Abc_Ntk_t *pNtkNewB = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
  Abc_Ntk_t *pNtkFunc;
  Abc_Obj_t *pObj, *pObjNew, *pObj1, *pObj2;
  int i;

  Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNewA);
  Abc_NtkForEachPi(pNtk, pObj, i) {
    if (strcmp(Abc_ObjName(pObj), yName) == 0) {
      pObjNew = Abc_AigConst1(pNtkNewA);
    } else {
      pObjNew = Abc_NtkCreatePi(pNtkNewA);
      // add name
      Abc_ObjAssignName(pObjNew, Abc_ObjName(pObj), NULL);
    }
    // remember this PI in the old PIs
    pObj->pCopy = pObjNew;
  }
  Util_NtkAppend(pNtkNewA, pNtk);

  pObj1 = Abc_ObjChild0Copy(Abc_NtkPo(pNtk, 0));

  pObj = Abc_NtkFindCi(pNtk, yName);
  pObj->pCopy = Abc_ObjNot(Abc_AigConst1(pNtkNewA));
  Util_NtkAppend(pNtkNewA, pNtk);

  pObj2 = Abc_ObjNot(Abc_ObjChild0Copy(Abc_NtkPo(pNtk, 0)));
  assert(pObj1);
  assert(pObj2);

  pObjNew = Abc_AigAnd((Abc_Aig_t *)pNtkNewA->pManFunc, pObj1, pObj2);

  Abc_Obj_t *pObjOut = Abc_NtkCreatePo(pNtkNewA);
  char name[10];
  sprintf(name, "%s", "test");
  Abc_ObjAssignName(pObjOut, name, NULL);
  Abc_ObjAddFanin(pObjOut, pObjNew);

  Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNewB);
  Abc_NtkForEachPi(pNtk, pObj, i) {
    if (strcmp(Abc_ObjName(pObj), yName) == 0) {
      pObjNew = Abc_ObjNot(Abc_AigConst1(pNtkNewB));
    } else {
      pObjNew = Abc_NtkCreatePi(pNtkNewB);
      // add name
      Abc_ObjAssignName(pObjNew, Abc_ObjName(pObj), NULL);
    }
    // remember this PI in the old PIs
    pObj->pCopy = pObjNew;
  }
  Util_NtkAppend(pNtkNewB, pNtk);

  pObj1 = Abc_ObjChild0Copy(Abc_NtkPo(pNtk, 0));

  pObj = Abc_NtkFindCi(pNtk, yName);
  pObj->pCopy = Abc_AigConst1(pNtkNewB);
  Util_NtkAppend(pNtkNewB, pNtk);

  pObj2 = Abc_ObjNot(Abc_ObjChild0Copy(Abc_NtkPo(pNtk, 0)));
  assert(pObj1);
  assert(pObj2);

  pObjNew = Abc_AigAnd((Abc_Aig_t *)pNtkNewB->pManFunc, pObj1, pObj2);

  pObjOut = Abc_NtkCreatePo(pNtkNewB);
  sprintf(name, "%s", "test");
  Abc_ObjAssignName(pObjOut, name, NULL);
  Abc_ObjAddFanin(pObjOut, pObjNew);

  //  cerr << "start add clause" << endl;
  pNtkFunc = Abc_NtkInter(pNtkNewA, pNtkNewB, 0, 0);
  Abc_NtkDelete(pNtkNewA);
  Abc_NtkDelete(pNtkNewB);

  return pNtkFunc;
}

Abc_Ntk_t *rel2FuncInt2(Abc_Ntk_t *pNtk, vector<char *> &vNameY,
                        vector<Abc_Ntk_t *> &vFunc, bool fVerbose) {
  Abc_Ntk_t *pNtkFunc;
  Abc_Ntk_t *pNtkRel = Abc_NtkDup(pNtk);
  pNtkRel = Util_NtkResyn2(pNtkRel, 1);

  vector<Abc_Ntk_t *> vNtkRel;
  // Abc_Ntk_t* pNtkRel == Abc_NtkCollapse( pNtk, 50000000, 0, 1, 0 );
  // cout << "function info during processing" << endl;
  // cout << "-------------------------------" << endl;
  if (fVerbose) {
    cout << "\tsoltype\tlev#\tnode#\tpi#\trel-lev#\trel-node#" << endl;
    cout << "-------------------------------" << endl;
  }
  for (unsigned i = 0; i < vNameY.size(); i++) {
    // cout << i << "\tRelation Node: " << Abc_NtkObjNum(pNtkRel) << endl;
    // cout << "begin solve\n" << endl;
    int type = 0;
    pNtkFunc = singleRel2FuncInt2(pNtkRel, vNameY[i], vNameY);
    // cout << "end solve\n" << endl;
    Abc_Ntk_t *pNtkFuncTemp = singleRel2Func(pNtkRel, vNameY[i]);
    if (pNtkFunc == NULL) {
      type = 1;
      pNtkFunc = pNtkFuncTemp;
    }
    // else {
    else if (Abc_NtkObjNum(pNtkFuncTemp) > Abc_NtkObjNum(pNtkFunc)) {
      // pNtkFunc = resyn(pNtkFunc);
      Abc_NtkDelete(pNtkFuncTemp);
    } else {
      if (fVerbose)
        cout << "Substitution is better than Interpolant" << endl;
      Abc_NtkDelete(pNtkFunc);
      pNtkFunc = pNtkFuncTemp;
      type = 1;
    }
    /* if(pNtkFunc == NULL) {
         cout <<  "sat time out" << MAX_AIG_NODE << endl;
         Abc_NtkDelete(pNtkRel);
         return;
     }*/
    if (Abc_NtkNodeNum(pNtkFunc) > MAX_AIG_NODE) {
      cout << "The number of AIG nodes reached " << MAX_AIG_NODE << endl;
      Abc_NtkDelete(pNtkFunc);
      Abc_NtkDelete(pNtkRel);
      return NULL;
    }
    pNtkFunc = Util_NtkDFraig(pNtkFunc, 1, 1);
    pNtkFunc = Util_NtkDc2(pNtkFunc, 1);
    pNtkFunc = Util_NtkResyn2rs(pNtkFunc, 1);
    pNtkRel = replaceX(pNtkRel, pNtkFunc, vNameY[i]);
    pNtkRel = Util_NtkDc2(pNtkRel, 1);
    // pNtkRel = Util_NtkResyn2rs(pNtkRel, 1);
    if (Abc_NtkNodeNum(pNtkRel) > MAX_AIG_NODE) {
      Abc_NtkDelete(pNtkFunc);
      Abc_NtkDelete(pNtkRel);
      return NULL;
    }
    if (fVerbose) {
      cout << i + 1 << "\t" << type << "\t" << Abc_AigLevel(pNtkFunc) << "\t"
           << Abc_NtkNodeNum(pNtkFunc) << "\t" << Abc_NtkPiNum(pNtkFunc) << "\t"
           << Abc_AigLevel(pNtkRel) << "\t" << Abc_NtkNodeNum(pNtkRel) << endl;
    }
    vFunc.push_back(pNtkFunc);
  }
  vNtkRel.clear();
  return pNtkRel;
}
