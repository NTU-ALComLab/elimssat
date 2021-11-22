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
#include "extUnitTest/catch.hpp"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

TEST_CASE( "network in ssat solver", "[ssatelim]" )
{
  ssat_solver* s = ssat_solver_new();
  SECTION ( "const 1" )
  {
    Abc_Ntk_t * pNtkConst = Util_CreatConstant1Ntk(1);
    ssat_solver_setnvars(s, 1);
    int a = 1;
    ssat_addrandom(s, &a, &a + 1, 0.5);
    ssat_parser_finished_process(s);

    REQUIRE( Util_NtkIsEqual(s->pNtk, pNtkConst) == 1 );
    
    Abc_NtkDelete(pNtkConst);
  }
  SECTION ( "const 0" )
  {
    Abc_Ntk_t * pNtkConst = Util_CreatConstant0Ntk(1);
    ssat_solver_setnvars(s, 1);
    int a = 1;
    ssat_addrandom(s, &a, &a + 1, 0.5);
    ssat_addclause(s, &a, &a);

    ssat_parser_finished_process(s);

    REQUIRE( Util_NtkIsEqual(s->pNtk, pNtkConst) == 1 );
    
    Abc_NtkDelete(pNtkConst);
  }
  ssat_solver_delete(s);
}

TEST_CASE( "ssat solver initialization", "[ssatelim]" )
{
  ssat_solver* s = ssat_solver_new();
  SECTION ( "init result" )
  {
    REQUIRE( ssat_result(s) < 0 );
  }
  ssat_solver_delete(s);
}

