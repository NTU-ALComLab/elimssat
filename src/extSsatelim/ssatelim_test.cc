/**CFile****************************************************************
  FileName    [ssatelim_test.cc]
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

#include "ssatelim.h"
#include "extUtil/util.h"
#include "ssatBooleanSort.h"
#include "extUnitTest/catch.hpp"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

//TEST_CASE( "function sort correction, part 3", "[ssatelim]" )
//{
//  Vec_Int_t * pScope = Vec_IntAlloc(0);
//  SECTION ( "1 block" )
//  {
//    const int input_num = 3;
//    const int sorted_bits_num = 1;
//    for (int i = input_num - sorted_bits_num; i < input_num; i++)
//    {
//      Vec_IntPush(pScope, i);
//    }

//    Abc_Ntk_t * pNtkRand = Util_CreatNtkFromTruthBin("00011011");
//    Abc_Ntk_t * pNtkRandBDD = Util_NtkCollapse(pNtkRand, 0);
//    REQUIRE( Abc_NtkIsBddLogic(pNtkRandBDD) == 1 );

//    Abc_Ntk_t * pNtkSorted = Util_FunctionSort_block(pNtkRandBDD, pScope);
//    Abc_Ntk_t * pNtkSortedStrash = Abc_NtkStrash( pNtkSorted, 0, 0, 0 );
//    REQUIRE( Abc_NtkIsStrash(pNtkSortedStrash) == 1 );

//    Abc_Ntk_t * pNtkResult = Util_CreatNtkFromTruthBin("00101011");

//    // Util_PrintTruthtable(pNtkSortedStrash) ;
    
//    REQUIRE( Util_NtkIsEqual(pNtkSortedStrash, pNtkResult) == 1 );
//    Abc_NtkDelete(pNtkRand);
//    Abc_NtkDelete(pNtkRandBDD);
//    Abc_NtkDelete(pNtkSorted);
//    Abc_NtkDelete(pNtkSortedStrash);
//    Abc_NtkDelete(pNtkResult);
//  }
//  SECTION ( "2 block" )
//  {
//    const int input_num = 3;
//    const int sorted_bits_num = 2;
//    for (int i = input_num - sorted_bits_num; i < input_num; i++)
//    {
//      Vec_IntPush(pScope, i);
//    }

//    Abc_Ntk_t * pNtkRand = Util_CreatNtkFromTruthBin("00011011");
//    Abc_Ntk_t * pNtkRandBDD = Util_NtkCollapse(pNtkRand, 0);
//    REQUIRE( Abc_NtkIsBddLogic(pNtkRandBDD) == 1 );

//    Abc_Ntk_t * pNtkSorted = Util_FunctionSort_block(pNtkRandBDD, pScope);
//    Abc_Ntk_t * pNtkSortedStrash = Abc_NtkStrash( pNtkSorted, 0, 0, 0 );
//    REQUIRE( Abc_NtkIsStrash(pNtkSortedStrash) == 1 );

//    Abc_Ntk_t * pNtkResult = Util_CreatNtkFromTruthBin("10001110");

//    // Util_PrintTruthtable(pNtkSortedStrash) ;
    
//    REQUIRE( Util_NtkIsEqual(pNtkSortedStrash, pNtkResult) == 1 );
//    Abc_NtkDelete(pNtkRand);
//    Abc_NtkDelete(pNtkRandBDD);
//    Abc_NtkDelete(pNtkSorted);
//    Abc_NtkDelete(pNtkSortedStrash);
//    Abc_NtkDelete(pNtkResult);
//  }
//  SECTION ( "3 block" )
//  {
//    const int input_num = 3;
//    const int sorted_bits_num = 3;
//    for (int i = input_num - sorted_bits_num; i < input_num; i++)
//    {
//      Vec_IntPush(pScope, i);
//    }

//    Abc_Ntk_t * pNtkRand = Util_CreatNtkFromTruthBin("00011011");
//    Abc_Ntk_t * pNtkRandBDD = Util_NtkCollapse(pNtkRand, 0);
//    REQUIRE( Abc_NtkIsBddLogic(pNtkRandBDD) == 1 );

//    Abc_Ntk_t * pNtkSorted = Util_FunctionSort_block(pNtkRandBDD, pScope);
//    Abc_Ntk_t * pNtkSortedStrash = Abc_NtkStrash( pNtkSorted, 0, 0, 0 );
//    REQUIRE( Abc_NtkIsStrash(pNtkSortedStrash) == 1 );

//    Abc_Ntk_t * pNtkResult = Util_CreatNtkFromTruthBin("11101000");

//    // Util_PrintTruthtable(pNtkSortedStrash) ;
    
//    REQUIRE( Util_NtkIsEqual(pNtkSortedStrash, pNtkResult) == 1 );
//    Abc_NtkDelete(pNtkRand);
//    Abc_NtkDelete(pNtkRandBDD);
//    Abc_NtkDelete(pNtkSorted);
//    Abc_NtkDelete(pNtkSortedStrash);
//    Abc_NtkDelete(pNtkResult);
//  }
//  Vec_IntFree(pScope);
//}

//TEST_CASE( "function sort correction", "[ssatelim]" )
//{
//  Vec_Int_t * pScope = Vec_IntAlloc(0);
//  SECTION ( "main" )
//  {
//    const int input_num = 4;
//    for (int i = 0; i < input_num; i++)
//    {
//      Vec_IntPush(pScope, i);
//    }

//    Abc_Ntk_t * pNtkRand = Util_CreatNtkFromTruthBin("1010101010010010");
//    pNtkRand = Util_FunctionSort(pNtkRand, pScope);
//    Abc_Ntk_t * pNtkResult = Util_CreatNtkFromTruthBin("1111111000000000");
    
//    REQUIRE( Util_NtkIsEqual(pNtkRand, pNtkResult) == 1 );
//    Abc_NtkDelete(pNtkRand);
//    Abc_NtkDelete(pNtkResult);
//  }
//  SECTION ( "1 input" )
//  {
//    const int input_num = 1;
//    for (int i = 0; i < input_num; i++)
//    {
//      Vec_IntPush(pScope, i);
//    }
//    Abc_Ntk_t * pNtkRand = Util_CreatNtkFromTruthBin("01");
//    pNtkRand = Util_FunctionSort(pNtkRand, pScope);
//    Abc_Ntk_t * pNtkResult = Util_CreatNtkFromTruthBin("10");
    
//    REQUIRE( Util_NtkIsEqual(pNtkRand, pNtkResult) == 1 );
//    Abc_NtkDelete(pNtkRand);
//    Abc_NtkDelete(pNtkResult);
//  }
//  SECTION ( "BDD method 1 input" )
//  {
//    const int input_num = 1;
//    for (int i = 0; i < input_num; i++)
//    {
//      Vec_IntPush(pScope, i);
//    }

//    Abc_Ntk_t * pNtkRand = Util_CreatNtkFromTruthBin("01");
//    Abc_Ntk_t * pNtkRandBDD = Util_NtkCollapse(pNtkRand, 0);
//    REQUIRE( Abc_NtkIsBddLogic(pNtkRandBDD) == 1 );

//    pNtkRandBDD = Util_NtkRandomQuantifyPis_BDD(pNtkRandBDD, pScope);
//    REQUIRE( Abc_NtkIsBddLogic(pNtkRandBDD) == 1 );

//    pNtkRandBDD = Abc_NtkStrash( pNtkRandBDD, 0, 0, 0 );
//    REQUIRE( Abc_NtkIsStrash(pNtkRandBDD) == 1 );
//    Abc_Ntk_t * pNtkResult = Util_CreatNtkFromTruthBin("10");
    
//    REQUIRE( Util_NtkIsEqual(pNtkRandBDD, pNtkResult) == 1 );
//    Abc_NtkDelete(pNtkRand);
//    Abc_NtkDelete(pNtkResult);
//  }
//  SECTION ( "BDD method 2 input" )
//  {
//    const int input_num = 2;
//    for (int i = 0; i < input_num; i++)
//    {
//      Vec_IntPush(pScope, i);
//    }

//    Abc_Ntk_t * pNtkRand = Util_CreatNtkFromTruthBin("0101");
//    Abc_Ntk_t * pNtkRandBDD = Util_NtkCollapse(pNtkRand, 0);
//    REQUIRE( Abc_NtkIsBddLogic(pNtkRandBDD) == 1 );

//    pNtkRandBDD = Util_NtkRandomQuantifyPis_BDD(pNtkRandBDD, pScope);
//    REQUIRE( Abc_NtkIsBddLogic(pNtkRandBDD) == 1 );

//    pNtkRandBDD = Abc_NtkStrash( pNtkRandBDD, 0, 0, 0 );
//    REQUIRE( Abc_NtkIsStrash(pNtkRandBDD) == 1 );
//    Abc_Ntk_t * pNtkResult = Util_CreatNtkFromTruthBin("1100");

//    // Util_PrintTruthtable(pNtkRandBDD) ;
    
//    REQUIRE( Util_NtkIsEqual(pNtkRandBDD, pNtkResult) == 1 );
//    Abc_NtkDelete(pNtkRand);
//    Abc_NtkDelete(pNtkResult);
//  }
//  Vec_IntClear(pScope);
//  SECTION ( "BDD method 3 input" )
//  {
//    const int input_num = 2;
//    for (int i = 0; i < input_num; i++)
//    {
//      Vec_IntPush(pScope, i);
//    }

//    Abc_Ntk_t * pNtkRand = Util_CreatNtkFromTruthBin("01011101");
//    Abc_Ntk_t * pNtkRandBDD = Util_NtkCollapse(pNtkRand, 0);
//    REQUIRE( Abc_NtkIsBddLogic(pNtkRandBDD) == 1 );

//    pNtkRandBDD = Util_NtkRandomQuantifyPis_BDD(pNtkRandBDD, pScope);
//    REQUIRE( Abc_NtkIsBddLogic(pNtkRandBDD) == 1 );

//    pNtkRandBDD = Abc_NtkStrash( pNtkRandBDD, 0, 0, 0 );
//    REQUIRE( Abc_NtkIsStrash(pNtkRandBDD) == 1 );
//    Abc_Ntk_t * pNtkResult = Util_CreatNtkFromTruthBin("11010101");

//    // Util_PrintTruthtable(pNtkRandBDD) ;
    
//    REQUIRE( Util_NtkIsEqual(pNtkRandBDD, pNtkResult) == 1 );
//    Abc_NtkDelete(pNtkRand);
//    Abc_NtkDelete(pNtkResult);
//  }
//  Vec_IntClear(pScope);
//  SECTION ("AIG method 2 input")
//  {
//    printf("TESTING sorting in AIG of 2 input\n");
//    const int input_num = 2;
//    for (int i = 0; i < input_num; i++)
//    {
//      Vec_IntPush(pScope, i);
//    }

//    Abc_Ntk_t * pNtkRand = Util_CreatNtkFromTruthBin("0101");
//    pNtkRand = Util_FunctionSort2(pNtkRand, pScope, 2);
//    Abc_Ntk_t * pNtkResult = Util_CreatNtkFromTruthBin("1100");
//    REQUIRE( Util_NtkIsEqual(pNtkRand, pNtkResult) == 1 );
//    Abc_NtkDelete(pNtkRand);
//    Abc_NtkDelete(pNtkResult);
//  }
//  Vec_IntClear(pScope);
//  SECTION ("AIG method 3 input")
//  {
//    printf("TESTING sorting in AIG of 3 input\n");
//    const int input_num = 2;
//    for (int i = 0; i < input_num; i++)
//    {
//      Vec_IntPush(pScope, i);
//    }

//    Abc_Ntk_t * pNtkRand = Util_CreatNtkFromTruthBin("01011101");
//    pNtkRand = Util_FunctionSort2(pNtkRand, pScope, 1);
//    Abc_Ntk_t * pNtkResult = Util_CreatNtkFromTruthBin("11010101");
//    REQUIRE( Util_NtkIsEqual(pNtkRand, pNtkResult) == 1 );
//    Abc_NtkDelete(pNtkRand);
//    Abc_NtkDelete(pNtkResult);
//  }
//  Vec_IntFree(pScope);
//}

//TEST_CASE( "function sort correction test on real instance", "[ssatelim]" )
//{
//  SECTION ("Random elimination")
//  {
//    int index, entry;
//    char filename[256] = "benchmarks/ssatER/planning/ToiletA/sdimacs/toilet_a_02_01.2.sdimacs";
//    // char filename[256] = "benchmarks/ssatER/planning/sand-castle//sdimacs-05/SC-5.sdimacs";
//    ssat_solver* s = ssat_solver_new();
//    ssat_Parser(s, filename);
//    ssat_parser_finished_process(s, filename);
//    ssat_build_bdd(s);
//    Abc_Ntk_t *pNtk = Abc_NtkDeriveFromBdd(s->dd, s->bFunc, NULL, NULL);
//    pNtk = Util_NtkStrash(pNtk, 1);
//    Vec_Int_t *pScope = (Vec_Int_t *)Vec_PtrEntryLast(s->pQuan);
//    Vec_Int_t *pRandomReverse = Vec_IntAlloc(0);
//    Vec_IntForEachEntry(pScope, entry, index) {
//      Vec_IntPush(pRandomReverse, entry);
//    }
//    // s->bFunc = sort(&s->dd, s->bFunc, pRandomReverse);
//    pNtk = Util_FunctionSort(pNtk, pRandomReverse);
//    Abc_Ntk_t *pNtkBdd = Abc_NtkDeriveFromBdd(s->dd, s->bFunc, NULL, NULL);
//    pNtkBdd = Util_NtkStrash(pNtkBdd, 1);
//    REQUIRE( Util_NtkIsEqual(pNtk, pNtkBdd) == 1 );
//  }
//}

////ABC_NAMESPACE_IMPL_END
////////////////////////////////////////////////////////////////////////////
///////                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////////
