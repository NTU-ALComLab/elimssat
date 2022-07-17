#include "base/abc/abc.h"
#include "synthesis.h"
#include "synthesisUtil.h"

extern "C" {
Abc_Ntk_t* Abc_NtkDC2(Abc_Ntk_t* pNtk, int fBalance, int fUpdateLevel,
                      int fFanout, int fPower, int fVerbose);
}
static Abc_Obj_t* createExistObjRecur(Abc_Ntk_t* pNtk, Abc_Ntk_t* pNtkNew,
                                      Abc_Obj_t* pOrigObj);

/**
 * @brief perform existential quantification using the tool cadet
 *
 * @param pNtk the network to be exist quantified
 * @param pScope the array of the id of the quantified PI
 * @return the existential quantified network
 */
Abc_Ntk_t* cadetExistsElim(Abc_Ntk_t* pNtk, Vec_Int_t* pScope) {
  Abc_Ntk_t* pNtkNew;
  Vec_Int_t* pScopeNew = collectTFO(pNtk, pScope);
  writeQdimacs(pNtk, pScopeNew, "tmp/temp.qdimacs");
  // writeQdimacs(s->pNtk, pScopeNew, "golden_exist.qdimacs");
  Vec_IntFree(pScopeNew);
  // TODO change system to popen
  system("bin/cadet -e tmp/temp_exist.aig tmp/temp.qdimacs");
  Abc_Ntk_t* pExists = Io_ReadAiger("tmp/temp_exist.aig", 1);
  // perform basic synthesis
  Abc_Ntk_t* pExistsSyn = Abc_NtkDC2(pExists, 0, 0, 1, 0, 0);
  pNtkNew = applyExists(pNtk, pExistsSyn);
  Abc_NtkDelete(pExists);
  Abc_NtkDelete(pExistsSyn);
  return pNtkNew;
}

/**
 * @brief create a new network where primary input is the pNtk
 *        with the functionality is equal to pExist
 *
 * @param pNtk the original network
 * @param pExist the result network of cadet
 * @return a network where the PI in pExist is changed to the
 *         correspond object in pNtk
 */
Abc_Ntk_t* applyExists(Abc_Ntk_t* pNtk, Abc_Ntk_t* pExist) {
  Abc_Ntk_t* pNtkNew;
  Abc_Obj_t* pObj, *pPo;
  int index;
  pNtkNew = Abc_NtkStartFrom(pNtk, pNtk->ntkType, pNtk->ntkFunc);
  pPo = Abc_NtkPo(pExist, 0);
  pObj = Abc_ObjFanin0(pPo);
  if (Abc_AigNodeIsConst(pObj)) {
    Abc_Obj_t *pConst = Abc_AigConst1(pNtkNew);
    if (Abc_ObjFaninC0(pPo)) {
      Abc_ObjNot(pConst);
    }
    Abc_ObjAddFanin(Abc_NtkPo(pNtkNew, 0), pConst);
    return pNtkNew;
  }
  Abc_NtkIncrementTravId(pNtk);
  Abc_NtkForEachCo(pExist, pObj, index) {
    pObj->pCopy = Abc_NtkPo(pNtkNew, index);
  }
  Abc_NtkForEachPi(pExist, pObj, index) {
    int id = atoi(Abc_ObjName(pObj));
    Abc_Obj_t* pCorr = Abc_NtkObj(pNtk, id);
    bool fInv = false;
    if (Abc_ObjIsPo(pCorr)) {
      fInv = Abc_ObjFaninC0(pCorr);
      pCorr = Abc_ObjRegular(Abc_ObjFanin0(pCorr));
      id = pCorr->Id;
    }
    if (Abc_ObjIsPi(pCorr)) {
      pObj->pCopy = Abc_NtkObj(pNtkNew, id);
    } else {
      pObj->pCopy = createExistObjRecur(pNtk, pNtkNew, pCorr);
    }
    pObj->pCopy = Abc_ObjNotCond(pObj->pCopy, fInv);
  }
  // copy from function Abc_NtkDup
  Abc_AigForEachAnd(pExist, pObj, index) {
    pObj->pCopy = Abc_AigAnd((Abc_Aig_t*)pNtkNew->pManFunc,
                             Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj));
  }
  Abc_AigForEachAnd(pExist, pObj, index) {
    if (pObj->pData) pObj->pCopy->pData = ((Abc_Obj_t*)pObj->pData)->pCopy;
  }
  // relink the CO nodes
  Abc_NtkForEachCo(pExist, pObj, index) {
    Abc_ObjAddFanin(pObj->pCopy, Abc_ObjChild0Copy(pObj));
  }
  return pNtkNew;
}

static Abc_Obj_t* createExistObjRecur(Abc_Ntk_t* pNtk, Abc_Ntk_t* pNtkNew,
                                      Abc_Obj_t* pOrigObj) {
  Abc_Aig_t* pMan = (Abc_Aig_t*)pNtkNew->pManFunc;
  Abc_Obj_t* pFanin0;
  Abc_Obj_t* pFanin1;
  Abc_Obj_t* pOrigFanin;
  if (Abc_NodeIsTravIdCurrent(pOrigObj)) {
    return pOrigObj->pCopy;
  }
  pOrigFanin = Abc_ObjRegular(Abc_ObjFanin0(pOrigObj));
  if (!Abc_ObjIsPi(pOrigFanin)) {
    pFanin0 = createExistObjRecur(pNtk, pNtkNew, pOrigFanin);
  } else {
    pFanin0 = Abc_NtkObj(pNtkNew, Abc_ObjId(pOrigFanin));
  }
  if (Abc_ObjFaninC0(pOrigObj)) {
    pFanin0 = Abc_ObjNot(pFanin0);
  }
  pOrigFanin = Abc_ObjRegular(Abc_ObjFanin1(pOrigObj));
  if (!Abc_ObjIsPi(pOrigFanin)) {
    pFanin1 = createExistObjRecur(pNtk, pNtkNew, pOrigFanin);
  } else {
    pFanin1 = Abc_NtkObj(pNtkNew, Abc_ObjId(pOrigFanin));
  }
  if (Abc_ObjFaninC1(pOrigObj)) {
    pFanin1 = Abc_ObjNot(pFanin1);
  }
  Abc_Obj_t* pObjNew = Abc_AigAnd(pMan, pFanin0, pFanin1);
  Abc_NodeSetTravIdCurrent(pOrigObj);
  pOrigObj->pCopy = pObjNew;
  return pObjNew;
}