TEST_CASE( "ssat solve", "[ssatelim]" )
{
  ssat_solver* s = ssat_solver_new();
  SECTION ( "existence-only with empty clause" )
  {
    // E 1, 2. ()
    ssat_solver_setnvars(s, 2);
    lit exist[] = { 1, 2 };
    lit clause[] = { }; // ()
    ssat_addexistence(s, exist, exist + 2);
    ssat_addclause(s, clause, clause);

    ssat_solver_solve2(s);

    REQUIRE_THAT( ssat_result(s), Catch::WithinRel(0.0) );
  }
  SECTION ( "existence-only with a clause" )
  {
    // E 1, 2. (1 + 2)
    ssat_solver_setnvars(s, 2);
    lit exist[] = { 1, 2 };
    lit clause[] = { 2, 4 }; // (1 + 2)
    ssat_addexistence(s, exist, exist + 2);
    ssat_addclause(s, clause, clause + 2);

    ssat_solver_solve2(s);

    REQUIRE_THAT( ssat_result(s), Catch::WithinRel(1.0) );
  }
  SECTION ( "random-only with empty clause" )
  {
    // R 1, 2. ()
    ssat_solver_setnvars(s, 2);
    lit random[] = { 1, 2 };
    lit clause[] = { }; // ()
    ssat_addrandom(s, random, random + 2, 0.5);
    ssat_addclause(s, clause, clause);

    ssat_solver_solve2(s);

    REQUIRE_THAT( ssat_result(s), Catch::WithinRel(0.0) );
  }
  SECTION ( "random-only with a clause" )
  {
    // R 1, 2. (1 + 2)
    ssat_solver_setnvars(s, 2);
    lit random[] = { 1, 2 };
    lit clause[] = { 2, 4 }; // (1 + 2)
    ssat_addrandom(s, random, random + 2, 0.5);
    ssat_addclause(s, clause, clause + 2);

    ssat_solver_solve2(s);

    REQUIRE_THAT( ssat_result(s), Catch::WithinRel(0.75) );
  }
  SECTION ( "random-only with 2 clause" )
  {
    // R 1, 2. (1)(2)
    ssat_solver_setnvars(s, 2);
    lit random[] = { 1, 2 };
    lit clause0[] = { 2 }; // (1)
    lit clause1[] = { 4 }; // (2)
    ssat_addrandom(s, random, random + 2, 0.5);
    ssat_addclause(s, clause0, clause0 + 1);
    ssat_addclause(s, clause1, clause1 + 1);

    ssat_solver_solve2(s);

    REQUIRE_THAT( ssat_result(s), Catch::WithinRel(0.25) );
  }
  SECTION ( "exist-random with empty clause" )
  {
    // E 1, R 2. ()
    ssat_solver_setnvars(s, 2);
    lit exist[] = { 1 };
    lit random[] = { 2 };
    lit clause[] = { }; // ()
    ssat_addexistence(s, exist, exist + 1);
    ssat_addrandom(s, random, random + 1, 0.5);
    ssat_addclause(s, clause, clause);

    ssat_solver_solve2(s);

    REQUIRE_THAT( ssat_result(s), Catch::WithinRel(0.0) );
  }
  SECTION ( "exist-random with a clause" )
  {
    // E 1, R 2. (1 + 2)
    ssat_solver_setnvars(s, 2);
    lit exist[] = { 1 };
    lit random[] = { 2 };
    lit clause[] = { 2, 4 }; // (1 + 2)
    ssat_addexistence(s, exist, exist + 1);
    ssat_addrandom(s, random, random + 1, 0.5);
    ssat_addclause(s, clause, clause+2);

    ssat_solver_solve2(s);

    REQUIRE_THAT( ssat_result(s), Catch::WithinRel(1.0) );
  }
  SECTION ( "random-exist with a clause" )
  {
    // R 1, E 2. (1 + 2)
    ssat_solver_setnvars(s, 2);
    lit random[] = { 2 };
    lit exist[] = { 1 };
    lit clause[] = { 2, 4 }; // (1 + 2)
    ssat_addrandom(s, random, random + 1, 0.5);
    ssat_addexistence(s, exist, exist + 1);
    ssat_addclause(s, clause, clause+2);

    ssat_solver_solve2(s);

    REQUIRE_THAT( ssat_result(s), Catch::WithinRel(1.0) );
  }
  SECTION ( "random-exist with 2 clause" )
  {
    // R 1, E 2. (1)(2)
    ssat_solver_setnvars(s, 2);
    lit random[] = { 2 };
    lit exist[] = { 1 };
    lit clause0[] = { 2 }; // (1)
    lit clause1[] = { 4 }; // (2)
    ssat_addrandom(s, random, random + 1, 0.5);
    ssat_addexistence(s, exist, exist + 1);
    ssat_addclause(s, clause0, clause0 + 1);
    ssat_addclause(s, clause1, clause1 + 1);

    ssat_solver_solve2(s);

    REQUIRE_THAT( ssat_result(s), Catch::WithinRel(0.5) );
  }
  SECTION ( "random-exist-random with 2 clause" )
  {
    // R 1, E 2, R 3,. (1)(2 + 3)
    ssat_solver_setnvars(s, 3);
    lit random1[] = { 1 };
    lit exist[] = { 2 };
    lit random2[] = { 3 };
    lit clause0[] = { 2 }; // (1)
    lit clause1[] = { 4, 6 }; // (2 + 3)
    ssat_addrandom(s, random1, random1 + 1, 0.5);
    ssat_addexistence(s, exist, exist + 1);
    ssat_addrandom(s, random2, random2 + 1, 0.5);
    ssat_addclause(s, clause0, clause0 + 1);
    ssat_addclause(s, clause1, clause1 + 2);

    ssat_solver_solve2(s);

    REQUIRE_THAT( ssat_result(s), Catch::WithinRel(0.5) );
  }
  SECTION ( "exist-random-exist with 3 clause" )
  {
    // E 1, R 2, E 3,. (1)(2 + 3)(-2)
    ssat_solver_setnvars(s, 3);
    lit exist1[] = { 1 };
    lit random[] = { 2 };
    lit exist2[] = { 3 };
    lit clause0[] = { 2 }; // (1)
    lit clause1[] = { 4, 6 }; // (2 + 3)
    lit clause2[] = { 5 }; // (-2)
    ssat_addexistence(s, exist1, exist1 + 1);
    ssat_addrandom(s, random, random + 1, 0.5);
    ssat_addexistence(s, exist2, exist2 + 1);
    ssat_addclause(s, clause0, clause0 + 1);
    ssat_addclause(s, clause1, clause1 + 2);
    ssat_addclause(s, clause2, clause2 + 1);

    ssat_solver_solve2(s);

    REQUIRE_THAT( ssat_result(s), Catch::WithinRel(0.5) );
  }
  ssat_solver_delete(s);
}

