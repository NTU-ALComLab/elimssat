/**CFile****************************************************************
  FileName    [utilCommand_test.cc]
  SystemName  []
  PackageName []
  Synopsis    [Test util commands]
  Author      [Kuan-Hua Tu]
  
  Affiliation [NTU]
  Date        [Sun Dec 23 18:47:51 CST 2018]
***********************************************************************/

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "base/main/main.h"
#include "extUnitTest/catch.hpp"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

TEST_CASE( "Util commands existd", "[util]" )
{
  Abc_Frame_t * pAbc = Abc_FrameGetGlobalFrame();
	// REQUIRE( Cmd_CommandIsDefined(pAbc, "util") == 1 );
	REQUIRE( Cmd_CommandIsDefined(pAbc, "print_network") == 1 );
	REQUIRE( Cmd_CommandIsDefined(pAbc, "print_obj") == 1 );
}

ABC_NAMESPACE_IMPL_END
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////