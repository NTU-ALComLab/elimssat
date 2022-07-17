#include "base/abc/abc.h"
#include "synthesis.h"
#include "synthesisUtil.h"
#include <unistd.h>

extern "C" {
Abc_Ntk_t *Abc_NtkDC2(Abc_Ntk_t *pNtk, int fBalance, int fUpdateLevel,
                      int fFanout, int fPower, int fVerbose);
}

Abc_Ntk_t *applySkolem(Abc_Ntk_t *pNtk, Abc_Ntk_t *pSkolem,
                              Vec_Int_t *vForalls, Vec_Int_t *vExists);
static Abc_Obj_t *createSkolemObjRecur(Abc_Ntk_t *pNtk, Abc_Ntk_t *pNtkSkolem,
                                       Abc_Ntk_t *pNtkNew,
                                       Abc_Obj_t *pSkolemObj,
                                       Vec_Int_t *vForalls);
static Abc_Obj_t *createOrigObjRecur(Abc_Ntk_t *pNtk, Abc_Ntk_t *pNtkNew,
                                     Abc_Obj_t *pObj);

static Abc_Ntk_t *preprocessSkolem(Abc_Ntk_t *pPre, Abc_Ntk_t *pSkolem,
                                   Vec_Int_t *vForalls, Vec_Int_t *vExists);

static void collectVariables(Abc_Ntk_t *pNtk, Vec_Int_t *pIndex,
                             Vec_Int_t *vExists, Vec_Int_t *vForalls) {
  Abc_NtkIncrementTravId(pNtk);
  int entry, index;
  Abc_Obj_t *pObj;
  Vec_IntForEachEntry(pIndex, entry, index) {
    Abc_Obj_t *pObj = Abc_NtkObj(pNtk, entry);
    Abc_NodeSetTravIdCurrent(pObj);
  }
  Abc_NtkForEachObj(pNtk, pObj, index) {
    if (Abc_ObjId(pObj) == 0)
      continue;
    if (Abc_NodeIsTravIdCurrent(pObj)) {
      Vec_IntPush(vExists, Abc_ObjId(pObj));
    } else {
      Vec_IntPush(vForalls, Abc_ObjId(pObj));
    }
  }
}

Abc_Ntk_t *manthanExistsElim(Abc_Ntk_t *pNtk, Vec_Int_t *pScope, int fVerbose, char* pName = NULL) {
  Vec_Int_t *vExists = Vec_IntAlloc(0);
  Vec_Int_t *vForalls = Vec_IntAlloc(0);
  Abc_Ntk_t *pNtkNew;
  Vec_Int_t *pScopeNew = collectTFO(pNtk, pScope);
  if (pName == NULL) {
    pName = "temp";
  }
  char temp_aigfile[256], temp_qdimcasfile[256], temp_manthanfile[256];
  sprintf(temp_qdimcasfile, "%s.qdimacs", pName);
  sprintf(temp_manthanfile, "../%s.qdimacs", pName);
  sprintf(temp_aigfile, "manthan/%s_skolem.aig", pName);
  collectVariables(pNtk, pScopeNew, vExists, vForalls);
  writeQdimacsFile(pNtk, temp_qdimcasfile, 0, vForalls, vExists);
  Vec_IntFree(pScopeNew);
  chdir("manthan");
  callProcess("python3", fVerbose, "python3", "manthan.py", // "--preprocess", "--unique",
              // "--unique",
              "--unique", "--preprocess",
              "--multiclass",
              // "--lexmaxsat",
              "--verb", "0", temp_manthanfile, NULL);
  chdir("../");
  if (fVerbose) {
    Abc_Print(1, "[INFO] Calling manthan done\n");
  }
  Abc_Ntk_t *pSkolem = Io_ReadAiger(temp_aigfile, 1);
  if (!pSkolem) return NULL;
  Abc_Ntk_t *pSkolemSyn = Abc_NtkDC2(pSkolem, 0, 0, 1, 0, 0);
  pNtkNew = applySkolem(pNtk, pSkolemSyn, vForalls, vExists);
  Abc_NtkDelete(pSkolem);
  Abc_NtkDelete(pSkolemSyn);
  Vec_IntFree(vExists);
  Vec_IntFree(vForalls);
  remove(temp_aigfile);
  remove(temp_qdimcasfile);
  return pNtkNew;
}

