#include "base/abc/abc.h"
#include "misc/vec/vecFlt.h"
#include "ssatelim.h"

static void ssat_wmc_writeClause(FILE *out, Abc_Ntk_t *pNtk) {
  Abc_Obj_t *pObj, *pFanin0, *pFanin1;
  int i;

  Abc_NtkForEachNode(pNtk, pObj, i) {
    pFanin0 = Abc_ObjFanin0(pObj);
    pFanin1 = Abc_ObjFanin1(pObj);
    if (!Abc_ObjFaninC0(pObj) && !Abc_ObjFaninC1(pObj)) {
      fprintf(out, "%d -%d -%d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0),
              Abc_ObjId(pFanin1));
      fprintf(out, "-%d %d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0));
      fprintf(out, "-%d %d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin1));
    }
    if (Abc_ObjFaninC0(pObj) && !Abc_ObjFaninC1(pObj)) {
      fprintf(out, "%d %d -%d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0),
              Abc_ObjId(pFanin1));
      fprintf(out, "-%d -%d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0));
      fprintf(out, "-%d %d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin1));
    }
    if (!Abc_ObjFaninC0(pObj) && Abc_ObjFaninC1(pObj)) {
      fprintf(out, "%d -%d %d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0),
              Abc_ObjId(pFanin1));
      fprintf(out, "-%d %d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0));
      fprintf(out, "-%d -%d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin1));
    }
    if (Abc_ObjFaninC0(pObj) && Abc_ObjFaninC1(pObj)) {
      fprintf(out, "%d %d %d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0),
              Abc_ObjId(pFanin1));
      fprintf(out, "-%d -%d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0));
      fprintf(out, "-%d -%d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin1));
    }
  }

  pObj = Abc_NtkPo(pNtk, 0);
  pFanin0 = Abc_ObjFanin0(pObj);
  if (!Abc_ObjFaninC0(pObj)) {
    fprintf(out, "%d -%d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0));
    fprintf(out, "-%d %d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0));
  } else {
    fprintf(out, "%d %d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0));
    fprintf(out, "-%d -%d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0));
  }
  fprintf(out, "%d 0\n", Abc_ObjId(pObj));
}

static void ssat_write_wmcWeight(FILE *out, Abc_Ntk_t *pNtk, Vec_Int_t *pRandom,
                                 Vec_Flt_t *pWeight) {
  Abc_Obj_t *pObj;
  int i, entry;
  Abc_NtkIncrementTravId(pNtk);
  Vec_IntForEachEntry(pRandom, entry, i) {
    fprintf(out, "w %d %f\n", Abc_ObjId(Abc_NtkPi(pNtk, entry)),
            Vec_FltEntry(pWeight, i));
    Abc_NodeSetTravIdCurrent(Abc_NtkPi(pNtk, entry));
  }

  Abc_NtkForEachPi(pNtk, pObj, i) {
    if (!Abc_NodeIsTravIdCurrent(pObj)) {
      fprintf(out, "w %d -1\n", Abc_ObjId(pObj));
    }
  }

  Abc_NtkForEachNode(pNtk, pObj, i)
      fprintf(out, "w %d %d\n", Abc_ObjId(pObj), -1);
  Abc_NtkForEachPo(pNtk, pObj, i)
      fprintf(out, "w %d %d\n", Abc_ObjId(pObj), -1);
}

void ssat_write_wmc(Abc_Ntk_t *pNtk, char *name, Vec_Int_t *pRandom,
                    Vec_Flt_t *pWeight) {
  FILE *out;
  int numVar, numCla;

  out = fopen(name, "w");
  if (!out) {
    Abc_Print(-1, "Cannot open file %s\n", name);
    return;
  }
  numVar = Abc_NtkObjNumMax(pNtk) - 1;
  numCla = 3 * Abc_NtkNodeNum(pNtk) + 2 + 1;
  fprintf(out, "c WMC file for ssat %s written by ssatelim\n",
          Abc_NtkName(pNtk));
  fprintf(out, "p cnf %d %d\n", numVar, numCla);

  ssat_wmc_writeClause(out, pNtk);
  ssat_write_wmcWeight(out, pNtk, pRandom, pWeight);
  fclose(out);
  Abc_Print(-2, "File %s is written.\n", name);
}
