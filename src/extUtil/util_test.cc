/**CFile****************************************************************
  FileName    [util_test.cc]
  SystemName  [ABC: Logic synthesis and verification system.]
  PackageName []
  Synopsis    []
  Author      [Kuan-Hua Tu(isuis3322@gmail.com)]
  
  Affiliation [NTU]
  Date        [2018.04.25]
***********************************************************************/

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "util.h"
#include "extUnitTest/catch.hpp"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

TEST_CASE( "Create Ntk from TruthTable", "[util]" )
{
    SECTION("Create NULL Ntk if TruthBin is NULL")
    {
        REQUIRE( Util_CreatNtkFromTruthBin( NULL ) == NULL );
    }
    SECTION("Create NULL Ntk if TruthBin is empty string")
    {
        REQUIRE( Util_CreatNtkFromTruthBin( "" ) == NULL );
    }
}
    
TEST_CASE( "Ntk equivelence", "[util]" )
{
    SECTION("Create NULL Ntk if TruthBin is empty string")
    {
        Abc_Ntk_t * pNtk1 = Util_CreatNtkFromTruthBin("10");
        Abc_Ntk_t * pNtk2 = Util_CreatNtkFromTruthBin("10");
        REQUIRE( Util_NtkIsEqual( pNtk1, pNtk2 ) == 1 );
        Abc_NtkDelete(pNtk1);
        Abc_NtkDelete(pNtk2);
    }
}

Abc_Ntk_t * Abc_NtkCreateCone(Abc_Ntk_t * pNtk, int Output)
{
    Abc_Obj_t * pCoOnSet = Abc_NtkCo( pNtk, Output );
    Abc_Ntk_t * pNtkOnSet = Abc_NtkCreateCone( pNtk, Abc_ObjFanin0(pCoOnSet), Abc_ObjName(pCoOnSet), 1 );
    if ( Abc_ObjFaninC0(pCoOnSet) )
    {
        Abc_NtkPo(pNtkOnSet, 0)->fCompl0  ^= 1;
    }
    return pNtkOnSet;
}
    
TEST_CASE( "Counter Ntk", "[util]" )
{
    SECTION("Counter Ntk with 1 input")
    {
        Abc_Ntk_t * pNtkCounter = Util_NtkCreateCounter(1);

        REQUIRE( Abc_NtkPiNum(pNtkCounter) == 1 );
        REQUIRE( Abc_NtkPoNum(pNtkCounter) == 1 );

        Abc_Ntk_t * pNtkExpect = Util_CreatNtkFromTruthBin("10");

        REQUIRE( Util_NtkIsEqual(pNtkCounter, pNtkExpect) == 1 );

        Abc_NtkDelete(pNtkCounter);
        Abc_NtkDelete(pNtkExpect);
    }

    SECTION("Counter Ntk with 2 input")
    {
        Abc_Ntk_t * pNtkCounter = Util_NtkCreateCounter(2);
        REQUIRE( Abc_NtkPiNum(pNtkCounter) == 2 );
        REQUIRE( Abc_NtkPoNum(pNtkCounter) == 2 );

        Abc_Ntk_t * pNtkSum = Abc_NtkCreateCone(pNtkCounter, 0);
        Abc_Ntk_t * pNtkSumExpect = Util_CreatNtkFromTruthBin("0110");

        REQUIRE( Util_NtkIsEqual(pNtkSum, pNtkSumExpect) == 1 );

        Abc_Ntk_t * pNtkCarry = Abc_NtkCreateCone(pNtkCounter, 1);
        Abc_Ntk_t * pNtkCarryExpect = Util_CreatNtkFromTruthBin("1000");

        REQUIRE( Util_NtkIsEqual(pNtkCarry, pNtkCarryExpect) == 1 );

        Abc_NtkDelete(pNtkCounter);
        Abc_NtkDelete(pNtkSum);
        Abc_NtkDelete(pNtkSumExpect);
        Abc_NtkDelete(pNtkCarry);
        Abc_NtkDelete(pNtkCarryExpect);
    }

    SECTION("Counter Ntk with 3 input")
    {
        Abc_Ntk_t * pNtkCounter = Util_NtkCreateCounter(3);
        REQUIRE( Abc_NtkPiNum(pNtkCounter) == 3 );
        REQUIRE( Abc_NtkPoNum(pNtkCounter) == 2 );

        Abc_Ntk_t * pNtkSum = Abc_NtkCreateCone(pNtkCounter, 0);
        Abc_Ntk_t * pNtkSumExpect = Util_CreatNtkFromTruthBin("10010110");

        REQUIRE( Util_NtkIsEqual(pNtkSum, pNtkSumExpect) == 1 );

        Abc_Ntk_t * pNtkCarry = Abc_NtkCreateCone(pNtkCounter, 1);
        Abc_Ntk_t * pNtkCarryExpect = Util_CreatNtkFromTruthBin("11101000");

        REQUIRE( Util_NtkIsEqual(pNtkCarry, pNtkCarryExpect) == 1 );

        Abc_NtkDelete(pNtkCounter);
        Abc_NtkDelete(pNtkSum);
        Abc_NtkDelete(pNtkSumExpect);
        Abc_NtkDelete(pNtkCarry);
        Abc_NtkDelete(pNtkCarryExpect);
    }
}
    
TEST_CASE( "Bit set", "[util]" )
{
    SECTION("set one bit")
    {
        unsigned int a = 0;
        unsigned int * p = &a;
        Util_BitSet_setValue( p, 0, 0 );
        REQUIRE( a == 0 );
        Util_BitSet_setValue( p, 0, 1 );
        REQUIRE( a == 0b10000000000000000000000000000000 );
        Util_BitSet_setValue( p, 31, 1 );
        REQUIRE( a == 0b10000000000000000000000000000001 );
        Util_BitSet_setValue( p, 0, 0 );
        REQUIRE( a == 0b00000000000000000000000000000001 );
        Util_BitSet_setValue( p, 31, 0 );
        REQUIRE( a == 0b00000000000000000000000000000000 );
    }

    SECTION("get one bit")
    {
        unsigned int a = 0b10000000000000000000000000000001;
        unsigned int * p = &a;
        Bool b = Util_BitSet_getValue( p, 0 );
        REQUIRE( b == 1 );
        b = Util_BitSet_getValue( p, 1 );
        REQUIRE( b == 0 );
        b = Util_BitSet_getValue( p, 31 );
        REQUIRE( b == 1 );
    }
}  

TEST_CASE( "Obj Type", "[util]" )
{
    // enum Abc_ObjType_t in abc.h
    // use for Util_ObjPrint() mapping
    REQUIRE( ABC_OBJ_NONE       == 0 );
    REQUIRE( ABC_OBJ_CONST1     == 1 );
    REQUIRE( ABC_OBJ_PI         == 2 );
    REQUIRE( ABC_OBJ_PO         == 3 );
    REQUIRE( ABC_OBJ_BI         == 4 );
    REQUIRE( ABC_OBJ_BO         == 5 );
    REQUIRE( ABC_OBJ_NET        == 6 );
    REQUIRE( ABC_OBJ_NODE       == 7 );
    REQUIRE( ABC_OBJ_LATCH      == 8 );
    REQUIRE( ABC_OBJ_WHITEBOX   == 9 );
    REQUIRE( ABC_OBJ_BLACKBOX   ==10 );
    REQUIRE( ABC_OBJ_NUMBER     ==11 );
}

ABC_NAMESPACE_IMPL_END
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