TEST_CASE( "function sort correction, part 1", "[ssatelim]" )
{
  Vec_Int_t * pScope = Vec_IntAlloc(0);
  SECTION ( "one bit sort, bit 0" )
  {
    const int input_num = 3;
    const int sorted_bit = 0;
    for (int i = 0; i < input_num; i++)
    {
      Vec_IntPush(pScope, i);
    }

    Abc_Ntk_t * pNtkRand = Util_CreatNtkFromTruthBin("00011011");
    Abc_Ntk_t * pNtkRandBDD = Util_NtkCollapse(pNtkRand, 0);
    REQUIRE( Abc_NtkIsBddLogic(pNtkRandBDD) == 1 );

    Abc_Ntk_t * pNtkSorted = Util_FunctionSort_1bit(pNtkRandBDD, pScope, sorted_bit);
    Abc_Ntk_t * pNtkSortedStrash = Abc_NtkStrash( pNtkSorted, 0, 0, 0 );
    REQUIRE( Abc_NtkIsStrash(pNtkSortedStrash) == 1 );

    Abc_Ntk_t * pNtkResult = Util_CreatNtkFromTruthBin("10110001");

    // Util_PrintTruthtable(pNtkSortedStrash) ;
    
    REQUIRE( Util_NtkIsEqual(pNtkSortedStrash, pNtkResult) == 1 );
    Abc_NtkDelete(pNtkRand);
    Abc_NtkDelete(pNtkRandBDD);
    Abc_NtkDelete(pNtkSorted);
    Abc_NtkDelete(pNtkSortedStrash);
    Abc_NtkDelete(pNtkResult);
  }
  SECTION ( "one bit sort, bit 1" )
  {
    const int input_num = 3;
    const int sorted_bit = 1;
    for (int i = 0; i < input_num; i++)
    {
      Vec_IntPush(pScope, i);
    }

    Abc_Ntk_t * pNtkRand = Util_CreatNtkFromTruthBin("00011011");
    Abc_Ntk_t * pNtkRandBDD = Util_NtkCollapse(pNtkRand, 0);
    REQUIRE( Abc_NtkIsBddLogic(pNtkRandBDD) == 1 );

    Abc_Ntk_t * pNtkSorted = Util_FunctionSort_1bit(pNtkRandBDD, pScope, sorted_bit);
    Abc_Ntk_t * pNtkSortedStrash = Abc_NtkStrash( pNtkSorted, 0, 0, 0 );
    REQUIRE( Abc_NtkIsStrash(pNtkSortedStrash) == 1 );

    Abc_Ntk_t * pNtkResult = Util_CreatNtkFromTruthBin("01001110");
    
    REQUIRE( Util_NtkIsEqual(pNtkSortedStrash, pNtkResult) == 1 );
    Abc_NtkDelete(pNtkRand);
    Abc_NtkDelete(pNtkRandBDD);
    Abc_NtkDelete(pNtkSorted);
    Abc_NtkDelete(pNtkSortedStrash);
    Abc_NtkDelete(pNtkResult);
  }
  SECTION ( "one bit sort, bit 2" )
  {
    const int input_num = 3;
    const int sorted_bit = 2;
    for (int i = 0; i < input_num; i++)
    {
      Vec_IntPush(pScope, i);
    }

    Abc_Ntk_t * pNtkRand = Util_CreatNtkFromTruthBin("00011011");
    Abc_Ntk_t * pNtkRandBDD = Util_NtkCollapse(pNtkRand, 0);
    REQUIRE( Abc_NtkIsBddLogic(pNtkRandBDD) == 1 );

    Abc_Ntk_t * pNtkSorted = Util_FunctionSort_1bit(pNtkRandBDD, pScope, sorted_bit);
    Abc_Ntk_t * pNtkSortedStrash = Abc_NtkStrash( pNtkSorted, 0, 0, 0 );
    REQUIRE( Abc_NtkIsStrash(pNtkSortedStrash) == 1 );

    Abc_Ntk_t * pNtkResult = Util_CreatNtkFromTruthBin("00101011");
    
    REQUIRE( Util_NtkIsEqual(pNtkSortedStrash, pNtkResult) == 1 );
    Abc_NtkDelete(pNtkRand);
    Abc_NtkDelete(pNtkRandBDD);
    Abc_NtkDelete(pNtkSorted);
    Abc_NtkDelete(pNtkSortedStrash);
    Abc_NtkDelete(pNtkResult);
  }
  Vec_IntFree(pScope);
}