Abc_Ntk_t *applySkolem(Abc_Ntk_t *pNtk, Abc_Ntk_t *pSkolem,
                              Vec_Int_t *vForalls, Vec_Int_t *vExists) {
  // printf("%d %d\n", Abc_NtkPiNum(pSkolem), Vec_IntSize(vForalls));
  // printf("%d %d\n", Abc_NtkPoNum(pSkolem), Vec_IntSize(vExists));
  assert(Abc_NtkPiNum(pSkolem) == Vec_IntSize(vForalls));
  assert(Abc_NtkPoNum(pSkolem) == Vec_IntSize(vExists));
  Abc_Ntk_t *pNtkNew;
  Abc_Obj_t *pObj;
  int index, entry;
  Abc_NtkCleanCopy(pNtk);
  pNtkNew = Abc_NtkStartFrom(pNtk, pNtk->ntkType, pNtk->ntkFunc);
  Abc_NtkIncrementTravId(pNtk);
  Vec_IntForEachEntry(vForalls, entry, index) {
    Abc_Obj_t *pCorr = Abc_NtkObj(pNtk, entry);
    if (Abc_ObjIsPi(pCorr)) {
      assert(pCorr->pCopy == Abc_NtkObj(pNtkNew, entry));
      continue;
    }
    pCorr->pCopy = createOrigObjRecur(pNtk, pNtkNew, pCorr);
  }
  Abc_NtkIncrementTravId(pSkolem);
  Abc_NtkForEachPo(pSkolem, pObj, index) {
    Abc_Obj_t *pCorr = Abc_NtkObj(pNtk, Vec_IntEntry(vExists, index));
    if (!Abc_ObjIsPi(pCorr)) {
      continue;
    }
    bool fInv = Abc_ObjFaninC0(pObj);
    pObj = Abc_ObjRegular(Abc_ObjFanin0(pObj));
    if (Abc_AigNodeIsConst(pObj)) {
      pCorr->pCopy = Abc_ObjNotCond(Abc_AigConst1(pNtkNew), fInv);
      continue;
    }
    Abc_Obj_t *pNew =
        createSkolemObjRecur(pNtk, pSkolem, pNtkNew, pObj, vForalls);
    pNew = Abc_ObjNotCond(pNew, fInv);
    pCorr->pCopy = pNew;
  }
  // copy the AND gates
  Abc_AigForEachAnd(pNtk, pObj, index) {
    if (pObj->pCopy)
      continue;
    pObj->pCopy = Abc_AigAnd((Abc_Aig_t *)pNtkNew->pManFunc,
                             Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj));
  }
  // relink the CO nodes
  Abc_NtkForEachCo(pNtk, pObj, index) {
    Abc_ObjAddFanin(pObj->pCopy, Abc_ObjChild0Copy(pObj));
  }
  assert(Abc_NtkCheck(pNtkNew));
  return pNtkNew;
}

static Abc_Obj_t *createOrigObjRecur(Abc_Ntk_t *pNtk, Abc_Ntk_t *pNtkNew,
                                     Abc_Obj_t *pObj) {
  Abc_Aig_t *pMan = (Abc_Aig_t *)pNtkNew->pManFunc;
  Abc_Obj_t *pFanin0;
  Abc_Obj_t *pFanin1;
  Abc_Obj_t *pOrigFanin;
  assert(pObj->pNtk == pNtk);
  if (Abc_NodeIsTravIdCurrent(pObj)) {
    return pObj->pCopy;
  }
  pOrigFanin = Abc_ObjRegular(Abc_ObjFanin0(pObj));
  if (Abc_ObjIsPi(pOrigFanin)) {
    pFanin0 = pOrigFanin->pCopy;
  } else {
    pFanin0 = createOrigObjRecur(pNtk, pNtkNew, pOrigFanin);
  }
  pFanin0 = Abc_ObjNotCond(pFanin0, Abc_ObjFaninC0(pObj));
  pOrigFanin = Abc_ObjRegular(Abc_ObjFanin1(pObj));
  if (Abc_ObjIsPi(pOrigFanin)) {
    pFanin1 = pOrigFanin->pCopy;
  } else {
    pFanin1 = createOrigObjRecur(pNtk, pNtkNew, pOrigFanin);
  }
  pFanin1 = Abc_ObjNotCond(pFanin1, Abc_ObjFaninC1(pObj));
  pObj->pCopy = Abc_AigAnd(pMan, pFanin0, pFanin1);
  Abc_NodeSetTravIdCurrent(pObj);
  return pObj->pCopy;
}

