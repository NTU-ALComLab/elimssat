/**CFile****************************************************************
  FileName    [ssatParser.cc]
  SystemName  []
  PackageName []
  Synopsis    []
  Author      [Kuan-Hua Tu]

  Affiliation []
  Date        [Sun Jun 23 02:39:25 CST 2019]
***********************************************************************/

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "base/abc/abc.h"
#include "extMinisat/utils/ParseUtils.h"
#include "extSsatelim/literal.hpp"
#include "extUtil/util.h"
#include "formula.hpp"
#include <vector>
#include "ssatelim.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <zlib.h>


// #include <math.h>
#include <math.h>
using namespace std;

ABC_NAMESPACE_IMPL_START

static const int DEBUG_PRINT = 0;
static hqspre::Formula formula;

static const unsigned BUFLEN = 1024000;
static int CLAUSE_NUM = 0;
static int PREFIX_NUM = 0;
static int EXIST_NUM = 0;
static int RANDOM_NUM = 0;

static void error(const char *const msg) {
  std::cerr << msg << "\n";
  exit(255);
}

static void ssat_addScope(ssat_solver *s, std::set<hqspre::Variable> &var_set,
                          QuantifierType type) {
  Vec_Int_t *pScope;
  if (Vec_IntSize(s->pQuanType) && Vec_IntEntryLast(s->pQuanType) == type) {
    pScope = (Vec_Int_t *)Vec_PtrEntryLast(s->pQuan);
  } else {
    pScope = Vec_IntAlloc(0);
    Vec_PtrPush(s->pQuan, pScope);
    Vec_IntPush(s->pQuanType, type);
  }

  for (auto &v : var_set) {
    Vec_IntPush(pScope, v - 1);
  }
}

static Abc_Obj_t *ssat_createMultiOr(Abc_Ntk_t *pNtk,
                                     vector<Abc_Obj_t *> &objVec) {
  int size = objVec.size();
  while (size > 1) {
    int half = size / 2;
    if (size % 2 == 0) {
      for (int i = 0; i < half; i++) {
        objVec[i] = Abc_AigOr((Abc_Aig_t *)pNtk->pManFunc, objVec[2 * i],
                              objVec[2 * i + 1]);
      }
      size = half;
    } else {
      for (int i = 0; i < half; i++) {
        objVec[i] = Abc_AigOr((Abc_Aig_t *)pNtk->pManFunc, objVec[2 * i],
                              objVec[2 * i + 1]);
      }
      objVec[half] = objVec[size - 1];
      size = half + 1;
    }
  }
  return objVec[0];
}

static Abc_Obj_t *ssat_createMultiAnd(Abc_Ntk_t *pNtk,
                                      vector<Abc_Obj_t *> &objVec) {
  int size = objVec.size();
  while (size > 1) {
    int half = size / 2;
    if (size % 2 == 0) {
      for (int i = 0; i < half; i++) {
        objVec[i] = Abc_AigAnd((Abc_Aig_t *)pNtk->pManFunc, objVec[2 * i],
                               objVec[2 * i + 1]);
      }
      size = half;
    } else {
      for (int i = 0; i < half; i++) {
        objVec[i] = Abc_AigAnd((Abc_Aig_t *)pNtk->pManFunc, objVec[2 * i],
                               objVec[2 * i + 1]);
      }
      objVec[half] = objVec[size - 1];
      size = half + 1;
    }
  }
  return objVec[0];
}

