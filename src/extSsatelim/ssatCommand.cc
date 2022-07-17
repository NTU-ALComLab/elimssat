/**CFile****************************************************************
  FileName    [ssatCommand.cc]
  SystemName  []
  PackageName []
  Synopsis    [Add command ssat]
  Author      [Kuan-Hua Tu]

  Affiliation [NTU]
  Date        [Sun Jun 23 02:41:03 CST 2019]
***********************************************************************/

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "base/main/main.h"
#include "base/main/mainInt.h"
#include "ssatelim.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

static Abc_Ntk_t *DummyFunction(Abc_Ntk_t *pNtk) {
  Abc_Print(-1, "Please rewrite DummyFunction() in file \"ssatCommand.cc\".\n");
  return NULL;
}

static int ssat_Command(Abc_Frame_t *pAbc, int argc, char **argv) {
  int c = 0;
  int fVerbose = 0;
  int fReorder = 1;
  int fProjected = 1;
  int fPreprocess = 0;
  int fGatedetect = 1;

  char *pFileName;
  // Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);
  // Abc_Ntk_t * pNtkResult;
  Extra_UtilGetoptReset();
  while ((c = Extra_UtilGetopt(argc, argv, "PprvgSh")) != EOF) {
    switch (c) {
    case 'p':
      fProjected ^= 1;
      break;
    case 'P':
      fPreprocess ^= 1;
      break;
    case 'g':
      fGatedetect ^= 1;
      break;
    case 'r':
      fReorder ^= 1;
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

  if (argc != globalUtilOptind + 1)
    goto usage;

  // get the input file name
  pFileName = argv[globalUtilOptind];

  // call main function
  ssat_main(pFileName, fReorder, fProjected, fPreprocess, fGatedetect,
            fVerbose);

  return 0;

usage:
  Abc_Print(-2, "usage: ssat [-vh] <file>\n");
  Abc_Print(-2, "\t           perform ssat\n");
  Abc_Print(-2, "\t-v       : verbosity [default = %d]\n", fVerbose);
  Abc_Print(-2, "\t-h       : print the command usage\n");
  Abc_Print(-2, "\tfile     : the SDIMACS file\n");
  return 1;
}

// called during ABC startup
static void init(Abc_Frame_t *pAbc) {
  Cmd_CommandAdd(pAbc, "Alcom", "ssat", ssat_Command, 1);
}

// called during ABC termination
static void destroy(Abc_Frame_t *pAbc) {}

// this object should not be modified after the call to Abc_FrameAddInitializer
static Abc_FrameInitializer_t ssat_frame_initializer = {init, destroy};

// register the initializer a constructor of a global object
// called before main (and ABC startup)
struct ssat_registrar {
  ssat_registrar() { Abc_FrameAddInitializer(&ssat_frame_initializer); }
} ssat_registrar_;

ABC_NAMESPACE_IMPL_END
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
