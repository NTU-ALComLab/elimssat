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

#include "abc.h"
#include "main.h"

#include "misc.hpp"
#include "subRel2Func.hpp"
#include "bddRel2Func.hpp"
#include "eqRel2Func.hpp"
#include "intRel2Func.hpp"
#include "intRel2Func2.hpp"

#define NAME_LEN 100

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
extern "C" void Abc_NtkCecSat( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int nConfLimit, int nInsLimit );

Abc_Ntk_t* createRelNtkAnd(Abc_Ntk_t*);
Abc_Ntk_t* createRelNtkOr(Abc_Ntk_t*);
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
    double clk, clkUsed;

    char * pFileName;
    //char * sNumY;
    int numY;

    int choice = atoi(argv[1]);
    vector<char*> vNameY;
    Abc_Ntk_t *pNtk;

    //////////////////////////////////////////////////////////////////////////
    // get the input file name

    pFileName = argv[2];
   /*
    numY = 10;
    pFileName = (char*) malloc(sizeof(char) * NAME_LEN);
    scanf("%s", pFileName);
    cout <<  "pFileName:" << pFileName << endl;
    sNumY = (char*) malloc(sizeof(char) * NAME_LEN);
    scanf("%s", sNumY);
    
       numY = atoi(sNumY);
    cout << "numY:" << numY << endl;

    for(int i = 0; i < numY; i++) {
        vNameY.push_back((char*) malloc(sizeof(char) * NAME_LEN));
        scanf("%s", vNameY[i]);
        cout << "y" << i << ": " <<vNameY[i] << endl;
    }
*/

    //////////////////////////////////////////////////////////////////////////
    // start the ABC framework
    Abc_Start();
    pAbc = Abc_FrameGetGlobalFrame();

    clk = (double) clock();
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
    Abc_Obj_t* pObj; 
    int j;

    Abc_NtkForEachPo(pNtk, pObj, j) {

        if(!Abc_ObjIsNode(Abc_ObjFanin0(pObj)))
            continue;
        Abc_Ntk_t* pNtkTemp = Abc_NtkCreateCone(pNtk, Abc_ObjFanin0(pObj), "test", 0);
        if(Abc_NtkObjNum(pNtkTemp) < 50) {
            Abc_NtkDelete(pNtkTemp);
            continue;
        }
        vector<char*> vNameYTemp;
/*
        int yCount = 0;
        for(unsigned i = 0; i < vNameY.size(); i++) {
            if(yCount >= Abc_NtkPiNum(pNtkTemp)) 
                break;
            if(Abc_NtkFindCi(pNtkTemp, vNameY[i]) != NULL) {
                vNameYTemp.push_back(vNameY[i]);
                yCount++;
            }
        }
  */     
        numY = ((Abc_NtkPiNum(pNtkTemp) / 2) < 20) ? Abc_NtkPiNum(pNtkTemp) / 2 : 20;
        for(int i = 0; i < numY; i++) {

            vNameYTemp.push_back(Abc_ObjName(Abc_NtkPi(pNtkTemp, i)));
        }
        
        cout << "Po" << j << " cone node#: " << Abc_NtkObjNum(pNtkTemp) << "\t";
        cout << " x#: " << Abc_NtkPiNum(pNtkTemp) - numY << "\t";
        cout << " y#: " << numY << endl;
        cout << "-------------------------------" << endl;
        
        vector<Abc_Ntk_t*> vFunc;
        //vector<Abc_Ntk_t*> vFunc2;
        if(choice == 0)
            rel2FuncSub(pNtkTemp, vNameYTemp, vFunc);
        else if( choice == 1) 
            rel2FuncInt(pNtkTemp, vNameYTemp, vFunc);
        else if( choice == 2) 
            rel2FuncInt2(pNtkTemp, vNameYTemp, vFunc);
        else if( choice == 3) 
            rel2FuncEQ(pNtkTemp, vNameYTemp, vFunc);
        else {
            rel2FuncBdd(pNtkTemp, vNameYTemp, vFunc);
           // rel2FuncEQ(pNtkTemp, vNameYTemp, vFunc2);
        }
        cout << "-------------------------------" << endl;
        cout << "function info final" << endl;
        cout << "-------------------------------" << endl;
        cout << "lev#\tnode#\tpi#" << endl;
        cout << "-------------------------------" << endl;
        
        for(unsigned i = 0; i < vFunc.size(); i++) {
            cout << Abc_AigLevel(vFunc[i]) << "\t" << Abc_NtkObjNum(vFunc[i]) << "\t";
            cout << Abc_NtkPiNum(vFunc[i]) << endl; 
         //   cout << Abc_AigLevel(vFunc2[i]) << "\t" << Abc_NtkObjNum(vFunc[i]) << "\t";
         //   cout << Abc_NtkPiNum(vFunc2[i]) << endl; 
          //  Abc_NtkCecSat(vFunc[i], vFunc2[i], 100000, 0);
            Abc_NtkDelete(vFunc[i]);
           // Abc_NtkDelete(vFunc2[i]);
        }
        cout << "-------------------------------" << endl;
        Abc_NtkDelete(pNtkTemp);
        cout << endl << endl << endl;
    }
    clkUsed = (double)clock() - clk;


    //////////////////////////////////////////////////////////////////////////
    // stop the ABC framework
    Abc_Stop();

    cout << "run time: " << clkUsed / CLOCKS_PER_SEC  << endl;
    return 0;
}

