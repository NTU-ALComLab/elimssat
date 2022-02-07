/**CFile****************************************************************

  FileName    [demo.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [ABC as a static library.]

  Synopsis    [A demo program illustrating the use of ABC as a static library.]

  Author      [Alan Mishchenko]

  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: demo.c,v 1.00 2005/11/14 00:00:00 alanmi Exp $]

 ***********************************************************************/

#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <vector>
#include <algorithm>

#include "abc.h"
#include "main.h"

#include "misc.hpp"
#include "subRel2Func.hpp"
#include "bddRel2Func.hpp"
#include "eqRel2Func.hpp"
#include "intRel2Func.hpp"
#include "intRel2Func2.hpp"

#define NAME_LEN 100

extern "C" void Abc_NtkCecSat( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int nConfLimit, int nInsLimit );
extern "C" Abc_Ntk_t * Abc_NtkTransRel( Abc_Ntk_t * pNtk, int fInputs, int fVerbose );

static inline double cpuTimeSec(void) {
    return (double)clock() / CLOCKS_PER_SEC; }

using namespace std;
////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// procedures to start and stop the ABC framework
// (should be called before and after the ABC procedures are called)
extern "C" void   Abc_Start();
extern "C" void   Abc_Stop();

extern "C" Abc_Ntk_t * Abc_NtkMulti( Abc_Ntk_t * pNtk, int nThresh, int nFaninMax, int fCnf, int fMulti, int fSimple, int fFactor );
extern "C" void Abc_NtkShow( Abc_Ntk_t * pNtk, int fGateNames, int fSeq, int fUseReverse );
extern "C" int Abc_AigLevel(Abc_Ntk_t*);


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [The main() procedure.]

  Description [This procedure compiles into a stand-alone program for 
  DAG-aware rewriting of the AIGs. A BLIF or PLA file to be considered
  for rewriting should be given as a command-line argument. Implementation 
  of the rewriting is inspired by the paper: Per Bjesse, Arne Boralv, 
  "DAG-aware circuit compression for formal verification", Proc. ICCAD 2004.]

  SideEffects []

  SeeAlso     []

 ***********************************************************************/
