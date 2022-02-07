/**HFile****************************************************************
  FileName    [util.h]
  SystemName  [ABC: Logic synthesis and verification system.]
  PackageName []
  Synopsis    [Utility box]
  Author      [Kuan-Hua Tu(isuis3322@gmail.com)]
  
  Affiliation [NTU]
  Date        [2018.04.24]
***********************************************************************/

#ifndef ABC__ext__util_h
#define ABC__ext__util_h

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "base/main/main.h"
#include "bdd/extrab/extraBdd.h" // 
#include "sat/cnf/cnf.h"

////////////////////////////////////////////////////////////////////////
///                         DECLARATION                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== util.c ===========================================================*/
// Ntk 
extern Abc_Ntk_t * Util_CreatNtkFromTruthBin( char * pTruth );
extern Abc_Ntk_t * Util_CreatConstant0Ntk( int nInput );
extern Abc_Ntk_t * Util_CreatConstant1Ntk( int nInput );
extern Abc_Ntk_t * Util_NtkAllocStrashWithName( char * NtkName );
extern Abc_Ntk_t * Util_NtkCreateCounter( int nInput );
extern Abc_Ntk_t * Util_NtkAnd( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2 );
extern Abc_Ntk_t * Util_NtkOr ( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2 );
extern Abc_Ntk_t * Util_NtkMaj( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, Abc_Ntk_t * pNtk3 );
extern Abc_Ntk_t * Util_NtkExistQuantify( Abc_Ntk_t * pNtk, int pi );
extern Abc_Ntk_t * Util_NtkExistQuantifyPis( Abc_Ntk_t * pNtk, Vec_Int_t * pPiIndex );
extern Abc_Ntk_t * Util_NtkExistQuantifyPisByBDD( Abc_Ntk_t * pNtk, Vec_Int_t * pPiIndex );
extern Abc_Ntk_t * Util_NtkExistQuantifyPis_BDD( Abc_Ntk_t * pNtk, Vec_Int_t * pPiIndex );


extern void Util_NtkCreateMultiPi( Abc_Ntk_t * pNtk, int n );
extern void Util_NtkCreateMultiPiWithPrefix( Abc_Ntk_t * pNtk, int n, char * prefix );
extern void Util_NtkCreateMultiPo( Abc_Ntk_t * pNtk, int n );
extern void Util_NtkCreatePiFrom ( Abc_Ntk_t * pNtkDest, Abc_Ntk_t * pNtkOri, char * pSuffix );
extern void Util_NtkConst1SetCopy( Abc_Ntk_t * pNtkSource, Abc_Ntk_t * pNtkDes );
extern void Util_NtkPiSetCopy    ( Abc_Ntk_t * pNtkSource, Abc_Ntk_t * pNtkDes, int start_index );
// simulate
extern void Util_NtkModelAlloc( Abc_Ntk_t* pNtk );
extern void Util_SetCiModelAs0( Abc_Ntk_t* pNtk );
extern void Util_SetCiModelAs1( Abc_Ntk_t* pNtk );
extern unsigned int Util_NtkSimulate( Abc_Ntk_t* pNtk );
extern void Util_MaxInputPatternWithOuputAs0( Abc_Ntk_t* pNtk, Vec_Int_t * pCaredPi );

extern void Util_PrintTruthtable( Abc_Ntk_t * pNtk );

extern Abc_Ntk_t * Util_NtkBalance( Abc_Ntk_t * pNtk, int fDelete );
extern Abc_Ntk_t * Util_NtkResyn2( Abc_Ntk_t * pNtk, int fDelete );
extern Abc_Ntk_t * Util_NtkDc2( Abc_Ntk_t * pNtk, int fDelete );
extern Abc_Ntk_t * Util_NtkDrwsat( Abc_Ntk_t * pNtk, int fDelete );
extern Abc_Ntk_t * Util_NtkIFraig( Abc_Ntk_t * pNtk, int fDelete );
extern Abc_Ntk_t * Util_NtkDFraig( Abc_Ntk_t * pNtk, int fDelete, int fDoSparse);
extern Abc_Ntk_t *Util_NtkGiaFraig(Abc_Ntk_t *pNtk, int fDelete);
extern Abc_Ntk_t *Util_NtkGiaSweep(Abc_Ntk_t *pNtk, int fDelete);
extern Abc_Ntk_t * Util_NtkCollapse( Abc_Ntk_t * pNtk, int fBddSizeMAx, int fDelete );
extern Abc_Ntk_t * Util_NtkStrash( Abc_Ntk_t * pNtk, int fDelete );

