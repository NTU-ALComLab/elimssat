#include "extUtil/easylogging++.h"
#include "extUtil/utilVec.h"
#include "proof/fra/fra.h"
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
    // printf("%d ", pArray[i]);
    printf("%s%d ", lit_sign(pArray[i]) ? "-" : "", lit_var(pArray[i]));
  }
  printf("\n");
}

/**
 * @brief perform basic synthesis on the extracted definition circuits
 *
 * @return the Aig manager after synthesis
 */
static Aig_Man_t *synthesisDefinition(Aig_Man_t *pAig) {
  if (Aig_ManNodeNum(pAig) < 100) {
    return pAig;
  }
  VLOG(1) << "size of definition before compress: " << Aig_ManNodeNum(pAig);
  Aig_Man_t *pAigNew = Dar_ManCompress2(pAig, 1, 0, 1, 0, 0);
  Aig_ManStop(pAig);
  VLOG(1) << "size of definition after compress: " << Aig_ManNodeNum(pAigNew);
  if (Aig_ManNodeNum(pAigNew) > 500) {
    Fra_Par_t pars, *p_pars = &pars;
    Fra_ParamsDefault(p_pars);
    pAig = pAigNew;
    pAigNew = Fra_FraigPerform(pAig, p_pars);
    Aig_ManStop(pAig);
    VLOG(1) << "Size after fraiging: " << Aig_ManNodeNum(pAigNew);
  }
  return pAigNew;
}

void ssatParser::collectVariable(vector<int> &consider_vector,
                                 vector<int> &shared_vector,
                                 unordered_set<int> &exist_map) {
  int index;
  Vec_Int_t *vVec;
  auto shared_push = [&shared_vector](int v) { shared_vector.push_back(v); };
  auto consider_push = [&consider_vector](int v) {
    consider_vector.push_back(v);
  };
  auto map_insert = [&exist_map](int v) { exist_map.insert(v); };
  vVec = (Vec_Int_t *)Vec_PtrEntry(_quanBlock, 0);
  Vec_PtrForEachEntry(Vec_Int_t *, _quanBlock, vVec, index) {
    int type = Vec_IntEntry(_quanType, index);
    if (index == 0)
      Vec_IntMap(vVec, shared_push);
    else if (index == 1 && type == Quantifier_Random)
      Vec_IntMap(vVec, shared_push);
    else {
      Vec_IntMap(vVec, consider_push);
    }
    if (type == Quantifier_Exist) {
      Vec_IntMap(vVec, map_insert);
    }
  }
}

/**
 * @brief add clauses to the interpolation solver
 */
avy::ItpMinisat *ssatParser::createItpSolver(vector<int> &consider_vector) {
  avy::ItpMinisat *solver = new avy::ItpMinisat(2, 1, false);
  unordered_set<int> consider_set;
  Vec_Int_t *vVec;
  int index;
  // create a hash table consist of considered variables
  for (auto &i : consider_vector) {
    consider_set.insert(i);
  }
  solver->reserve(2 * _nVar + 2 * consider_vector.size() + 1);
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
    if (Vec_IntSize(vClause) == 1) {
      solver->addUnit(vClause->pArray[0]);
    } else {
      solver->addClause(vClause->pArray,
                        vClause->pArray + Vec_IntSize(vClause));
    }
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
    if (Vec_IntSize(vClause) == 1) {
      solver->addUnit(vClause->pArray[0]);
    } else {
      solver->addClause(vClause->pArray,
                        vClause->pArray + Vec_IntSize(vClause));
    }
    Vec_IntClear(vClause);
  }
  return solver;
}

Abc_Obj_t *ssatParser::buildDefinition(vector<int> &shared_vector,
                                       Aig_Man_t *pAig) {
  int index;
  Aig_Obj_t *pObj;
  pAig = synthesisDefinition(pAig);
  Vec_Ptr_t *vNodes;
  vNodes = Aig_ManDfs(pAig, 1);
  Aig_ManConst1(pAig)->pData = Abc_AigConst1(_solver->pNtk);
  Aig_ManForEachCi(pAig, pObj, index) {
    int var = shared_vector[index];
    if (_definedVariables.find(var) != _definedVariables.end()) {
      pObj->pData = _definedVariables.at(var);
    } else {
      pObj->pData = Abc_NtkPi(_solver->pNtk, shared_vector[index] - 1);
    }
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
  if (Vec_IntEntryLast(_quanType) != Quantifier_Exist)
    return;

  VLOG(1) << "perform interpolation-based semantic gate extraction";
  vector<int> shared_vector;
  vector<int> consider_vector;
  unordered_set<int> exist_map;
  collectVariable(consider_vector, shared_vector, exist_map);
  VLOG(1) << "size of shared_vector: " << shared_vector.size();
  VLOG(1) << "size of considered_vector: " << consider_vector.size();
  avy::ItpMinisat *solver = createItpSolver(consider_vector);
  vector<int> temp = {};
  boost::tribool is_sat = solver->solve(temp, CONFLICT_LIMIT);
  if (!is_sat) {
    VLOG(1) << "matrix unsat";
    std::cout << std::endl;
    std::cout << "> Result = 0" << std::endl;
    exit(0);
  }

  // solving
  vector<int> select_vars;
  int select_var = _nVar * 2 + 1;
  for (int index = 0; index < consider_vector.size(); index++) {
    int entry = consider_vector[index];
    VLOG(1) << index + 1 << "/" << consider_vector.size() << ": " << entry << " size of shared_vector: " << shared_vector.size();
    int pClause[2];
    if (exist_map.find(entry) != exist_map.end()) {
      pClause[0] = toLitCond(select_var, true);
      pClause[1] = toLit(entry);
      solver->markPartition(1);
      solver->addClause(pClause, pClause + 2);
      pClause[0] = toLitCond(select_var + 1, true);
      pClause[1] = toLitCond(_nVar + entry, true);
      solver->markPartition(2);
      solver->addClause(pClause, pClause + 2);
      vector<int> assumptions = {toLit(select_var), toLit(select_var + 1)};
      // for (int &v : select_vars) {
      //   assumptions.push_back(toLitCond(v, true));
      // }
      select_vars.push_back(select_var);
      select_vars.push_back(select_var + 1);
      select_var += 2;
      boost::tribool result = solver->solve(assumptions, CONFLICT_LIMIT);
      if (!result) {
        Aig_Man_t *pAig =
            solver->getInterpolant(shared_vector, shared_vector.size());
        _definedVariables[entry] = buildDefinition(shared_vector, pAig);
      }
    }
    pClause[0] = toLit(entry);
    pClause[1] = toLitCond(entry + _nVar, true);
    solver->addClause(pClause, pClause + 2);
    pClause[0] = toLit(entry + _nVar);
    pClause[1] = toLitCond(entry, true);
    solver->addClause(pClause, pClause + 2);
    shared_vector.push_back(entry);
  }
  VLOG(1) << "found " << _definedVariables.size() << " definitions";
}

ABC_NAMESPACE_IMPL_END