static Abc_Obj_t *
ssat_createAndGate(ssat_solver *s, const hqspre::Gate &g,
                   const unordered_map<hqspre::Variable, int> &varObjMap) {
  Abc_Obj_t *pOut;
  // create a balance AIG network instead a flat conversion
  vector<Abc_Obj_t *> objVec;
  for (auto &l : g._input_literals) {
    int index = varObjMap.at(hqspre::lit2var(l));
    Abc_Obj_t *pObj = Abc_NtkPi(s->pNtk, index)->pCopy;
    objVec.push_back(Abc_ObjNotCond(pObj, hqspre::isNegative(l)));
  }
  pOut = ssat_createMultiAnd(s->pNtk, objVec);
  objVec.clear();
  pOut = Abc_ObjNotCond(pOut, hqspre::isNegative(g._output_literal));
  Abc_NtkPi(s->pNtk, varObjMap.at(hqspre::lit2var(g._output_literal)))->pCopy =
      pOut;
  for (auto &c_nr : g._encoding_clauses) {
    formula.removeClause(c_nr);
  }
  return pOut;
}

int ssat_addScope(ssat_solver *s, int *begin, int *end, QuantifierType type) {
  Vec_Int_t *pScope;
  if (Vec_IntSize(s->pQuanType) && Vec_IntEntryLast(s->pQuanType) == type) {
    pScope = (Vec_Int_t *)Vec_PtrEntryLast(s->pQuan);
  } else {
    pScope = Vec_IntAlloc(0);
    Vec_PtrPush(s->pQuan, pScope);
    Vec_IntPush(s->pQuanType, type);
  }

  for (int *it = begin; it < end; it++) {
    Vec_IntPush(pScope, *it - 1);
  }
  return 1;
}

int ssat_addexistence(ssat_solver *s, int *begin, int *end) {
  Vec_FltPush(s->pQuanWeight, -1);
  return ssat_addScope(s, begin, end, Quantifier_Exist);
}

int ssat_addforall(ssat_solver *s, int *begin, int *end) {
  Vec_FltPush(s->pQuanWeight, -1);
  return ssat_addScope(s, begin, end, Quantifier_Forall);
}

int ssat_addrandom(ssat_solver *s, int *begin, int *end, double prob) {
  Vec_FltPush(s->pQuanWeight, prob);
  if (prob != 0.5) {
    Abc_Print(-1, "Probility of random var is not 0.5\n");
  }
  return ssat_addScope(s, begin, end, Quantifier_Random);
}

int ssat_addclause(ssat_solver *s, lit *begin, lit *end) {
  Abc_Obj_t *pObj = Abc_AigConst0(s->pNtk);
  for (lit *it = begin; it < end; it++) {
    int var = lit_var(*it);
    int sign = lit_sign(*it);
    Abc_Obj_t *pPi = Abc_NtkPi(s->pNtk, var - 1);
    if (sign) {
      pPi = Abc_ObjNot(pPi);
    }
    pObj = Util_AigNtkOr(s->pNtk, pObj, pPi);
  }
  s->pPo = Util_AigNtkAnd(s->pNtk, s->pPo, pObj);
  return 1;
}