extern void Util_NtkRewrite( Abc_Ntk_t * pNtk );
extern void Util_NtkRewriteZ( Abc_Ntk_t * pNtk );
extern void Util_NtkRefactor( Abc_Ntk_t * pNtk );
extern void Util_NtkRefactorZ( Abc_Ntk_t * pNtk );
// Obj
extern Abc_Obj_t * Util_NtkCreatePoWithName( Abc_Ntk_t * pNtk, char * PoName, char * pSuffix );
extern Abc_Obj_t * Util_NtkAppend(Abc_Ntk_t * pNtkDes, Abc_Ntk_t * pNtkSource);
extern Abc_Obj_t * Util_NtkAppendWithPiOrder( Abc_Ntk_t * pNtkDes, Abc_Ntk_t * pNtkSource, int start_index );

extern Abc_Obj_t * Util_AigNtkMaj( Abc_Ntk_t * pNtk, Abc_Obj_t * p0, Abc_Obj_t * p1, Abc_Obj_t * p2 );
extern Abc_Obj_t * Util_AigNtkAnd( Abc_Ntk_t * pNtk, Abc_Obj_t * p0, Abc_Obj_t * p1 );
extern Abc_Obj_t * Util_AigNtkOr ( Abc_Ntk_t * pNtk, Abc_Obj_t * p0, Abc_Obj_t * p1 );
extern Abc_Obj_t * Util_AigNtkXor( Abc_Ntk_t * pNtk, Abc_Obj_t * p0, Abc_Obj_t * p1 );
extern Abc_Obj_t * Util_AigNtkEqual( Abc_Ntk_t * pNtk, Abc_Obj_t * p0, Abc_Obj_t * p1 );
extern Abc_Obj_t * Util_AigNtkImply( Abc_Ntk_t * pNtk, Abc_Obj_t * p0, Abc_Obj_t * p1 );
extern Abc_Obj_t * Util_AigNtkAdderSum  ( Abc_Ntk_t * pNtk, Abc_Obj_t * p0, Abc_Obj_t * p1, Abc_Obj_t * c );
extern Abc_Obj_t * Util_AigNtkAdderCarry( Abc_Ntk_t * pNtk, Abc_Obj_t * p0, Abc_Obj_t * p1, Abc_Obj_t * c );
extern Vec_Ptr_t * Util_AigNtkAdder     ( Abc_Ntk_t * pNtk, Vec_Ptr_t * p1, Vec_Ptr_t * p2 );
extern Vec_Ptr_t * Util_AigNtkCounter   ( Abc_Ntk_t * pNtk, Vec_Ptr_t * pIn, int index_begin, int index_end );

extern Abc_Obj_t * Abc_AigConst0( Abc_Ntk_t * pNtk );
extern Abc_Obj_t * Util_AigMaj( Abc_Aig_t * pMan, Abc_Obj_t * p0, Abc_Obj_t * p1, Abc_Obj_t * p2 );

extern void Util_ObjPrint( Abc_Obj_t * pObj );

extern void Util_CollectPi( Vec_Ptr_t * vPi, Abc_Ntk_t * pNtk, int index_start, int index_end );
// SAT
extern int Util_NtkSat(Abc_Ntk_t * pNtk, int fVerbose);
extern int Util_SatResultToBool(int SatResult);
extern int Util_NtkIsEqual( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2);
// fork process
extern int Util_CallProcess(char *command, int fVerbose, char *exec_command, ...);
// 
extern void Vec_IntPtrFree( Vec_Ptr_t * vVecInts );
// 
typedef char Bool;
extern void Util_BitSet_setValue( unsigned int * bitset, int index, Bool value );
extern Bool Util_BitSet_getValue( unsigned int * bitset, int index );
extern void Util_BitSet_print( unsigned int bitset );

ABC_NAMESPACE_HEADER_END
#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