int main( int argc, char * argv[] )
{
    // parameters
    // variables
    Abc_Frame_t* pAbc;
    char Command[1000];
    clock_t clk, clk2;

    char * pFileName;
    //char * sNumY;
    //int numY;

    int choice = atoi(argv[1]);
   // vector<char*> vNameY;
    Abc_Ntk_t *pNtk;

    //////////////////////////////////////////////////////////////////////////
    // get the input file name

    pFileName = argv[2];
   
  //  numY = 10;
  //  pFileName = (char*) malloc(sizeof(char) * NAME_LEN);
  //  scanf("%s", pFileName);
  //  cout <<  "pFileName:" << pFileName << endl;
 //   sNumY = (char*) malloc(sizeof(char) * NAME_LEN);
   // scanf("%s", sNumY);
    
    //   numY = atoi(sNumY);
  //  cout << "numY:" << numY << endl;

//    for(int i = 0; i < numY; i++) {
      //  vNameY.push_back((char*) malloc(sizeof(char) * NAME_LEN));
     //   scanf("%s", vNameY[i]);
   //     cout << "y" << i << ": " <<vNameY[i] << endl;
 //   }


    //////////////////////////////////////////////////////////////////////////
    // start the ABC framework
    Abc_Start();
    pAbc = Abc_FrameGetGlobalFrame();

    //clk = (double) clock();
    //////////////////////////////////////////////////////////////////////////
    // read the file
    sprintf( Command, "read %s", pFileName );
    if ( Cmd_CommandExecute( pAbc, Command ) )
    {
        fprintf( stdout, "Cannot execute command \"%s\".\n", Command );
        return 1;
    }

    pNtk = Abc_FrameReadNtk(pAbc);
    pNtk = Abc_NtkStrash(pNtk, 0, 0, 0);
    
   // pNtk = createRelNtk(pNtk);
    Abc_Obj_t* pObj; 
    int j;

    int tFunc= 0;
    int tSuc = 0;
    int tZeroCount = 0;
    long int tNode = 0;
    long int tLev = 0;
    double avgNode = 0.0f;
    double avgLev = 0.0f;
    double avgMem = 0.0f;
    double avgTime = 0.0f;
    
    Abc_NtkForEachPo(pNtk, pObj, j) {
        

        if(!Abc_ObjIsNode(Abc_ObjFanin0(pObj)))
            continue;
        Abc_Ntk_t* pNtkTemp = Abc_NtkCreateCone(pNtk, Abc_ObjFanin0(pObj), "test", 0);
        if(Abc_NtkObjNum(pNtkTemp) < 50) {
            Abc_NtkDelete(pNtkTemp);
            continue;
        }
        if(Abc_NtkPiNum(pNtkTemp) < 40) {
            Abc_NtkDelete(pNtkTemp);
            continue;
        }
         //   continue;
       // vector<char*> vNameY;
       // vector<char*> vNameYTemp;
       // int k;
       // Abc_Obj_t *pPi;
       // Abc_NtkForEachPi(pNtkTemp, pPi, k) {
         //   vNameY.push_back(Abc_ObjName(pPi));
       // }

       // random_shuffle(vNameY.begin(), vNameY.end());
        //for(unsigned i = 0; i < 20; i++) {
         //   if( i > vNameY.size() - 1)
          //      break;
        //    vNameYTemp.push_back(vNameY[i]);
      //  }
        

 //       int yCount = 0;
   //     for(unsigned i = 0; i < vNameY.size(); i++) {
     //       if(yCount >= Abc_NtkPiNum(pNtkTemp)) 
       //         break;
         //   if(Abc_NtkFindCi(pNtkTemp, vNameY[i]) != NULL) {
           //     vNameYTemp.push_back(vNameY[i]);
             //   yCount++;
           // }
       // }
       
        vector<char*> vNameYTemp;
       // numY = ((Abc_NtkPiNum(pNtkTemp) / 2) < 20) ? Abc_NtkPiNum(pNtkTemp) / 2 : 20;
        for(int i = 0; i < 20; i++) {

            vNameYTemp.push_back(Abc_ObjName(Abc_NtkPi(pNtkTemp, i)));
        }
        
        cout << "##H\t" << pFileName << "_po" << j << " cone node#: " << Abc_NtkObjNum(pNtkTemp) << "\t";
        cout << " x#: " << Abc_NtkPiNum(pNtkTemp) - vNameYTemp.size() << "\t";
        cout << " y#: " << vNameYTemp.size() << endl;
        //for(unsigned i = 0; i < vNameYTemp.size(); i++) {
         //   cout << vNameYTemp[i] << " ";
       // }
       // cout << endl;
        cout << "-------------------------------" << endl;
        clk = clock();
        int long memInit = checkMem();
        int long memUsed;
        vector<Abc_Ntk_t*> vFunc;
       // vector<Abc_Ntk_t*> vFunc2;
        if(choice == 0) {
            rel2FuncSub(pNtkTemp, vNameYTemp, vFunc);
            memUsed = checkMem() - memInit;
        }
        else if( choice == 1) {
            rel2FuncInt(pNtkTemp, vNameYTemp, vFunc);
            memUsed = checkMem() - memInit;
        }
        else if( choice == 2) {
            rel2FuncInt2(pNtkTemp, vNameYTemp, vFunc);
            memUsed = checkMem() - memInit;
        }
        else if( choice == 3) {
            rel2FuncEQ(pNtkTemp, vNameYTemp, vFunc);
            memUsed = checkMem() - memInit;
        }
        else if( choice == 4) {
            rel2FuncEQ2(pNtkTemp, vNameYTemp, vFunc);
          //  rel2FuncEQ(pNtkTemp, vNameYTemp, vFunc2);
            memUsed = checkMem() - memInit;
        }
        else if( choice == 5) {
            rel2FuncEQ3(pNtkTemp, vNameYTemp, vFunc);
            memUsed = checkMem() - memInit;
        }
        else {
            // 6 aig->Bdd
            // 7 Bdd with cofactor
            // 8 Bdd with Minimize
            // 9 Bdd with Restrict
            // 10 Bdd with Constrain
            // 11 Bdd with Squeeze
            assert(choice - 6 >= 0);
            memUsed = rel2FuncBdd(pNtkTemp, vNameYTemp, vFunc, choice - 6);
           // rel2FuncEQ(pNtkTemp, vNameYTemp, vFunc2);
        }
        cout << "-------------------------------" << endl;
        cout << "function info final" << endl;
        cout << "-------------------------------" << endl;
        cout << "\tlev#\tnode#\tsupport#" << endl;
        cout << "-------------------------------" << endl;
           
        clk2 = clock();
       // if(choice <= 4)
        int levCount = 0;
        int funcCount = 0;
        int nodeCount = 0;
        int suppCount= 0;
        int nSuc = vNameYTemp.size() - vFunc.size();
        for(unsigned i = 0; i < vFunc.size(); i++) {
            if(!nSuc)
                cout << "##F\t";
            else cout << "##FD\t";
            cout << Abc_AigLevel(vFunc[i]) << "\t" << Abc_NtkObjNum(vFunc[i]) << "\t";
            cout << Abc_NtkPiNum(vFunc[i]) << endl; 
            if(Abc_AigLevel(vFunc[i]) != 0) {
                levCount += Abc_AigLevel(vFunc[i]);
                nodeCount += Abc_NtkObjNum(vFunc[i]);
                suppCount += Abc_NtkPiNum(vFunc[i]);
                funcCount++;
            }
         //   cout << Abc_AigLevel(vFunc2[i]) << "\t" << Abc_NtkObjNum(vFunc[i]) << "\t";
         //   cout << Abc_NtkPiNum(vFunc2[i]) << endl; 
         //   Abc_NtkCecSat(vFunc[i], vFunc2[i], 100000, 0);
            Abc_NtkDelete(vFunc[i]);
           // Abc_NtkDelete(vFunc2[i]);
        }
        for(int i = 0; i < nSuc; i++) {
            cout << "##FD\t";
            cout << MAX_AIG_NODE << "\t" << MAX_AIG_NODE << "\t";
            cout << MAX_AIG_NODE << endl; 
            levCount += MAX_AIG_NODE;
            nodeCount += MAX_AIG_NODE;
            suppCount += MAX_AIG_NODE;
        }
        cout << "-------------------------------" << endl;
        double rAvgNode = (funcCount == 0)? 0.0f : (double)nodeCount/(double)funcCount;
        double rAvgLev = (funcCount == 0)? 0.0f : (double)levCount/(double)funcCount;
        double rAvgSupp = (funcCount == 0)? 0.0f: (double)suppCount/(double)funcCount;
        double rMem = (double)memUsed/1000.0f;
        double rTime = (double)(clk2 - clk) / CLOCKS_PER_SEC;
        avgMem += rMem; 
        avgTime += rTime;
        
        if(levCount == 0)
            tZeroCount++;
        if(!nSuc) { // succ
            printf("##S\t%s_po%d\t%d\t%d\t%d\t%.2lf\t%.2lf\t%.2lf\t%.2lfs\t%.2lf Mb\n", pFileName, j, levCount, nodeCount, suppCount,  rAvgLev, rAvgNode, rAvgSupp, rTime, rMem);
            tSuc++;
            tNode+= nodeCount;
            tLev += levCount;
            avgNode += rAvgNode;
            avgLev += rAvgLev;
        }
        else 
            printf("##SD\t%s_po%d\t%d\t%d\t%d\t%.2lf\t%.2lf\t%.2lf\t%.2lfs\t%.2lf Mb\n", pFileName, j, levCount, nodeCount, suppCount,  rAvgLev, rAvgNode, rAvgSupp, rTime, rMem);
        cout << "-------------------------------" << endl;
        Abc_NtkDelete(pNtkTemp);
        cout << endl << endl << endl;
        tFunc++;
        printf("##TD\t%s\t%d\t%d\t%.2lf\t%.2lf\t%.2lfs\t%.2lf Mb\n", pFileName, tFunc, tSuc, avgLev/(tSuc - tZeroCount), avgNode/(tSuc - tZeroCount), avgTime/tFunc, avgMem/tFunc);
    }
    printf("##T\t%s\t%d\t%d\t%.2lf\t%.2lf\t%.2lfs\t%.2lf Mb\n", pFileName, tFunc, tSuc, avgLev/(tSuc- tZeroCount), avgNode/(tSuc - tZeroCount), avgTime/tFunc, avgMem/tFunc);
   // clkUsed = (double)clock() - clk;


    //////////////////////////////////////////////////////////////////////////
    // stop the ABC framework
    Abc_Stop();

  //  cout << "run time: " << clkUsed / CLOCKS_PER_SEC  << endl;
    return 0;
}

