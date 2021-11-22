/**CFile****************************************************************
  FileName    [utilCommand.cc]
  SystemName  []
  PackageName []
  Synopsis    [Add util commands]
  Author      [Kuan-Hua Tu]
  
  Affiliation [NTU]
  Date        [Sun Dec 23 18:47:51 CST 2018]
***********************************************************************/

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "base/main/main.h"
#include "base/main/mainInt.h"
#include "extUtil/util.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

static Abc_Ntk_t * DummyFunction( Abc_Ntk_t * pNtk )
{
    Abc_Print( -1, "Please rewrite DummyFunction() in file \"utilCommand.cc\".\n" );
    return NULL;
}

static int util_Command_example( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    int c            = 0;
    int fVerbose     = 0;
    int fIntOption   = 0;
    int fTrigger     = 0;
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);
    Abc_Ntk_t * pNtkResult;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "Ifvh" ) ) != EOF )
    {
        switch ( c )
        {
            case 'I':
                if ( globalUtilOptind >= argc )
                {
                    Abc_Print( -1, "Command line switch \"-I\" should be followed by an integer.\n" );
                    goto usage;
                }
                fIntOption = atoi(argv[globalUtilOptind]);
                globalUtilOptind++;
                break;
            case 'f':
                fTrigger ^= 1;
                break;
            case 'v':
                fVerbose ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }

    // check precondition
    if ( pNtk == NULL )
    {
        Abc_Print( -1, "Empty network.\n" );
        return 1;
    }
    
    if ( !Abc_NtkIsStrash(pNtk) )
    {
        Abc_Print( -1, "This command can only handle strash network. Use 'strash' command to build strash network.\n" );
        return 1;
    }
    
    // call main function
    pNtkResult = DummyFunction( pNtk );
    
    // replace the current network if needed
    if ( pNtkResult )
    {
        Abc_FrameReplaceCurrentNetwork( pAbc, pNtkResult );
    }
    return 0;
    
usage:
    Abc_Print( -2, "usage: util [-I <num>] [-fvh]\n" );
    Abc_Print( -2, "\t           command description \n" );
    Abc_Print( -2, "\t-I <num> : option with integer [default = %d]\n", fIntOption );
    Abc_Print( -2, "\t-f       : trigger if allow blahblahblah [default = %d]\n", fTrigger );
    Abc_Print( -2, "\t-v       : verbosity [default = %d]\n", fVerbose );
    Abc_Print( -2, "\t-h       : print the command usage\n" );
    return 1;
}

static int Abc_CommandPrintNetworkStats( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);
    int c            = 0;
    int fVerbose     = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "vh" ) ) != EOF )
    {
        switch ( c )
        {
            case 'v':
                fVerbose ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }

    if ( pNtk == NULL )
    {
        Abc_Print( -1, "Empty network.\n" );
        return 1;
    }

	switch( pNtk->ntkType )
    {
		case ABC_NTK_NONE:
			Abc_Print( 1, "Network type: NONE.    ");
			break;
		case ABC_NTK_NETLIST:
			Abc_Print( 1, "Network type: NETLIST. ");
			break;
		case ABC_NTK_LOGIC:
			Abc_Print( 1, "Network type: LOGIC.   ");
			break;
		case ABC_NTK_STRASH:
			Abc_Print( 1, "Network type: STRASH.  ");
			break;
		case ABC_NTK_OTHER:
			Abc_Print( 1, "Network type: OTHER.   ");
			break;
		default:
			Abc_Print( 1, "Network type: unknown. ");
			break;
	}

	switch( pNtk->ntkFunc )
    {
		case ABC_FUNC_NONE:
			Abc_Print( 1, "Network function: NONE.     \n");
			break;
		case ABC_FUNC_SOP:
			Abc_Print( 1, "Network function: SOP.      \n");
			break;
		case ABC_FUNC_BDD:
			Abc_Print( 1, "Network function: BDD.      \n");
			break;
		case ABC_FUNC_AIG:
			Abc_Print( 1, "Network function: AIG.      \n");
			break;
		case ABC_FUNC_MAP:
			Abc_Print( 1, "Network function: MAP.      \n");
			break;
		case ABC_FUNC_BLIFMV:
			Abc_Print( 1, "Network function: BLIFMV.   \n");
			break;
		case ABC_FUNC_BLACKBOX:
			Abc_Print( 1, "Network function: BLACKBOX. \n");
			break;
		case ABC_FUNC_OTHER:
			Abc_Print( 1, "Network function: OTHER.    \n");
			break;
		default:
			Abc_Print( 1, "Network function: unknown.  \n");
			break;
	}
	return 0;

usage:
    Abc_Print( -2, "usage: print_network [-vh]\n" );
    Abc_Print( -2, "\t           print network type and network function. \n" );
    Abc_Print( -2, "\t-v       : verbosity [default = %d]\n", fVerbose );
    Abc_Print( -2, "\t-h       : print the command usage\n" );
    return 1;
}

