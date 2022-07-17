#include "base/main/main.h"
#include "base/main/mainInt.h"
#include "synthesis.h"

ABC_NAMESPACE_IMPL_START

static int testCadet_Command(Abc_Frame_t* pAbc, int argc, char** argv) {
  int c = 0;
  int fVerbose = 0;
  char* pFileName;
  // Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);
  // Abc_Ntk_t * pNtkResult;
  Extra_UtilGetoptReset();
  while ((c = Extra_UtilGetopt(argc, argv, "vSh")) != EOF) {
    switch (c) {
      case 'v':
        fVerbose ^= 1;
        break;
      case 'h':
        goto usage;
      default:
        goto usage;
    }
  }

  if (argc != globalUtilOptind + 1) goto usage;

  // get the input file name
  pFileName = argv[globalUtilOptind];

  // call main function
  cadetTest(pFileName, fVerbose);

  return 0;

usage:
  Abc_Print(-2, "usage: cadet_test [-vh] <file>\n");
  return 1;
}

static int testManthan_Command(Abc_Frame_t* pAbc, int argc, char** argv) {
  int c = 0;
  int fVerbose = 0;
  char* pFileName;
  // Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);
  // Abc_Ntk_t * pNtkResult;
  Extra_UtilGetoptReset();
  while ((c = Extra_UtilGetopt(argc, argv, "vSh")) != EOF) {
    switch (c) {
      case 'v':
        fVerbose ^= 1;
        break;
      case 'h':
        goto usage;
      default:
        goto usage;
    }
  }

  if (argc != globalUtilOptind + 1) goto usage;

  // get the input file name
  pFileName = argv[globalUtilOptind];

  // call main function
  manthanTest(pFileName, fVerbose);

  return 0;

usage:
  Abc_Print(-2, "usage: manthan_test [-vh] <file>\n");
  return 1;
}

static int testr2f_Command(Abc_Frame_t* pAbc, int argc, char** argv) {
  int c = 0;
  int fVerbose = 0;
  char* pFileName;
  // Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);
  // Abc_Ntk_t * pNtkResult;
  Extra_UtilGetoptReset();
  while ((c = Extra_UtilGetopt(argc, argv, "vSh")) != EOF) {
    switch (c) {
      case 'v':
        fVerbose ^= 1;
        break;
      case 'h':
        goto usage;
      default:
        goto usage;
    }
  }

  if (argc != globalUtilOptind + 1) goto usage;

  // get the input file name
  pFileName = argv[globalUtilOptind];

  // call main function
  r2fTest(pFileName, fVerbose);

  return 0;

usage:
  Abc_Print(-2, "usage: r2f_test [-vh] <file>\n");
  return 1;
}


// called during ABC startup
static void init(Abc_Frame_t* pAbc) {
  Cmd_CommandAdd(pAbc, "Alcom", "cadet_test", testCadet_Command, 1);
  Cmd_CommandAdd(pAbc, "Alcom", "manthan_test", testManthan_Command, 1);
  Cmd_CommandAdd(pAbc, "Alcom", "r2f_test", testr2f_Command, 1);
}

// called during ABC termination
static void destroy(Abc_Frame_t* pAbc) {}

// this object should not be modified after the call to Abc_FrameAddInitializer
static Abc_FrameInitializer_t ssat_frame_initializer = {init, destroy};

// register the initializer a constructor of a global object
// called before main (and ABC startup)
struct cadet_registrar {
  cadet_registrar() { Abc_FrameAddInitializer(&ssat_frame_initializer); }
} cadet_registrar_;

ABC_NAMESPACE_IMPL_END