void ssat_Parser(ssat_solver *s, char *filename) {
  if (filename == NULL) {
    return;
  }
  std::ifstream in(filename);
  if (!in) {
    printf("Cannot open file\n");
    std::exit(-1);
  }
  try {
    in >> formula;
  } catch (hqspre::FileFormatException &e) {
    printf("Error in file format\n");
    in.close();
    return;
  }
  in.close();

  std::unordered_map<hqspre::Variable, int> varObjMap;
  std::unordered_set<hqspre::Variable> definingVar;
  const auto exist_vars = formula.numEVars();
  const auto univ_vars = formula.numUVars();
  auto literals = formula.numLiterals();
  auto clauses = formula.numClauses();
  printf("Number of exist variables: %zu\n", exist_vars);
  printf("Number of univ variables: %zu\n", univ_vars);
  printf("Number of literals: %zu\n", literals);
  printf("Number of clauses: %zu\n", clauses);
  formula.settings().preserve_gates = true;

  formula.unitPropagation();
  // formula.determineGates(true, false, false, false);
  const std::vector<hqspre::Gate> gate_vec = formula.getGates();
  clauses = formula.numClauses();
  literals = formula.numLiterals();

  for (const hqspre::Gate &g: gate_vec) {
    const hqspre::Variable out_var = hqspre::lit2var(g._output_literal);
    definingVar.insert(out_var);
  }
  Abc_Obj_t *pObj;
  int i;
  vector<Abc_Obj_t *> objVec;
  i = 0;
  const hqspre::QBFPrefix *_prefix = formula.getQBFPrefix();
  size_t block_num = _prefix->getMaxLevel() + 1;
  for (hqspre::Variable var = _prefix->minVarIndex();
       var <= _prefix->maxVarIndex(); ++var) {
    if (_prefix->varDeleted(var)) {
      continue;
    }
    Abc_NtkCreatePi(s->pNtk);
    varObjMap[var] = i;
    i++;
  }
  printf("Number of Detected Gate: %zu\n", gate_vec.size());
  Abc_NtkForEachPi(s->pNtk, pObj, i) { Abc_ObjSetCopy(pObj, pObj); }
  // create gates
  for (const hqspre::Gate &g : gate_vec) {
    assert(g._type == hqspre::GateType::AND_GATE);
    // objVec.push_back(ssat_createAnd(s, g));
    ssat_createAndGate(s, g, varObjMap);
  }

  // read prefixes
  for (int i = 0; i < block_num; ++i) {
    std::set<hqspre::Variable> var_set = _prefix->getVarBlock(i);
    hqspre::VariableStatus status = _prefix->getLevelQuantifier(i);
    if (status == hqspre::VariableStatus::DELETED)
      continue;
    Vec_Int_t *pScope;
    QuantifierType type = (QuantifierType)status;
    if (Vec_IntSize(s->pQuanType) && Vec_IntEntryLast(s->pQuanType) == type) {
      pScope = (Vec_Int_t *)Vec_PtrEntryLast(s->pQuan);
    } else {
      pScope = Vec_IntAlloc(0);
      Vec_PtrPush(s->pQuan, pScope);
      Vec_IntPush(s->pQuanType, type);
    }

    for (auto &v : var_set) {
      if (type == Quantifier_Exist && definingVar.find(v) != definingVar.end()) continue;
      Vec_IntPush(pScope, varObjMap[v]);
    }
  }

  // create clauses

  for (int i = 0; i < formula.maxClauseIndex() + 1; i++) {
    if (formula.clauseDeleted(i))
      continue;
    vector<Abc_Obj_t *> clauseVec;
    const hqspre::Clause c = formula.getClause(i);
    for (auto &l : c) {
      Abc_Obj_t *pPi = Abc_NtkPi(s->pNtk, varObjMap[hqspre::lit2var(l)])->pCopy;
      clauseVec.push_back(Abc_ObjNotCond(pPi, hqspre::isNegative(l)));
    }
    objVec.push_back(ssat_createMultiOr(s->pNtk, clauseVec));
  }
  // for (auto i: formula._unit_stack)
  s->pPo = ssat_createMultiAnd(s->pNtk, objVec);

  // ssat_ParseFile(s, gzopen(filename, "rb"));
  int entry, index;
  Vec_IntForEachEntry(s->pQuanType, entry, index) {
    PREFIX_NUM++;
    if (entry == Quantifier_Exist) {
      EXIST_NUM += Vec_IntSize((Vec_Int_t *)(Vec_PtrEntry(s->pQuan, index)));
    } else {
      RANDOM_NUM += Vec_IntSize((Vec_Int_t *)(Vec_PtrEntry(s->pQuan,
      index)));
    }
  }
  printf("\n");
  printf("==== benchmark statistics ====\n");
  printf("  > Number of Prefixes                = %d\n", PREFIX_NUM);
  printf("  > Number of Existitential Variables = %d\n", EXIST_NUM);
  printf("  > Number of Randomize Variables     = %d\n", RANDOM_NUM);

  return;
}

ABC_NAMESPACE_IMPL_END
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