static int Abc_CommandPrintNodeStats( Abc_Frame_t * pAbc, int argc, char ** argv ) 
{
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);
    Abc_Obj_t * pObj;
    int i, j;
    int Entry;
    int fPrintName = 0;
    int fVerbose   = 0;
    int c;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "nvh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'n':
            fPrintName ^= 1;
			break;
        case 'v':
            fVerbose ^= 1;
            break;
        case 'h':
            goto usage;
        default:
            goto usage;
        }
    }

    if ( pNtk == NULL )
    {
        Abc_Print( -1, "Empty network.\n" );
        return 1;
    }
	
    Abc_NtkForEachObj( pNtk, pObj, i )
    {
        if( fPrintName ) 
        {
            Abc_Print( 1, "%s: ", Abc_ObjName(pObj) );
        }
        Abc_Print( 1, "ID = %d, type = ", Abc_ObjId(pObj) );
        switch( Abc_ObjType(pObj) )
        {
            case ABC_OBJ_NONE:
                Abc_Print( 1, "unknown");
                break;
            case ABC_OBJ_CONST1:
                Abc_Print( 1, "constant 1 node");
                break;
            case ABC_OBJ_PI:
                Abc_Print( 1, "primary input terminal");
                break;
            case ABC_OBJ_PO:
                Abc_Print( 1, "primary output terminal");
                break;
            case ABC_OBJ_BI:
                Abc_Print( 1, "box input terminal");
                break;
            case ABC_OBJ_BO:
                Abc_Print( 1, "box output terminal");
                break;
            case ABC_OBJ_NET:
                Abc_Print( 1, "net");
                break;
            case ABC_OBJ_NODE:
                Abc_Print( 1, "node");
                break;
            case ABC_OBJ_LATCH:
                Abc_Print( 1, "latch");
                break;
            case ABC_OBJ_WHITEBOX:
                Abc_Print( 1, "box with known contents");
                break;
            case ABC_OBJ_BLACKBOX:
                Abc_Print( 1, "box with unknown contents");
                break;
            case ABC_OBJ_NUMBER:
                Abc_Print( 1, "unused");
                break;
            default:
                Abc_Print( 1, "unknown");
        }
        
        Abc_Print( 1, ", fanin IDs =");
        Vec_IntForEachEntry( Abc_ObjFaninVec(pObj), Entry, j )
            Abc_Print( 1, " %d", Entry);
        Abc_Print( 1, "\n");
    }
	
    return 0;

usage:
    Abc_Print( -2, "usage: print_obj [-nvh]\n" );
    Abc_Print( -2, "\t           print obj info. \n" );
    Abc_Print( -2, "\t-n       : trigger if print obj name [default = %d]\n", fPrintName );
    Abc_Print( -2, "\t-v       : verbosity [default = %d]\n", fVerbose );
    Abc_Print( -2, "\t-h       : print the command usage\n" );
    return 1;
}

static int Abc_CommandPrintTruthTable( Abc_Frame_t * pAbc, int argc, char ** argv ) 
{
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);
    Util_PrintTruthtable(pNtk);
}

// called during ABC startup
static void init(Abc_Frame_t* pAbc)
{
    // Cmd_CommandAdd( pAbc, "Alcom", "util", util_Command, 1);
	Cmd_CommandAdd( pAbc, "Alcom", "print_network",  Abc_CommandPrintNetworkStats,   0 );
    Cmd_CommandAdd( pAbc, "Alcom", "print_obj",      Abc_CommandPrintNodeStats,      0 );
    Cmd_CommandAdd( pAbc, "Alcom", "print_tt",  Abc_CommandPrintTruthTable,  0 );

}

// called during ABC termination
static void destroy(Abc_Frame_t* pAbc)
{
}

// this object should not be modified after the call to Abc_FrameAddInitializer
static Abc_FrameInitializer_t util_frame_initializer = { init, destroy };

// register the initializer a constructor of a global object
// called before main (and ABC startup)
struct util_registrar
{
    util_registrar() 
    {
        Abc_FrameAddInitializer(&util_frame_initializer);
    }
} util_registrar_;

ABC_NAMESPACE_IMPL_END
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////