TEST_CASE( "function sort correction, part 2", "[ssatelim]" )
{
  Vec_Int_t * pScope = Vec_IntAlloc(0);
  SECTION ( "all bit sort, 1 bit / 3" )
  {
    const int input_num = 3;
    const int sorted_bits_num = 1;
    for (int i = input_num - sorted_bits_num; i < input_num; i++)
    {
      Vec_IntPush(pScope, i);
    }

    Abc_Ntk_t * pNtkRand = Util_CreatNtkFromTruthBin("00011011");
    Abc_Ntk_t * pNtkRandBDD = Util_NtkCollapse(pNtkRand, 0);
    REQUIRE( Abc_NtkIsBddLogic(pNtkRandBDD) == 1 );

    Abc_Ntk_t * pNtkSorted = Util_FunctionSort_allbit(pNtkRandBDD, pScope);
    Abc_Ntk_t * pNtkSortedStrash = Abc_NtkStrash( pNtkSorted, 0, 0, 0 );
    REQUIRE( Abc_NtkIsStrash(pNtkSortedStrash) == 1 );

    Abc_Ntk_t * pNtkResult = Util_CreatNtkFromTruthBin("00101011");

    // Util_PrintTruthtable(pNtkSortedStrash) ;
    
    REQUIRE( Util_NtkIsEqual(pNtkSortedStrash, pNtkResult) == 1 );
    Abc_NtkDelete(pNtkRand);
    Abc_NtkDelete(pNtkRandBDD);
    Abc_NtkDelete(pNtkSorted);
    Abc_NtkDelete(pNtkSortedStrash);
    Abc_NtkDelete(pNtkResult);
  }
  SECTION ( "all bit sort, 2 bit / 3" )
  {
    const int input_num = 3;
    const int sorted_bits_num = 2;
    for (int i = input_num - sorted_bits_num; i < input_num; i++)
    {
      Vec_IntPush(pScope, i);
    }

    Abc_Ntk_t * pNtkRand = Util_CreatNtkFromTruthBin("00011011");
    Abc_Ntk_t * pNtkRandBDD = Util_NtkCollapse(pNtkRand, 0);
    REQUIRE( Abc_NtkIsBddLogic(pNtkRandBDD) == 1 );

    Abc_Ntk_t * pNtkSorted = Util_FunctionSort_allbit(pNtkRandBDD, pScope);
    Abc_Ntk_t * pNtkSortedStrash = Abc_NtkStrash( pNtkSorted, 0, 0, 0 );
    REQUIRE( Abc_NtkIsStrash(pNtkSortedStrash) == 1 );

    Abc_Ntk_t * pNtkResult = Util_CreatNtkFromTruthBin("10001101");

    // Util_PrintTruthtable(pNtkSortedStrash) ;
    
    REQUIRE( Util_NtkIsEqual(pNtkSortedStrash, pNtkResult) == 1 );
    Abc_NtkDelete(pNtkRand);
    Abc_NtkDelete(pNtkRandBDD);
    Abc_NtkDelete(pNtkSorted);
    Abc_NtkDelete(pNtkSortedStrash);
    Abc_NtkDelete(pNtkResult);
  }
  SECTION ( "all bit sort, 3 bit / 3" )
  {
    const int input_num = 3;
    const int sorted_bits_num = 3;
    for (int i = input_num - sorted_bits_num; i < input_num; i++)
    {
      Vec_IntPush(pScope, i);
    }

    Abc_Ntk_t * pNtkRand = Util_CreatNtkFromTruthBin("00011011");
    Abc_Ntk_t * pNtkRandBDD = Util_NtkCollapse(pNtkRand, 0);
    REQUIRE( Abc_NtkIsBddLogic(pNtkRandBDD) == 1 );

    Abc_Ntk_t * pNtkSorted = Util_FunctionSort_allbit(pNtkRandBDD, pScope);
    Abc_Ntk_t * pNtkSortedStrash = Abc_NtkStrash( pNtkSorted, 0, 0, 0 );
    REQUIRE( Abc_NtkIsStrash(pNtkSortedStrash) == 1 );

    Abc_Ntk_t * pNtkResult = Util_CreatNtkFromTruthBin("11011000");

    // Util_PrintTruthtable(pNtkSortedStrash) ;
    
    REQUIRE( Util_NtkIsEqual(pNtkSortedStrash, pNtkResult) == 1 );
    Abc_NtkDelete(pNtkRand);
    Abc_NtkDelete(pNtkRandBDD);
    Abc_NtkDelete(pNtkSorted);
    Abc_NtkDelete(pNtkSortedStrash);
    Abc_NtkDelete(pNtkResult);
  }
  Vec_IntFree(pScope);
}

TEST_CASE( "function sort correction, part 3", "[ssatelim]" )
{
  Vec_Int_t * pScope = Vec_IntAlloc(0);
  SECTION ( "1 block" )
  {
    const int input_num = 3;
    const int sorted_bits_num = 1;
    for (int i = input_num - sorted_bits_num; i < input_num; i++)
    {
      Vec_IntPush(pScope, i);
    }

    Abc_Ntk_t * pNtkRand = Util_CreatNtkFromTruthBin("00011011");
    Abc_Ntk_t * pNtkRandBDD = Util_NtkCollapse(pNtkRand, 0);
    REQUIRE( Abc_NtkIsBddLogic(pNtkRandBDD) == 1 );

    Abc_Ntk_t * pNtkSorted = Util_FunctionSort_block(pNtkRandBDD, pScope);
    Abc_Ntk_t * pNtkSortedStrash = Abc_NtkStrash( pNtkSorted, 0, 0, 0 );
    REQUIRE( Abc_NtkIsStrash(pNtkSortedStrash) == 1 );

    Abc_Ntk_t * pNtkResult = Util_CreatNtkFromTruthBin("00101011");

    // Util_PrintTruthtable(pNtkSortedStrash) ;
    
    REQUIRE( Util_NtkIsEqual(pNtkSortedStrash, pNtkResult) == 1 );
    Abc_NtkDelete(pNtkRand);
    Abc_NtkDelete(pNtkRandBDD);
    Abc_NtkDelete(pNtkSorted);
    Abc_NtkDelete(pNtkSortedStrash);
    Abc_NtkDelete(pNtkResult);
  }
  SECTION ( "2 block" )
  {
    const int input_num = 3;
    const int sorted_bits_num = 2;
    for (int i = input_num - sorted_bits_num; i < input_num; i++)
    {
      Vec_IntPush(pScope, i);
    }

    Abc_Ntk_t * pNtkRand = Util_CreatNtkFromTruthBin("00011011");
    Abc_Ntk_t * pNtkRandBDD = Util_NtkCollapse(pNtkRand, 0);
    REQUIRE( Abc_NtkIsBddLogic(pNtkRandBDD) == 1 );

    Abc_Ntk_t * pNtkSorted = Util_FunctionSort_block(pNtkRandBDD, pScope);
    Abc_Ntk_t * pNtkSortedStrash = Abc_NtkStrash( pNtkSorted, 0, 0, 0 );
    REQUIRE( Abc_NtkIsStrash(pNtkSortedStrash) == 1 );

    Abc_Ntk_t * pNtkResult = Util_CreatNtkFromTruthBin("10001110");

    // Util_PrintTruthtable(pNtkSortedStrash) ;
    
    REQUIRE( Util_NtkIsEqual(pNtkSortedStrash, pNtkResult) == 1 );
    Abc_NtkDelete(pNtkRand);
    Abc_NtkDelete(pNtkRandBDD);
    Abc_NtkDelete(pNtkSorted);
    Abc_NtkDelete(pNtkSortedStrash);
    Abc_NtkDelete(pNtkResult);
  }
  SECTION ( "3 block" )
  {
    const int input_num = 3;
    const int sorted_bits_num = 3;
    for (int i = input_num - sorted_bits_num; i < input_num; i++)
    {
      Vec_IntPush(pScope, i);
    }

    Abc_Ntk_t * pNtkRand = Util_CreatNtkFromTruthBin("00011011");
    Abc_Ntk_t * pNtkRandBDD = Util_NtkCollapse(pNtkRand, 0);
    REQUIRE( Abc_NtkIsBddLogic(pNtkRandBDD) == 1 );

    Abc_Ntk_t * pNtkSorted = Util_FunctionSort_block(pNtkRandBDD, pScope);
    Abc_Ntk_t * pNtkSortedStrash = Abc_NtkStrash( pNtkSorted, 0, 0, 0 );
    REQUIRE( Abc_NtkIsStrash(pNtkSortedStrash) == 1 );

    Abc_Ntk_t * pNtkResult = Util_CreatNtkFromTruthBin("11101000");

    // Util_PrintTruthtable(pNtkSortedStrash) ;
    
    REQUIRE( Util_NtkIsEqual(pNtkSortedStrash, pNtkResult) == 1 );
    Abc_NtkDelete(pNtkRand);
    Abc_NtkDelete(pNtkRandBDD);
    Abc_NtkDelete(pNtkSorted);
    Abc_NtkDelete(pNtkSortedStrash);
    Abc_NtkDelete(pNtkResult);
  }
  Vec_IntFree(pScope);
}

