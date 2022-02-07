#ifndef ABC__ext__utilSat_h
#define ABC__ext__utilSat_h
#include "base/main/main.h"
#include "sat/bsat/satSolver.h"
#include "sat/cnf/cnf.h"
// sat_solver
extern int sat_solver_add_conditional_equal( sat_solver * pSat, int iVar, int iVar2, int iVarCond );
extern int sat_solver_add_conditional_nonequal( sat_solver * pSat, int iVar, int iVar2, int iVarCond );
extern int sat_solver_add_dual_conditional_equal( sat_solver * pSat, int iVar, int iVar2, int iVarCond1, int iVarCond2 );
extern int sat_solver_add_equal( sat_solver * pSat, int iVar, int iVar2 );
extern int sat_solver_add_dual_clause( sat_solver * pSat, lit litA, lit litB );
extern int sat_solver_addclause_from( sat_solver* pSat, Cnf_Dat_t * pCnf );
extern void sat_solver_print( sat_solver* pSat, int fDimacs );
#endif
