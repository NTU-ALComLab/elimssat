#include <iostream> 

#include "CnfConvert.hpp"
#include "misc.hpp"
//#define _DEBUG

using namespace std;

CnfConvert::CnfConvert(Abc_Ntk_t *p) {
    pNtk = p;
    ntkToCnf();
}

int
CnfConvert::ntkToCnf(Abc_Ntk_t *p) {
    pNtk = p;
    return ntkToCnf();
}



void 
CnfConvert::printLits(const vector<Lit> &lits) {

    if(lits.size() < 1)
        return;
    cout << "(";
    for(unsigned j = 0; j < lits.size()-1; j++) {
        if(sign(lits[j]))
            cout << "-";
        else 
            cout << " ";
        cout << var(lits[j]) + 1 << " + " ;
    }
    if(sign(lits[lits.size()-1]))
        cout << "-";
    else 
        cout << " ";
    cout << var(lits[lits.size()-1]) + 1;
    cout << ")" << endl;
    return;
}
void
CnfConvert::printCnf() {
    for(unsigned i = 0; i < pCnf.size(); i++) {
        printLits(pCnf[i]);
    }
}

void 
CnfConvert::addNodeClauses(char *pSop0, char *pSop1, Abc_Obj_t* pNode) {
    Abc_Obj_t * pFanin;
    int i, c, nFanins;
    char * pCube;

    //veci* lits;
    vector<Lit> lits;


    nFanins = Abc_ObjFaninNum( pNode );
    assert( nFanins == Abc_SopGetVarNum( pSop0 ) );
    
    if ( Cudd_Regular(pNode->pData) == Cudd_ReadOne((DdManager*)pNode->pNtk->pManFunc) )
    {
        lits.clear();
        if ( !Cudd_IsComplement(pNode->pData) )
            lits.push_back(Lit(pNode->Id));
        else
            lits.push_back(~Lit(pNode->Id));
        pCnf.push_back(lits);
        mInter[pNode->Id] = true;
        return;
    }
 
    // add pCnf for the negative phase
    for ( c = 0; ; c++ )
    {
        // get the cube
        pCube = pSop0 + c * (nFanins + 3);
        if ( *pCube == 0 )
            break;
        // add the clause
        
        lits.clear();
        Abc_ObjForEachFanin( pNode, pFanin, i )
        {
            maxVar = (pFanin->Id > maxVar)? pFanin->Id : maxVar;
            if ( pCube[i] == '0' )
                lits.push_back(Lit(pFanin->Id));
            else if ( pCube[i] == '1' )
                lits.push_back(~Lit(pFanin->Id));
            mInter[pFanin->Id] = true;
        }
        lits.push_back(~Lit(pNode->Id));
        pCnf.push_back(lits);
        mInter[pNode->Id] = true;
    }

    // add pCnf for the positive phase
    for ( c = 0; ; c++ )
    {
        // get the cube
        pCube = pSop1 + c * (nFanins + 3);
        if ( *pCube == 0 )
            break;
        // add the clause
        lits.clear();
        Abc_ObjForEachFanin( pNode, pFanin, i )
        {
            maxVar = (pFanin->Id > maxVar)? pFanin->Id : maxVar;
            if ( pCube[i] == '0' )
                lits.push_back(Lit(pFanin->Id));
            else if ( pCube[i] == '1' )
                lits.push_back(~Lit(pFanin->Id));
            mInter[pFanin->Id] = true;
        }
        lits.push_back(Lit(pNode->Id));
        pCnf.push_back(lits);
        mInter[pNode->Id] = true;
    }
}

int
CnfConvert::ntkToCnf() {
    
    Mem_Flex_t * pMmFlex;
    Abc_Obj_t * pNode;
    Vec_Str_t * vCube;
    char * pSop0, * pSop1;
    int fAllPrimes = 0;
    int i;
   
    assert( Abc_NtkIsBddLogic(pNtk) );
    
    pMmFlex = Mem_FlexStart();
    vCube   = Vec_StrAlloc( 100 );
    maxVar = -1;

/*
    printf("\npi node ===============");
    Abc_NtkForEachPi(pNtk, pNode, i) {
        printf("%d, \n", pNode->Id);
    }
    printf("\npo node ===============");
    Abc_NtkForEachPo(pNtk, pNode, i) {
        printf("%d, \n", pNode->Id);
    }
    printf("\nLatch node ===============");
    Abc_NtkForEachLatch(pNtk, pNode, i) {
        printf("%d, \n", pNode->Id);
    }
    printf("\nLatchIn node ===============");
    Abc_NtkForEachLatchInput(pNtk, pNode, i) {
        printf("%d, \n", pNode->Id);
    }
    printf("\nLatchOut node ===============");
    Abc_NtkForEachLatchOutput(pNtk, pNode, i) {
        printf("%d, \n", pNode->Id);
    }
    printf("\nnode ===============");
    Abc_NtkForEachNode(pNtk, pNode, i) {
        printf("%d, \n", pNode->Id);
    }
    printf("\nObj, node ===============");
    Abc_NtkForEachObj(pNtk, pNode, i) {
        printf("%d, \n", pNode->Id);
    }
    printf("\n");
    */
    // ============== bfs =================================
    // add pCnf for each internal nodes
    Vec_Ptr_t *vNodes = ntkBfs(pNtk);
    void *pObj;
    Vec_PtrForEachEntry( void*, vNodes, pObj, i ) {
        pNode = (Abc_Obj_t*) pObj;
        // derive SOPs for both phases of the node
        maxVar = (pNode->Id > maxVar)? pNode->Id : maxVar;
        Abc_NodeBddToCnf( pNode, pMmFlex, vCube, fAllPrimes, &pSop0, &pSop1 );
        addNodeClauses(pSop0, pSop1, pNode);
    }
    Vec_PtrFree(vNodes);
    // ============== bfs end =================================
    // ============== dfs =================================
    /*Vec_Ptr_t *vNodes = Abc_NtkDfs(pNtk, 1);
    void *pObj;
    Vec_PtrForEachEntry( vNodes, pObj, i ) {
        pNode = (Abc_Obj_t*) pObj;
        // derive SOPs for both phases of the node
        maxVar = (pNode->Id > maxVar)? pNode->Id : maxVar;
        Abc_NodeBddToCnf( pNode, pMmFlex, vCube, fAllPrimes, &pSop0, &pSop1 );
        addNodeClauses(pSop0, pSop1, pNode);
    }
    Vec_PtrFree(vNodes);*/
    // ============== dfs end =================================
    // ============== ori  =================================
    
    /*Abc_NtkForEachNode( pNtk, pNode, i )
    {
        // derive SOPs for both phases of the node
        maxVar = (pNode->Id > maxVar)? pNode->Id : maxVar;
        Abc_NodeBddToCnf( pNode, pMmFlex, vCube, fAllPrimes, &pSop0, &pSop1 );
        addNodeClauses(pSop0, pSop1, pNode);
    }*/
    // ============== ori  end =================================
#ifdef _DEBUG    
    printCnf();
#endif
    Vec_StrFree( vCube );
    Mem_FlexStop2( pMmFlex );
    
    return maxVar;
}