static Abc_Obj_t *createSkolemObjRecur(Abc_Ntk_t *pNtk, Abc_Ntk_t *pNtkSkolem,
                                       Abc_Ntk_t *pNtkNew,
                                       Abc_Obj_t *pSkolemObj,
                                       Vec_Int_t *vForalls) {
  Abc_Aig_t *pMan = (Abc_Aig_t *)pNtkNew->pManFunc;
  Abc_Obj_t *pFanin0;
  Abc_Obj_t *pFanin1;
  Abc_Obj_t *pOrigFanin;
  if (Abc_AigNodeIsConst(pSkolemObj)) {
    return Abc_AigConst1(pNtkNew);
  }
  if (Abc_ObjIsPi(pSkolemObj)) {
    int pi_index = Abc_ObjId(pSkolemObj) - 1;
    assert(pSkolemObj == Abc_NtkPi(pNtkSkolem, pi_index));
    return Abc_NtkObj(pNtk, Vec_IntEntry(vForalls, pi_index))->pCopy;
  }
  if (pSkolemObj->pCopy) {
    return pSkolemObj->pCopy;
  }
  pOrigFanin = Abc_ObjRegular(Abc_ObjFanin0(pSkolemObj));
  if (Abc_ObjIsPi(pOrigFanin)) {
    int pi_index = Abc_ObjId(pOrigFanin) - 1;
    assert(pOrigFanin == Abc_NtkPi(pNtkSkolem, pi_index));
    pFanin0 = Abc_NtkObj(pNtk, Vec_IntEntry(vForalls, pi_index))->pCopy;
  } else if (Abc_AigNodeIsConst(pOrigFanin)) {
    pFanin0 = Abc_AigConst1(pNtkNew);
  } else {
    pFanin0 =
        createSkolemObjRecur(pNtk, pNtkSkolem, pNtkNew, pOrigFanin, vForalls);
  }
  pFanin0 = Abc_ObjNotCond(pFanin0, Abc_ObjFaninC0(pSkolemObj));
  pOrigFanin = Abc_ObjRegular(Abc_ObjFanin1(pSkolemObj));
  if (Abc_ObjIsPi(pOrigFanin)) {
    int pi_index = Abc_ObjId(pOrigFanin) - 1;
    assert(pOrigFanin == Abc_NtkPi(pNtkSkolem, pi_index));
    pFanin1 = Abc_NtkObj(pNtk, Vec_IntEntry(vForalls, pi_index))->pCopy;
  } else if (Abc_AigNodeIsConst(pOrigFanin)) {
    pFanin1 = Abc_AigConst1(pNtkNew);
  } else {
    pFanin1 =
        createSkolemObjRecur(pNtk, pNtkSkolem, pNtkNew, pOrigFanin, vForalls);
  }
  pFanin1 = Abc_ObjNotCond(pFanin1, Abc_ObjFaninC1(pSkolemObj));
  Abc_Obj_t *pObjNew = Abc_AigAnd(pMan, pFanin0, pFanin1);
  // NOTE pObjNew can be const0 or const1
  Abc_NodeSetTravIdCurrent(Abc_ObjRegular(pObjNew));
  pSkolemObj->pCopy = pObjNew;
  return pObjNew;
}