/*
int main( int argc, char * argv[] )
{
    // parameters
    // variables
    Abc_Frame_t* pAbc;
    char Command[1000];
    int clk, clkUsed;

    char * pFileName;
    //char * sNumY;
    int numY;
        
    int long memInit = checkMem();
    int long memUsed;

    int choice = atoi(argv[1]);
    vector<char*> vNameY;
    Abc_Ntk_t *pNtk;

    //////////////////////////////////////////////////////////////////////////
    // get the input file name

    pFileName = argv[2];

    //////////////////////////////////////////////////////////////////////////
    // start the ABC framework
    Abc_Start();
    pAbc = Abc_FrameGetGlobalFrame();

    clk = clock();
    //////////////////////////////////////////////////////////////////////////
    // read the file
    sprintf( Command, "read %s", pFileName );
    if ( Cmd_CommandExecute( pAbc, Command ) )
    {
        fprintf( stdout, "Cannot execute command \"%s\".\n", Command );
        return 1;
    }

    pNtk = Abc_FrameReadNtk(pAbc);
    pNtk = Abc_NtkStrash(pNtk, 0, 0, 0);

    Abc_Ntk_t* pNtkRel = ntkTransRel(pNtk, 0, 0);
//    Abc_Ntk_t* pNtkRel = Abc_NtkTransRel(pNtk, 0, 0);
  //  Abc_NtkShow(pNtkRel, 0, 0, 0);
    //Abc_Ntk_t* pNtkRel = createRelNtk(pNtk);

    assert(Abc_NtkPoNum(pNtkRel) == 1);

    Abc_Obj_t* pObj; 
    int j;

    Abc_NtkForEachPi(pNtkRel, pObj, j) {
        if(strncmp(Abc_ObjName(pObj), "_yPo_", 5) == 0)
            //  cout << &Abc_ObjName(pObj)[5] << endl;
            vNameY.push_back(Abc_ObjName(pObj));
    }
    numY = vNameY.size();

    cout << "Po" << j << " cone node#: " << Abc_NtkObjNum(pNtkRel) << "\t";
    cout << " x#: " << Abc_NtkPiNum(pNtkRel) - numY << "\t";
    cout << " y#: " << numY << endl;
    cout << "-------------------------------" << endl;

    long int levCount = 0;
    long int nodeCount = 0;
    long int suppCount = 0;

    vector<Abc_Ntk_t*> vFunc;
        if(choice == 0) {
            rel2FuncSub(pNtkRel, vNameY, vFunc);
            memUsed = checkMem() - memInit;
        }
        else if( choice == 1) {
            rel2FuncInt(pNtkRel, vNameY, vFunc);
            memUsed = checkMem() - memInit;
        }
        else if( choice == 2) {
            rel2FuncInt2(pNtkRel, vNameY, vFunc);
            memUsed = checkMem() - memInit;
        }
        else if( choice == 3) {
            rel2FuncEQ(pNtkRel, vNameY, vFunc);
            memUsed = checkMem() - memInit;
        }
        else if( choice == 4) {
            rel2FuncEQ2(pNtkRel, vNameY, vFunc);
          //  rel2FuncEQ(pNtkRel, vNameY, vFunc2);
            memUsed = checkMem() - memInit;
        }
        else if( choice == 5) {
            rel2FuncEQ3(pNtkRel, vNameY, vFunc);
            memUsed = checkMem() - memInit;
        }
        else {
            // 6 aig->Bdd
            // 7 Bdd with cofactor
            // 8 Bdd with Minimize
            // 9 Bdd with Restrict
            // 10 Bdd with Constrain
            // 11 Bdd with Squeeze
            assert(choice - 6 >= 0);
            memUsed = rel2FuncBdd(pNtkRel, vNameY, vFunc, choice - 6);
           // rel2FuncEQ(pNtkTemp, vNameYTemp, vFunc2);
        }
    
    //checkEqu(pNtk, vFunc, vNameY);
    cout << "-------------------------------" << endl;
    cout << "function info final" << endl;
    cout << "-------------------------------" << endl;
    cout << "lev#\tnode#\tpi#" << endl;
    cout << "-------------------------------" << endl;

    for(unsigned i = 0; i < vFunc.size(); i++) {
        cout << Abc_AigLevel(vFunc[i]) << "\t" << Abc_NtkObjNum(vFunc[i]) << "\t";
        cout << Abc_NtkPiNum(vFunc[i]) << endl; 
        levCount += Abc_AigLevel(vFunc[i]);
        nodeCount += Abc_NtkObjNum(vFunc[i]);
        suppCount += Abc_NtkPiNum(vFunc[i]);
        Abc_NtkDelete(vFunc[i]);
    }
    cout << "-------------------------------" << endl;
    

    printf("##S\t%s%ld\t%ld\t%ld\t%.2lf\t%.2lf\t%.2lf\t%.2lf Mb\n", pFileName, levCount, nodeCount, suppCount,  (double)levCount/vNameY.size(), (double)nodeCount/vNameY.size(), (double)suppCount/vNameY.size(), (double)memUsed/1000.0f);
    Abc_NtkDelete(pNtkRel);
    cout << endl << endl << endl;


    clkUsed = clock() - clk;


    //////////////////////////////////////////////////////////////////////////
    // stop the ABC framework
    Abc_Stop();

    cout << "run time: " << clkUsed / CLOCKS_PER_SEC  << endl;
    return 0;
}*/
