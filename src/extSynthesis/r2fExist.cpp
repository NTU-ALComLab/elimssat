#include "base/abc/abc.h"
#include "synthesis.h"
#include "unate.h"
#include "unique.h"
#include "synthesisUtil.h"
#include "extR2f/intRel2Func2.hpp"
#include "extUtil/util.h"
#include <vector>

Abc_Ntk_t* r2fExistElim(Abc_Ntk_t* pNtk, Vec_Int_t* pScope, int fVerbose) {
  Abc_Ntk_t* pNtkNew = Abc_NtkStartFrom(pNtk, pNtk->ntkType, pNtk->ntkFunc);
  Abc_Obj_t* pObj;
  int entry, index;
  std::vector<char *> vNameY;
  // TODO preprocess => unate detection
  // printf("size of pScope : %d\n", Vec_IntSize(pScope));
  Abc_Ntk_t *pNtkTemp = Abc_NtkDup(pNtk);
  std::vector<Abc_Ntk_t *> vFunc;
  Vec_IntForEachEntry(pScope, entry, index) {
    pObj = Abc_NtkObj(pNtkTemp, entry);
    vNameY.push_back(Abc_ObjName(pObj));
  }
  // rel2FuncSub(pNtk, vNameY, vFunc);
  // Abc_Ntk_t* pNtkRel = rel2FuncEQ2(pNtk, vNameY, vFunc);
  Abc_Ntk_t* pNtkRel = rel2FuncInt2(pNtkTemp, vNameY, vFunc, fVerbose);
  if (!pNtkRel) return NULL;
  // Abc_Ntk_t* pNtkRel = rel2FuncInt(pNtk, vNameY, vFunc);
  Abc_NtkForEachPi(pNtkTemp, pObj, index) {
    pObj->pCopy = Abc_NtkPi(pNtkNew, index);
  }
  Abc_NtkForEachPi(pNtkRel, pObj, index) {
    Abc_Obj_t* pCorr;
    int j;
    Abc_NtkForEachPi(pNtkTemp, pCorr, j) {
      if (strcmp(Abc_ObjName(pObj), Abc_ObjName(pCorr)) == 0) {
        break;
      }
    }
    pObj->pCopy = pCorr->pCopy;
  }
  Abc_AigConst1(pNtkRel)->pCopy = Abc_AigConst1(pNtkNew);
  Abc_AigForEachAnd(pNtkRel, pObj, index) {
    pObj->pCopy = Abc_AigAnd((Abc_Aig_t*)pNtkNew->pManFunc,
                             Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj));
  }
  Abc_AigForEachAnd(pNtkRel, pObj, index) {
    if (pObj->pData) pObj->pCopy->pData = ((Abc_Obj_t*)pObj->pData)->pCopy;
  }
  // relink the CO nodes
  Abc_NtkForEachCo(pNtkRel, pObj, index) {
    pObj->pCopy = Abc_NtkPo(pNtkNew, index);
  }
  Abc_NtkForEachCo(pNtkRel, pObj, index) {
    Abc_ObjAddFanin(Abc_NtkPo(pNtkNew, index), Abc_ObjChild0Copy(pObj));
  }
  // printf("%d\n", Abc_NtkPiNum(pNtk) - Vec_IntSize(pScope));
  // applyRelation(pNtk, vFunc, vNameY);
  Abc_NtkDelete(pNtkTemp);
  return pNtkNew;
}
