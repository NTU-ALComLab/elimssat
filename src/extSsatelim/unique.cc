#include "ssatParser.h"
#include <functional>
#include <unordered_set>

ABC_NAMESPACE_IMPL_START

using std::unordered_set;
using std::vector;

static const int CONFLICT_LIMIT = 1000;

static void Vec_IntMap(Vec_Int_t *vVec, std::function<void(int)> func) {
  int entry, index;
  Vec_IntForEachEntry(vVec, entry, index) { func(entry); }
}

/**
 * @brief perform interpolation based semantic gate extraction
 * 
 * For more information, see
 * - Interpolation-Based Semantic Gate Extraction and Its Application to QBF Preprocessing
 *   published on CAV 2020
 */
void ssatParser::uniquePreprocess() {
  val_assert_msg(Vec_IntSize(_quanType) != 0 && Vec_PtrSize(_quanBlock) != 0,
                 "no quantifier to be processed!");
  val_assert_msg(Vec_PtrSize(_clauseSet) != 0, "no clause to be processed!");
  // TODO add general support
  if (Vec_IntEntryLast(_quanType) != Quantifier_Exist)
    return;

  vector<int> selectionVariables;
  Vec_Int_t *vVec, *vConsider;
  int entry, index;
  vConsider = (Vec_Int_t *)Vec_PtrEntryLast(_quanBlock);
  unordered_set<int> shared_set;
  unordered_set<int> consider_set;
  auto shared_insert = [&shared_set](int v) { shared_set.insert(v); };
  auto consider_insert = [&consider_set](int v) { consider_set.insert(v); };
  Vec_IntMap(vConsider, consider_insert);
  // collect shared variables
  Vec_PtrForEachEntry(Vec_Int_t *, _quanBlock, vVec, index) {
    if (index == Vec_PtrSize(_quanBlock) - 1)
      break;
    Vec_IntMap(vVec, shared_insert);
  }
  // create ItpMinisat solver
  _itpSolver = new avy::ItpMinisat(2, 2 * _nVar + consider_set.size() * 2);
  _itpSolver->markPartition(1);
  // when inserting the clause, convert to minisat format
  Vec_Int_t *vClause = Vec_IntAlloc(0);
  auto convert_clause = [&vClause](int l) {
    int var = abs(l);
    int lit = signbit(l) ? 2 * var : 2 * var + 1;
    Vec_IntPush(vClause, lit);
  };
  Vec_PtrForEachEntry(Vec_Int_t *, _clauseSet, vVec, index) {
    Vec_IntMap(vVec, convert_clause);
    int j;
    Vec_IntForEachEntry(vClause, entry, j) {
    }
    _itpSolver->addClause(vClause->pArray,
                          vClause->pArray + Vec_IntSize(vClause));
    Vec_IntClear(vClause);
  }
  // when adding the second part, make a new copy for every consider variables
  _itpSolver->markPartition(2);
  int n = _nVar;
  auto convert_clause_copy = [&vClause, &consider_set, n](int l) {
    int var = consider_set.find(abs(l)) != consider_set.end()
                  ? abs(l) + n
                  : abs(l);
    int lit = signbit(l) ? 2 * var : 2 * var + 1;
    Vec_IntPush(vClause, lit);
  };
  Vec_PtrForEachEntry(Vec_Int_t *, _clauseSet, vVec, index) {
    Vec_IntMap(vVec, convert_clause_copy);
    int j;
    Vec_IntForEachEntry(vClause, entry, j) {
    }
    _itpSolver->addClause(vClause->pArray,
                          vClause->pArray + Vec_IntSize(vClause));
    Vec_IntClear(vClause);
  }
  // add selector clause
  Vec_IntForEachEntry(vConsider, entry, index) {
    int pClause[2];
    pClause[0] = toLit(entry);
    pClause[1] = toLitCond(2 * _nVar + 2 * index + 1, true);
    selectionVariables.push_back(2 * _nVar + 2 * index + 1);
    _itpSolver->markPartition(1);
    _itpSolver->addClause(pClause, pClause + 2);
    pClause[0] = toLitCond(entry + _nVar, true);
    pClause[1] = toLitCond(2 * _nVar + 2 * index + 2, true);
    selectionVariables.push_back(2 * _nVar + 2 * index + 2);
    _itpSolver->markPartition(2);
    _itpSolver->addClause(pClause, pClause + 2);
  }
  // solving
  Vec_IntForEachEntry(vConsider, entry, index) {
    vector<int> assumptions;
    printf("%d/%d ", entry + 1, Vec_IntSize(vConsider));
    std::transform(selectionVariables.begin(), selectionVariables.end(),
                   std::back_inserter(assumptions),
                   [index, n](int v) -> int {
                     int j = 2 * n + 2 * index;
                     return toLitCond(v, (j + 1 != v) && (j + 2 != v));
                   });
    boost::tribool result = _itpSolver->solve(assumptions, CONFLICT_LIMIT);
    if (!result) {
      printf("UNSAT\n");
    } else {
      printf("SAT\n");
    }
  }
}

ABC_NAMESPACE_IMPL_END
