#include "extUtil/easylogging++.h"
#include "extUtil/utilVec.h"
#include "ssatParser.h"
#include <functional>
#include <unordered_set>

ABC_NAMESPACE_IMPL_START

using std::unordered_set;
using std::vector;

static const int CONFLICT_LIMIT = 1000;

/**
 * @brief debug function to print the clauses to standard out
 */
static void printClause(int *pArray, int size) {
  for (int i = 0; i < size; i++) {
    printf("%s%d ", lit_sign(pArray[i]) ? "-" : "", lit_var(pArray[i]));
  }
  printf("\n");
}

/**
 * @brief add clauses to the interpolation solver
 */
avy::ItpMinisat* ssatParser::createItpSolver(Vec_Int_t *vConsider) {
  avy::ItpMinisat *solver = new avy::ItpMinisat(2, 1, false);
  unordered_set<int> consider_set;
  Vec_Int_t *vVec;
  int index, entry;
  // create a hash table consist of considered variables
  auto consider_insert = [&consider_set](int v) { consider_set.insert(v); };
  Vec_IntMap(vConsider, consider_insert);
  solver->reserve(2 * _nVar + 2 * Vec_IntSize(vConsider) + 1);
  Vec_Int_t *vClause = Vec_IntAlloc(0);
  // lambda function to convert the clause into minisat format
  auto convert_clause = [&vClause](int l) {
    int var = abs(l);
    int lit = signbit(l) ? 2 * var + 1 : 2 * var;
    Vec_IntPush(vClause, lit);
  };
  // insert clauses into solver
  Vec_PtrForEachEntry(Vec_Int_t *, _clauseSet, vVec, index) {
    solver->markPartition(1);
    Vec_IntMap(vVec, convert_clause);
    solver->addClause(vClause->pArray,
                          vClause->pArray + Vec_IntSize(vClause));
    Vec_IntClear(vClause);
  }
  // lambda function to copy variables and convert the clause to minisat format
  int n = _nVar;
  auto convert_clause_copy = [&vClause, &consider_set, n](int l) {
    int var =
        consider_set.find(abs(l)) != consider_set.end() ? n + abs(l) : abs(l);
    int lit = signbit(l) ? 2 * var + 1 : 2 * var;
    Vec_IntPush(vClause, lit);
  };
  // insert clauses with considered varibles copied
  Vec_PtrForEachEntry(Vec_Int_t *, _clauseSet, vVec, index) {
    solver->markPartition(2);
    Vec_IntMap(vVec, convert_clause_copy);
    int j;
    Vec_IntForEachEntry(vClause, entry, j) {}
    solver->addClause(vClause->pArray,
                          vClause->pArray + Vec_IntSize(vClause));
    Vec_IntClear(vClause);
  }
  return solver;
}

Abc_Obj_t *ssatParser::buildDefinition(vector<int> &shared_vector,
                                       Aig_Man_t *pAig) {
  int index;
  Aig_Obj_t *pObj;
  Vec_Ptr_t *vNodes;
  vNodes = Aig_ManDfs(pAig, 1);
  Aig_ManConst1(pAig)->pData = Abc_AigConst1(_solver->pNtk);
  Aig_ManForEachCi(pAig, pObj, index) {
    pObj->pData = Abc_NtkPi(_solver->pNtk, shared_vector[index] - 1);
  }
  Vec_PtrForEachEntry(Aig_Obj_t *, vNodes, pObj, index) {
    if (Aig_ObjIsBuf(pObj)) {
      pObj->pData = (Abc_Obj_t *)Aig_ObjChild0Copy(pObj);
    } else {
      pObj->pData = Abc_AigAnd((Abc_Aig_t *)_solver->pNtk->pManFunc,
                               (Abc_Obj_t *)Aig_ObjChild0Copy(pObj),
                               (Abc_Obj_t *)Aig_ObjChild1Copy(pObj));
    }
  }
  Vec_PtrFree(vNodes);
  pObj = Aig_ManCo(pAig, 0);
  Abc_Obj_t *pRet = (Abc_Obj_t *)Aig_ObjChild0Copy(pObj);
  assert(Abc_ObjRegular(pRet)->pNtk->pManFunc ==
         (Abc_Aig_t *)_solver->pNtk->pManFunc);
  return pRet;
}

/**
 * @brief perform interpolation based semantic gate extraction
 *
 * For more information, see
 * - Interpolation-Based Semantic Gate Extraction and Its Application to QBF
 * Preprocessing published on CAV 2020
 *
 * extracted definition would be stored in _definedVariables
 */
void ssatParser::uniquePreprocess() {
  val_assert_msg(Vec_IntSize(_quanType) != 0 && Vec_PtrSize(_quanBlock) != 0,
                 "no quantifier to be processed!");
  val_assert_msg(Vec_PtrSize(_clauseSet) != 0, "no clause to be processed!");
  // TODO add general support
  if (Vec_IntEntryLast(_quanType) != Quantifier_Exist)
    return;

  VLOG(1) << "perform interpolation-based semantic gate extraction";
  vector<int> shared_vector;
  unordered_set<int> consider_set;
  Vec_Int_t *vVec, *vConsider;
  int entry, index;
  vConsider = (Vec_Int_t *)Vec_PtrEntryLast(_quanBlock);
  // collect shared variables
  auto shared_push = [&shared_vector](int v) { shared_vector.push_back(v); };
  Vec_PtrForEachEntry(Vec_Int_t *, _quanBlock, vVec, index) {
    if (index == Vec_PtrSize(_quanBlock) - 1)
      break;
    Vec_IntMap(vVec, shared_push);
  }
  avy::ItpMinisat* solver = createItpSolver(vConsider);

  // solving
  Vec_IntForEachEntry(vConsider, entry, index) {
    int pClause[2];
    int select_var = _nVar * 2 + 2 * index + 1;
    pClause[0] = toLitCond(select_var, true);
    pClause[1] = toLit(entry);
    solver->markPartition(1);
    solver->addClause(pClause, pClause + 2);
    pClause[0] = toLitCond(select_var + 1, true);
    pClause[1] = toLitCond(_nVar + entry, true);
    solver->markPartition(2);
    solver->addClause(pClause, pClause + 2);
    vector<int> assumptions = {toLit(select_var), toLit(select_var + 1)};
    boost::tribool result = solver->solve(assumptions, CONFLICT_LIMIT);
    if (!result) {
      Aig_Man_t *pAig =
          solver->getInterpolant(shared_vector, shared_vector.size());
      _definedVariables[entry] = buildDefinition(shared_vector, pAig);
    }
  }
}

ABC_NAMESPACE_IMPL_END