TEST_CASE( "Exist elimination BDD method", "[ssatelim]" )
{
  Vec_Int_t * pScope = Vec_IntAlloc(0);
  SECTION ( "3 bit / 3" )
  {
    const int input_num = 3;
    const int exist_bit_index = 2;
    Vec_IntPush(pScope, exist_bit_index);

    Abc_Ntk_t * pNtkRand = Util_CreatNtkFromTruthBin("00011011");
    Abc_Ntk_t * pNtkRandBDD = Util_NtkCollapse(pNtkRand, 0);
    REQUIRE( Abc_NtkIsBddLogic(pNtkRandBDD) == 1 );

    Abc_Ntk_t * pNtkSorted = Util_NtkExistQuantifyPis_BDD(pNtkRandBDD, pScope);
    Abc_Ntk_t * pNtkSortedStrash = Abc_NtkStrash( pNtkSorted, 0, 0, 0 );
    REQUIRE( Abc_NtkIsStrash(pNtkSortedStrash) == 1 );

    Abc_Ntk_t * pNtkResult = Util_CreatNtkFromTruthBin("00111111");

    // Util_PrintTruthtable(pNtkSortedStrash) ;
    
    REQUIRE( Util_NtkIsEqual(pNtkSortedStrash, pNtkResult) == 1 );
    Abc_NtkDelete(pNtkRand);
    Abc_NtkDelete(pNtkRandBDD);
    Abc_NtkDelete(pNtkSorted);
    Abc_NtkDelete(pNtkSortedStrash);
    Abc_NtkDelete(pNtkResult);
  }
  SECTION ( "2 bit / 3" )
  {
    const int input_num = 3;
    const int exist_bit_index = 1;
    Vec_IntPush(pScope, exist_bit_index);

    Abc_Ntk_t * pNtkRand = Util_CreatNtkFromTruthBin("00011011");
    Abc_Ntk_t * pNtkRandBDD = Util_NtkCollapse(pNtkRand, 0);
    REQUIRE( Abc_NtkIsBddLogic(pNtkRandBDD) == 1 );

    Abc_Ntk_t * pNtkSorted = Util_NtkExistQuantifyPis_BDD(pNtkRandBDD, pScope);
    Abc_Ntk_t * pNtkSortedStrash = Abc_NtkStrash( pNtkSorted, 0, 0, 0 );
    REQUIRE( Abc_NtkIsStrash(pNtkSortedStrash) == 1 );

    Abc_Ntk_t * pNtkResult = Util_CreatNtkFromTruthBin("01011111");

    // Util_PrintTruthtable(pNtkSortedStrash) ;
    
    REQUIRE( Util_NtkIsEqual(pNtkSortedStrash, pNtkResult) == 1 );
    Abc_NtkDelete(pNtkRand);
    Abc_NtkDelete(pNtkRandBDD);
    Abc_NtkDelete(pNtkSorted);
    Abc_NtkDelete(pNtkSortedStrash);
    Abc_NtkDelete(pNtkResult);
  }
  SECTION ( "1 bit / 3" )
  {
    const int input_num = 3;
    const int exist_bit_index = 0;
    Vec_IntPush(pScope, exist_bit_index);

    Abc_Ntk_t * pNtkRand = Util_CreatNtkFromTruthBin("00011011");
    Abc_Ntk_t * pNtkRandBDD = Util_NtkCollapse(pNtkRand, 0);
    REQUIRE( Abc_NtkIsBddLogic(pNtkRandBDD) == 1 );

    Abc_Ntk_t * pNtkSorted = Util_NtkExistQuantifyPis_BDD(pNtkRandBDD, pScope);
    Abc_Ntk_t * pNtkSortedStrash = Abc_NtkStrash( pNtkSorted, 0, 0, 0 );
    REQUIRE( Abc_NtkIsStrash(pNtkSortedStrash) == 1 );

    Abc_Ntk_t * pNtkResult = Util_CreatNtkFromTruthBin("10111011");

    // Util_PrintTruthtable(pNtkSortedStrash) ;
    
    REQUIRE( Util_NtkIsEqual(pNtkSortedStrash, pNtkResult) == 1 );
    Abc_NtkDelete(pNtkRand);
    Abc_NtkDelete(pNtkRandBDD);
    Abc_NtkDelete(pNtkSorted);
    Abc_NtkDelete(pNtkSortedStrash);
    Abc_NtkDelete(pNtkResult);
  }
  Vec_IntFree(pScope);
}