Abc_Ntk_t* createRelNtkAnd(Abc_Ntk_t* pNtk) {

    Abc_Ntk_t *pNtkNew = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    Abc_Obj_t *pObj, *pObjNew;
    int i;


    if(Abc_NtkPoNum(pNtk) < 2)
        return Abc_NtkDup(pNtk);

    Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNew);
    Abc_NtkForEachPi( pNtk, pObj, i )
    {
        pObjNew = Abc_NtkCreatePi( pNtkNew );
        // add name
        Abc_ObjAssignName( pObjNew, Abc_ObjName(pObj), NULL );
        // remember this PI in the old PIs
        pObj->pCopy = pObjNew;
    }
    ntkAddOne(pNtk, pNtkNew);
    vector<Abc_Obj_t*> vObj;
    Abc_NtkForEachPo( pNtk, pObj, i ) {
      //  pObjNew = Abc_NtkCreatePi( pNtkNew );
        // add name
      //  char yName[100];
      //  sprintf(yName, "_yPo_%s", Abc_ObjName(pObj));
      //  Abc_ObjAssignName( pObjNew, yName, NULL );
        pObjNew = Abc_ObjChild0Copy(pObj);
        vObj.push_back(pObjNew);
    }

    pObjNew = vObj[0];
    for(unsigned j = 1; j < vObj.size(); j++) {
        pObjNew = Abc_AigAnd((Abc_Aig_t*)pNtkNew->pManFunc, vObj[j], pObjNew);
    }
    Abc_Obj_t* pPo = Abc_NtkCreatePo(pNtkNew);
    char name[10];
    sprintf(name, "%s", "test");
    Abc_ObjAssignName(pPo, name, NULL);
    Abc_ObjAddFanin(pPo, pObjNew);
    pNtkNew = resyn(pNtkNew);
    Abc_NtkCheck(pNtkNew);
    return pNtkNew;
}
Abc_Ntk_t* createRelNtkOr(Abc_Ntk_t* pNtk) {

    Abc_Ntk_t *pNtkNew = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    Abc_Obj_t *pObj, *pObjNew;
    int i;


    if(Abc_NtkPoNum(pNtk) < 2)
        return Abc_NtkDup(pNtk);

    Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNew);
    Abc_NtkForEachPi( pNtk, pObj, i )
    {
        pObjNew = Abc_NtkCreatePi( pNtkNew );
        // add name
        Abc_ObjAssignName( pObjNew, Abc_ObjName(pObj), NULL );
        // remember this PI in the old PIs
        pObj->pCopy = pObjNew;
    }
    ntkAddOne(pNtk, pNtkNew);
    vector<Abc_Obj_t*> vObj;
    Abc_NtkForEachPo( pNtk, pObj, i ) {
 //       pObjNew = Abc_NtkCreatePi( pNtkNew );
        // add name
 //       char yName[100];
 //       sprintf(yName, "_yPo_%s", Abc_ObjName(pObj));
 //       Abc_ObjAssignName( pObjNew, yName, NULL );
        pObjNew = Abc_ObjChild0Copy(pObj);
        vObj.push_back(pObjNew);
    }

    pObjNew = vObj[0];
    for(unsigned j = 1; j < vObj.size(); j++) {
        pObjNew = Abc_AigOr((Abc_Aig_t*)pNtkNew->pManFunc, vObj[j], pObjNew);
    }
    Abc_Obj_t* pPo = Abc_NtkCreatePo(pNtkNew);
    char name[10];
    sprintf(name, "%s", "test");
    Abc_ObjAssignName(pPo, name, NULL);
    Abc_ObjAddFanin(pPo, pObjNew);
    pNtkNew = resyn(pNtkNew);
    Abc_NtkCheck(pNtkNew);
    return pNtkNew;
}
