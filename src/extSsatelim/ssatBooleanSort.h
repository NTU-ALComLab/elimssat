/**HFile****************************************************************
  FileName    [ssatBooleanSort.h]
  SystemName  []
  PackageName []
  Synopsis    []
  Author      [Kuan-Hua Tu]
  
  Affiliation []
  Date        [Sat Apr 11 00:20:25 CST 2020]
***********************************************************************/

#ifndef ABC__ext__ssatBooleanSort_h
#define ABC__ext__ssatBooleanSort_h

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "base/main/main.h"
#include "bdd/extrab/extraBdd.h" // 

////////////////////////////////////////////////////////////////////////
///                         DECLARATION                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

extern Abc_Ntk_t * Util_FunctionSortSeq(Abc_Ntk_t * pNtk, Vec_Int_t * pScopeReverse);
extern Abc_Ntk_t * Util_FunctionSort(Abc_Ntk_t * pNtk, Vec_Int_t * pScopeReverse);
extern Abc_Ntk_t * Util_FunctionSortSeq_Select(Abc_Ntk_t * pNtk, Vec_Int_t * pScopeReverse, int method);
extern Abc_Ntk_t * Util_FunctionSort_Select(Abc_Ntk_t * pNtk, Vec_Int_t * pScopeReverse, int method);


extern Abc_Ntk_t * Util_FunctionSort_1bit( Abc_Ntk_t * pNtk, Vec_Int_t * pScope, int index_var );
extern Abc_Ntk_t * Util_FunctionSort_allbit( Abc_Ntk_t * pNtk, Vec_Int_t * pScope);
extern Abc_Ntk_t * Util_FunctionSort_block( Abc_Ntk_t * pNtk, Vec_Int_t * pScope );
extern Abc_Ntk_t * Util_NtkRandomQuantifyPis_BDD( Abc_Ntk_t * pNtk, Vec_Int_t * pScope );
extern Abc_Ntk_t * Util_NtkExistRandomQuantifyPis_BDD( Abc_Ntk_t * pNtk, Vec_Int_t * pScopeE, Vec_Int_t * pScopeR );

extern DdNode * block(DdManager * dd, DdNode * bFunc, Vec_Int_t * pScopeReverse);
extern DdNode * random_eliminate_reverse_all_bit2_BDD(DdManager * dd, DdNode * bFunc, Vec_Int_t * pScopeReverse);
extern Abc_Ntk_t * random_eliminate_reverse_all_bit2(Abc_Ntk_t * pNtk, Vec_Int_t * pScopeReverse);
extern DdNode * sort(DdManager ** dd, DdNode * bFunc, Vec_Int_t * pScope);

ABC_NAMESPACE_HEADER_END
#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