TEST_CASE( "Exist Random elimination BDD method", "[ssatelim]" )
{
  Vec_Int_t * pScope = Vec_IntAlloc(0);
  SECTION ( "exist-random with ratio 0" )
  {
    // E 1, R 2. ()
    // Abc_Print( 1, "exist-random with ratio 0\n" );
    Abc_Ntk_t * pNtk = Util_CreatNtkFromTruthBin("0001");
    Abc_Ntk_t * pNtkBDD = Util_NtkCollapse(pNtk, 0);
    REQUIRE( Abc_NtkIsBddLogic(pNtkBDD) == 1 );

    Vec_IntPush(pScope, 1);

    Abc_Ntk_t * pNtkRandomElim = Util_NtkRandomQuantifyPis_BDD(pNtkBDD, pScope);
    REQUIRE( Abc_NtkIsBddLogic(pNtkRandomElim) == 1 );

    // Abc_NtkDup is important to prevent pNtkRandomElim be starsh too
    Abc_Ntk_t * pNtkRandomElimStrash = Abc_NtkStrash( Abc_NtkDup(pNtkRandomElim), 0, 0, 0 );
    REQUIRE( Abc_NtkIsStrash(pNtkRandomElimStrash) == 1 );
    // Util_PrintTruthtable(pNtkRandomElimStrash) ;
    Abc_Ntk_t * pNtkRandomElimExpect = Util_CreatNtkFromTruthBin("0010");
    REQUIRE( Util_NtkIsEqual(pNtkRandomElimStrash, pNtkRandomElimExpect) == 1 );

    Vec_IntClear(pScope);
    Vec_IntPush(pScope, 0);

    REQUIRE( Abc_NtkIsBddLogic(pNtkRandomElim) == 1 );
    Abc_Ntk_t * pNtkExistElim = Util_NtkExistQuantifyPis_BDD(pNtkRandomElim, pScope);
    REQUIRE( Abc_NtkIsBddLogic(pNtkExistElim) == 1 );
    Abc_Ntk_t * pNtkExistElimStrash = Abc_NtkStrash( pNtkExistElim, 0, 0, 0 );
    REQUIRE( Abc_NtkIsStrash(pNtkExistElimStrash) == 1 );
    // Util_PrintTruthtable(pNtkExistElimStrash) ;
    Abc_Ntk_t * pNtkExistElimStrashExpect = Util_CreatNtkFromTruthBin("1010");
    REQUIRE( Util_NtkIsEqual(pNtkExistElimStrash, pNtkExistElimStrashExpect) == 1 );


    Abc_NtkDelete(pNtk);
    Abc_NtkDelete(pNtkBDD);
    Abc_NtkDelete(pNtkRandomElim);
    Abc_NtkDelete(pNtkRandomElimStrash);
    Abc_NtkDelete(pNtkRandomElimExpect);
    Abc_NtkDelete(pNtkExistElim);
    Abc_NtkDelete(pNtkExistElimStrash);
    Abc_NtkDelete(pNtkExistElimStrashExpect);
  }
  SECTION ( "exist-random with ratio 0, method 2" )
  {
    // E 1, R 2. ()
    Abc_Ntk_t * pNtk = Util_CreatNtkFromTruthBin("0001");
    Abc_Ntk_t * pNtkBDD = Util_NtkCollapse(pNtk, 0);


    Vec_Int_t * pScopeE = Vec_IntAlloc(0);
    Vec_Int_t * pScopeR = Vec_IntAlloc(0);
    Vec_IntPush(pScopeE, 0);
    Vec_IntPush(pScopeR, 1);

    Abc_Ntk_t * pNtkExistElim = Util_NtkExistRandomQuantifyPis_BDD(pNtkBDD, pScopeE, pScopeR);
    REQUIRE( Abc_NtkIsBddLogic(pNtkExistElim) == 1 );
    Abc_Ntk_t * pNtkExistElimStrash = Abc_NtkStrash( pNtkExistElim, 0, 0, 0 );
    REQUIRE( Abc_NtkIsStrash(pNtkExistElimStrash) == 1 );
    // Util_PrintTruthtable(pNtkExistElimStrash) ;
    Abc_Ntk_t * pNtkExistElimStrashExpect = Util_CreatNtkFromTruthBin("1010");
    REQUIRE( Util_NtkIsEqual(pNtkExistElimStrash, pNtkExistElimStrashExpect) == 1 );

    Abc_NtkDelete(pNtk);
    Abc_NtkDelete(pNtkBDD);
    Abc_NtkDelete(pNtkExistElim);
    Abc_NtkDelete(pNtkExistElimStrash);
    Abc_NtkDelete(pNtkExistElimStrashExpect);
    Vec_IntFree(pScopeE);
    Vec_IntFree(pScopeR);
  }
  Vec_IntFree(pScope);
}

