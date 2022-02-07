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
#include "bddRel2Func.hpp"
#include "eqRel2Func.hpp"
#include "intRel2Func.hpp"
#include "intRel2Func2.hpp"
#include "misc.hpp"
#include "subRel2Func.hpp"
#include <iostream>
#include <vector>
using std::cout;
using std::endl;

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////
static int r2f_Command(Abc_Frame_t *pAbc, int argc, char **argv) {
  int c = 0;
  int fVerbose = 0;
  int fChoice = 0;
  clock_t clk, clk2;
  int tFunc = 0;
  int tSuc = 0;
  int tZeroCount = 0;
  long int tNode = 0;
  long int tLev = 0;
  double avgNode = 0.0f;
  double avgLev = 0.0f;
  double avgMem = 0.0f;
  double avgTime = 0.0f;

  char *pFileName;
  Abc_Ntk_t *pNtk;
  // Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);
  // Abc_Ntk_t * pNtkResult;
  Extra_UtilGetoptReset();
  while ((c = Extra_UtilGetopt(argc, argv, "cSh")) != EOF) {
    switch (c) {
    case 'c':
      if (globalUtilOptind >= argc) {
        Abc_Print(
            -1,
            "Command line switch \"-I\" should be followed by an integer.\n");
        goto usage;
      }
      fChoice = atoi(argv[globalUtilOptind]);
      globalUtilOptind++;
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
  char Command[1000];
  sprintf(Command, "read %s", pFileName);
  if (Cmd_CommandExecute(pAbc, Command)) {
    fprintf(stdout, "Cannot execute command \"%s\".\n", Command);
    return 1;
  }
  pNtk = Abc_FrameReadNtk(pAbc);
  pNtk = Abc_NtkStrash(pNtk, 0, 0, 0);
  Abc_Obj_t *pObj;
  int j;

  Abc_NtkForEachPo(pNtk, pObj, j) {

    if (!Abc_ObjIsNode(Abc_ObjFanin0(pObj)))
      continue;
    Abc_Ntk_t *pNtkTemp =
        Abc_NtkCreateCone(pNtk, Abc_ObjFanin0(pObj), "test", 0);
    if (Abc_NtkObjNum(pNtkTemp) < 50) {
      Abc_NtkDelete(pNtkTemp);
      continue;
    }
    if (Abc_NtkPiNum(pNtkTemp) < 40) {
      Abc_NtkDelete(pNtkTemp);
      continue;
    }
    cout << j << endl;
    std::vector<char *> vNameYTemp;
    // numY = ((Abc_NtkPiNum(pNtkTemp) / 2) < 20) ? Abc_NtkPiNum(pNtkTemp) / 2 :
    // 20;
    for (int i = 0; i < 20; i++) {

      vNameYTemp.push_back(Abc_ObjName(Abc_NtkPi(pNtkTemp, i)));
    }

    cout << "##H\t" << pFileName << "_po" << j
         << " cone node#: " << Abc_NtkObjNum(pNtkTemp) << "\t";
    cout << " x#: " << Abc_NtkPiNum(pNtkTemp) - vNameYTemp.size() << "\t";
    cout << " y#: " << vNameYTemp.size() << endl;
    // for(unsigned i = 0; i < vNameYTemp.size(); i++) {
    //   cout << vNameYTemp[i] << " ";
    // }
    // cout << endl;
    cout << "-------------------------------" << endl;
    clk = clock();
    int long memInit = checkMem();
    int long memUsed;
    std::vector<Abc_Ntk_t *> vFunc;
    // vector<Abc_Ntk_t*> vFunc2;
    if (fChoice == 0) {
      rel2FuncSub(pNtkTemp, vNameYTemp, vFunc);
      memUsed = checkMem() - memInit;
    } else if (fChoice == 1) {
      rel2FuncInt(pNtkTemp, vNameYTemp, vFunc);
      memUsed = checkMem() - memInit;
    } else if (fChoice == 2) {
      rel2FuncInt2(pNtkTemp, vNameYTemp, vFunc);
      memUsed = checkMem() - memInit;
    } else if (fChoice == 3) {
      rel2FuncEQ(pNtkTemp, vNameYTemp, vFunc);
      memUsed = checkMem() - memInit;
    } else if (fChoice == 4) {
      rel2FuncEQ2(pNtkTemp, vNameYTemp, vFunc);
      //  rel2FuncEQ(pNtkTemp, vNameYTemp, vFunc2);
      memUsed = checkMem() - memInit;
    } else if (fChoice == 5) {
      rel2FuncEQ3(pNtkTemp, vNameYTemp, vFunc);
      memUsed = checkMem() - memInit;
    } else {
      // 6 aig->Bdd
      // 7 Bdd with cofactor
      // 8 Bdd with Minimize
      // 9 Bdd with Restrict
      // 10 Bdd with Constrain
      // 11 Bdd with Squeeze
      assert(fChoice - 6 >= 0);
      memUsed = rel2FuncBdd(pNtkTemp, vNameYTemp, vFunc, fChoice - 6);
      // rel2FuncEQ(pNtkTemp, vNameYTemp, vFunc2);
    }
    cout << "-------------------------------" << endl;
    cout << "function info final" << endl;
    cout << "-------------------------------" << endl;
    cout << "\tlev#\tnode#\tsupport#" << endl;
    cout << "-------------------------------" << endl;

    clk2 = clock();
    int levCount = 0;
    int funcCount = 0;
    int nodeCount = 0;
    int suppCount = 0;
    int nSuc = vNameYTemp.size() - vFunc.size();
    for (unsigned i = 0; i < vFunc.size(); i++) {
      if (!nSuc)
        cout << "##F\t";
      else
        cout << "##FD\t";
      cout << Abc_AigLevel(vFunc[i]) << "\t" << Abc_NtkObjNum(vFunc[i]) << "\t";
      cout << Abc_NtkPiNum(vFunc[i]) << endl;
      if (Abc_AigLevel(vFunc[i]) != 0) {
        levCount += Abc_AigLevel(vFunc[i]);
        nodeCount += Abc_NtkObjNum(vFunc[i]);
        suppCount += Abc_NtkPiNum(vFunc[i]);
        funcCount++;
      }
      //   cout << Abc_AigLevel(vFunc2[i]) << "\t" << Abc_NtkObjNum(vFunc[i]) <<
      //   "\t"; cout << Abc_NtkPiNum(vFunc2[i]) << endl;
      //   Abc_NtkCecSat(vFunc[i], vFunc2[i], 100000, 0);
      Abc_NtkDelete(vFunc[i]);
      // Abc_NtkDelete(vFunc2[i]);
    }
    for (int i = 0; i < nSuc; i++) {
      cout << "##FD\t";
      cout << MAX_AIG_NODE << "\t" << MAX_AIG_NODE << "\t";
      cout << MAX_AIG_NODE << endl;
      levCount += MAX_AIG_NODE;
      nodeCount += MAX_AIG_NODE;
      suppCount += MAX_AIG_NODE;
    }
    cout << "-------------------------------" << endl;
    double rAvgNode =
        (funcCount == 0) ? 0.0f : (double)nodeCount / (double)funcCount;
    double rAvgLev =
        (funcCount == 0) ? 0.0f : (double)levCount / (double)funcCount;
    double rAvgSupp =
        (funcCount == 0) ? 0.0f : (double)suppCount / (double)funcCount;
    double rMem = (double)memUsed / 1000.0f;
    double rTime = (double)(clk2 - clk) / CLOCKS_PER_SEC;
    avgMem += rMem;
    avgTime += rTime;

    if (levCount == 0)
      tZeroCount++;
    if (!nSuc) { // succ
      printf(
          "##S\t%s_po%d\t%d\t%d\t%d\t%.2lf\t%.2lf\t%.2lf\t%.2lfs\t%.2lf Mb\n",
          pFileName, j, levCount, nodeCount, suppCount, rAvgLev, rAvgNode,
          rAvgSupp, rTime, rMem);
      tSuc++;
      tNode += nodeCount;
      tLev += levCount;
      avgNode += rAvgNode;
      avgLev += rAvgLev;
    } else
      printf(
          "##SD\t%s_po%d\t%d\t%d\t%d\t%.2lf\t%.2lf\t%.2lf\t%.2lfs\t%.2lf Mb\n",
          pFileName, j, levCount, nodeCount, suppCount, rAvgLev, rAvgNode,
          rAvgSupp, rTime, rMem);
    cout << "-------------------------------" << endl;
    Abc_NtkDelete(pNtkTemp);
    cout << endl << endl << endl;
    tFunc++;
    printf("##TD\t%s\t%d\t%d\t%.2lf\t%.2lf\t%.2lfs\t%.2lf Mb\n", pFileName,
           tFunc, tSuc, avgLev / (tSuc - tZeroCount),
           avgNode / (tSuc - tZeroCount), avgTime / tFunc, avgMem / tFunc);
  }

  // call main function

  return 0;

usage:
  Abc_Print(-2, "usage: r2f [-vh] <file>\n");
  Abc_Print(-2, "\t-v       : verbosity [default = %d]\n", fVerbose);
  Abc_Print(-2, "\t-h       : print the command usage\n");
  Abc_Print(-2, "\tfile     : the SDIMACS file\n");
  return 1;
}

// called during ABC startup
static void init(Abc_Frame_t *pAbc) {
  Cmd_CommandAdd(pAbc, "Alcom", "r2f", r2f_Command, 1);
}

// called during ABC termination
static void destroy(Abc_Frame_t *pAbc) {}

// this object should not be modified after the call to Abc_FrameAddInitializer
static Abc_FrameInitializer_t r2f_frame_initializer = {init, destroy};

// register the initializer a constructor of a global object
// called before main (and ABC startup)
struct r2f_registrar {
  r2f_registrar() { Abc_FrameAddInitializer(&r2f_frame_initializer); }
} r2f_registrar_;

ABC_NAMESPACE_IMPL_END
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