TEST_CASE( "function sort correction", "[ssatelim]" )
{
  Vec_Int_t * pScope = Vec_IntAlloc(0);
  SECTION ( "main" )
  {
    const int input_num = 4;
    for (int i = 0; i < input_num; i++)
    {
      Vec_IntPush(pScope, i);
    }

    Abc_Ntk_t * pNtkRand = Util_CreatNtkFromTruthBin("1010101010010010");
    pNtkRand = Util_FunctionSort(pNtkRand, pScope);
    Abc_Ntk_t * pNtkResult = Util_CreatNtkFromTruthBin("1111111000000000");
    
    REQUIRE( Util_NtkIsEqual(pNtkRand, pNtkResult) == 1 );
    Abc_NtkDelete(pNtkRand);
    Abc_NtkDelete(pNtkResult);
  }
  SECTION ( "1 input" )
  {
    const int input_num = 1;
    for (int i = 0; i < input_num; i++)
    {
      Vec_IntPush(pScope, i);
    }
    Abc_Ntk_t * pNtkRand = Util_CreatNtkFromTruthBin("01");
    pNtkRand = Util_FunctionSort(pNtkRand, pScope);
    Abc_Ntk_t * pNtkResult = Util_CreatNtkFromTruthBin("10");
    
    REQUIRE( Util_NtkIsEqual(pNtkRand, pNtkResult) == 1 );
    Abc_NtkDelete(pNtkRand);
    Abc_NtkDelete(pNtkResult);
  }
  SECTION ( "BDD method 1 input" )
  {
    const int input_num = 1;
    for (int i = 0; i < input_num; i++)
    {
      Vec_IntPush(pScope, i);
    }

    Abc_Ntk_t * pNtkRand = Util_CreatNtkFromTruthBin("01");
    Abc_Ntk_t * pNtkRandBDD = Util_NtkCollapse(pNtkRand, 0);
    REQUIRE( Abc_NtkIsBddLogic(pNtkRandBDD) == 1 );

    pNtkRandBDD = Util_NtkRandomQuantifyPis_BDD(pNtkRandBDD, pScope);
    REQUIRE( Abc_NtkIsBddLogic(pNtkRandBDD) == 1 );

    pNtkRandBDD = Abc_NtkStrash( pNtkRandBDD, 0, 0, 0 );
    REQUIRE( Abc_NtkIsStrash(pNtkRandBDD) == 1 );
    Abc_Ntk_t * pNtkResult = Util_CreatNtkFromTruthBin("10");
    
    REQUIRE( Util_NtkIsEqual(pNtkRandBDD, pNtkResult) == 1 );
    Abc_NtkDelete(pNtkRand);
    Abc_NtkDelete(pNtkResult);
  }
  SECTION ( "BDD method 2 input" )
  {
    const int input_num = 2;
    for (int i = 0; i < input_num; i++)
    {
      Vec_IntPush(pScope, i);
    }

    Abc_Ntk_t * pNtkRand = Util_CreatNtkFromTruthBin("0101");
    Abc_Ntk_t * pNtkRandBDD = Util_NtkCollapse(pNtkRand, 0);
    REQUIRE( Abc_NtkIsBddLogic(pNtkRandBDD) == 1 );

    pNtkRandBDD = Util_NtkRandomQuantifyPis_BDD(pNtkRandBDD, pScope);
    REQUIRE( Abc_NtkIsBddLogic(pNtkRandBDD) == 1 );

    pNtkRandBDD = Abc_NtkStrash( pNtkRandBDD, 0, 0, 0 );
    REQUIRE( Abc_NtkIsStrash(pNtkRandBDD) == 1 );
    Abc_Ntk_t * pNtkResult = Util_CreatNtkFromTruthBin("1100");

    // Util_PrintTruthtable(pNtkRandBDD) ;
    
    REQUIRE( Util_NtkIsEqual(pNtkRandBDD, pNtkResult) == 1 );
    Abc_NtkDelete(pNtkRand);
    Abc_NtkDelete(pNtkResult);
  }
  SECTION ( "BDD method 3 input" )
  {
    const int input_num = 3;
    for (int i = 0; i < input_num; i++)
    {
      Vec_IntPush(pScope, i);
    }

    Abc_Ntk_t * pNtkRand = Util_CreatNtkFromTruthBin("01011101");
    Abc_Ntk_t * pNtkRandBDD = Util_NtkCollapse(pNtkRand, 0);
    REQUIRE( Abc_NtkIsBddLogic(pNtkRandBDD) == 1 );

    pNtkRandBDD = Util_NtkRandomQuantifyPis_BDD(pNtkRandBDD, pScope);
    REQUIRE( Abc_NtkIsBddLogic(pNtkRandBDD) == 1 );

    pNtkRandBDD = Abc_NtkStrash( pNtkRandBDD, 0, 0, 0 );
    REQUIRE( Abc_NtkIsStrash(pNtkRandBDD) == 1 );
    Abc_Ntk_t * pNtkResult = Util_CreatNtkFromTruthBin("11111000");

    // Util_PrintTruthtable(pNtkRandBDD) ;
    
    REQUIRE( Util_NtkIsEqual(pNtkRandBDD, pNtkResult) == 1 );
    Abc_NtkDelete(pNtkRand);
    Abc_NtkDelete(pNtkResult);
  }
  Vec_IntFree(pScope);
}

TEST_CASE( "function sort performace", "[.][ssatelim]" )
{
  int nTest = 1000;
  Vec_Int_t * pScope = Vec_IntAlloc(0);
  for (int i = 0; i < 4; i++)
  {
    Vec_IntPush(pScope, i);
  }

  SECTION ( "Util_function_sort" )
  {
    abctime clk = Abc_Clock();
    for (int i = 0; i <= nTest; i++)
    {
      Abc_Ntk_t * pNtkRand = Util_CreatNtkFromTruthBin("1010101010010010");
      pNtkRand = Util_FunctionSort_Select(pNtkRand, pScope, 1);
      Abc_NtkDelete(pNtkRand);
    }
    Abc_PrintTime( 1, "Function sort method 1 runtime", Abc_Clock() - clk );
  }

  SECTION ( "Util_function_sort2" )
  {
    abctime clk = Abc_Clock();
    for (int i = 0; i <= nTest; i++)
    {
      Abc_Ntk_t * pNtkRand = Util_CreatNtkFromTruthBin("1010101010010010");
      pNtkRand = Util_FunctionSort_Select(pNtkRand, pScope, 2);
      Abc_NtkDelete(pNtkRand);
    }
    Abc_PrintTime( 1, "Function sort method 2 runtime", Abc_Clock() - clk );
  }

  Vec_IntFree(pScope);
}

Abc_Ntk_t * Abc_NtkMiterQuantifyPis_test( Abc_Ntk_t * pNtk )
{
    Abc_Ntk_t * pNtkTemp;
    Abc_Obj_t * pObj;
    int i;
    assert( Abc_NtkHasOnlyLatchBoxes(pNtk) );

    Abc_NtkForEachPi( pNtk, pObj, i )
    {
        pNtk = Util_NtkExistQuantify( pNtkTemp = pNtk, i );
        if ( pNtkTemp != pNtk) Abc_NtkDelete( pNtkTemp );
    }

    return pNtk;
}

TEST_CASE( "quantify eliminate performace", "[.][ssatelim]" )
{
  int nTest = 1000;
  abctime clk = Abc_Clock();
  for (int i = 0; i <= nTest; i++)
  {
    Abc_Ntk_t * pNtkRand = Util_CreatNtkFromTruthBin("1010101010010010");
    pNtkRand = Abc_NtkMiterQuantifyPis(pNtkRand);
    Abc_NtkDelete(pNtkRand);
  }
  Abc_PrintTime( 1, "Abc_NtkMiterQuantifyPis() runtime", Abc_Clock() - clk );

  clk = Abc_Clock();
  for (int i = 0; i <= nTest; i++)
  {
    Abc_Ntk_t * pNtkRand = Util_CreatNtkFromTruthBin("1010101010010010");
    pNtkRand = Abc_NtkMiterQuantifyPis_test(pNtkRand);
    Abc_NtkDelete(pNtkRand);
  }
  Abc_PrintTime( 1, "Abc_NtkMiterQuantifyPis_test() runtime", Abc_Clock() - clk );
}

ABC_NAMESPACE_IMPL_END
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
