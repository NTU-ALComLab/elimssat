#include "synthesisUtil.h"
#include <cstdarg>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

static void collectTFORecur(Abc_Ntk_t *pNtk, Vec_Int_t *pScope,
                            Abc_Obj_t *pObj);
static void writeCnfClause(FILE *out, Abc_Ntk_t *pNtk, int numPo);
static pid_t C_PID;

static void (*orig_sighandler)(int) = NULL;
void sigintHandler(int signal_number) {
  kill(C_PID, SIGTERM);
  orig_sighandler(signal_number);
}

void callProcess(char *command, int fVerbose, char *exec_command, ...) {
  int status;
  struct sigaction orig_sig;
  sigaction(SIGINT, NULL, &orig_sig);
  orig_sighandler = orig_sig.sa_handler;
  signal(SIGINT, sigintHandler);
  va_list ap;
  char *args[256];
  // parse variable length arguments
  int i = 0;
  args[i] = exec_command;
  va_start(ap, exec_command);
  do {
    i++;
    args[i] = va_arg(ap, char*);
  } while (args[i] != NULL);

  C_PID = fork();
  if (C_PID == -1) {
    perror("fork()");
    exit(-1);
  } else if (C_PID == 0) { // child process
    if (!fVerbose) {
      int fd = open("/dev/null", O_WRONLY);
      dup2(fd, 1);
      dup2(fd, 2);
      close(fd);
    }
    execvp(command, args);
  } else {
    wait(&status); // wait for child process
    if (!WIFEXITED(status)) {
      printf("forked process execution failed\n");
      exit(-1);
    } else if (WIFSIGNALED(status)) {
      printf("forked process terminated by signal\n");
      exit(-1);
    } else {
      if (WEXITSTATUS(status)) {
        exit(-1);
      }
    }
    signal(SIGINT, orig_sighandler);
  }
}

Vec_Int_t *collectTFO(Abc_Ntk_t *pNtk, Vec_Int_t *pScope) {
  int entry, index;
  Vec_Int_t *pScopeNew;
  Abc_Obj_t *pObj;
  pScopeNew = Vec_IntAlloc(pScope->nSize);
  Abc_NtkIncrementTravId(pNtk);
  Vec_IntForEachEntry(pScope, entry, index) {
    pObj = Abc_NtkObj(pNtk, entry);
    collectTFORecur(pNtk, pScopeNew, pObj);
    Vec_IntPush(pScopeNew, Abc_ObjId(pObj));
  }
  return pScopeNew;
}

void writeQdimacs(Abc_Ntk_t *pNtk, Vec_Int_t *pIndex, char *pFileName) {
  Abc_Obj_t *pObj;
  int index, entry;
  Vec_Int_t *vExists = Vec_IntAlloc(0);
  Vec_Int_t *vForalls = Vec_IntAlloc(0);
  Abc_NtkIncrementTravId(pNtk);
  Vec_IntForEachEntry(pIndex, entry, index) {
    Abc_Obj_t *pObj = Abc_NtkObj(pNtk, entry);
    Abc_NodeSetTravIdCurrent(pObj);
  }
  Abc_NtkForEachObj(pNtk, pObj, index) {
    if (Abc_ObjId(pObj) == 0)
      continue;
    if (Abc_NodeIsTravIdCurrent(pObj)) {
      Vec_IntPush(vExists, Abc_ObjId(pObj));
    } else {
      Vec_IntPush(vForalls, Abc_ObjId(pObj));
    }
  }
  writeQdimacsFile(pNtk, pFileName, 0, vForalls, vExists);
  Vec_IntFree(vExists);
  Vec_IntFree(vForalls);
}

static void collectTFORecur(Abc_Ntk_t *pNtk, Vec_Int_t *pScope,
                            Abc_Obj_t *pObj) {
  int v;
  Abc_Obj_t *pFanout;
  Abc_ObjForEachFanout(pObj, pFanout, v) {
    if (Abc_NodeIsTravIdCurrent(pFanout)) {
      continue;
    }
    collectTFORecur(pNtk, pScope, pFanout);
  }
  Abc_NodeSetTravIdCurrent(pObj);
  if (Abc_ObjType(pObj) != ABC_OBJ_PI) {
    Vec_IntPush(pScope, Abc_ObjId(pObj));
  }
}

void writeQdimacsFile(Abc_Ntk_t *pNtk, char *name, int numPo,
                      Vec_Int_t *vForalls, Vec_Int_t *vExists) {
  FILE *out;
  int numVar, numCla;
  int entry, index;

  out = fopen(name, "w");
  if (!out) {
    Abc_Print(-1, "Cannot open file %s\n", name);
    return;
  }
  numVar = Abc_NtkObjNumMax(pNtk) - 1;
  numCla = 3 * Abc_NtkNodeNum(pNtk) + 2 + 1;
  fprintf(out, "c qdimacs file for boolean function synthesis\n");
  fprintf(out, "p cnf %d %d\n", numVar, numCla);
  fprintf(out, "a ");
  Vec_IntForEachEntry(vForalls, entry, index) { fprintf(out, "%d ", entry); }
  fprintf(out, "0\n");
  fprintf(out, "e ");
  Vec_IntForEachEntry(vExists, entry, index) { fprintf(out, "%d ", entry); }
  fprintf(out, "0\n");

  writeCnfClause(out, pNtk, numPo);
  fclose(out);
  // Abc_Print(-2, "File %s is written.\n", name);
}

static void writeCnfClause(FILE *out, Abc_Ntk_t *pNtk, int numPo) {
  Abc_Obj_t *pObj, *pFanin0, *pFanin1;
  int i;

  Abc_NtkForEachNode(pNtk, pObj, i) {
    pFanin0 = Abc_ObjFanin0(pObj);
    pFanin1 = Abc_ObjFanin1(pObj);
    if (!Abc_ObjFaninC0(pObj) && !Abc_ObjFaninC1(pObj)) {
      fprintf(out, "%d -%d -%d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0),
              Abc_ObjId(pFanin1));
      fprintf(out, "-%d %d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0));
      fprintf(out, "-%d %d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin1));
    }
    if (Abc_ObjFaninC0(pObj) && !Abc_ObjFaninC1(pObj)) {
      fprintf(out, "%d %d -%d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0),
              Abc_ObjId(pFanin1));
      fprintf(out, "-%d -%d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0));
      fprintf(out, "-%d %d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin1));
    }
    if (!Abc_ObjFaninC0(pObj) && Abc_ObjFaninC1(pObj)) {
      fprintf(out, "%d -%d %d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0),
              Abc_ObjId(pFanin1));
      fprintf(out, "-%d %d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0));
      fprintf(out, "-%d -%d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin1));
    }
    if (Abc_ObjFaninC0(pObj) && Abc_ObjFaninC1(pObj)) {
      fprintf(out, "%d %d %d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0),
              Abc_ObjId(pFanin1));
      fprintf(out, "-%d -%d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0));
      fprintf(out, "-%d -%d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin1));
    }
  }

  pObj = Abc_NtkPo(pNtk, numPo);
  pFanin0 = Abc_ObjFanin0(pObj);
  if (!Abc_ObjFaninC0(pObj)) {
    fprintf(out, "%d -%d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0));
    fprintf(out, "-%d %d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0));
  } else {
    fprintf(out, "%d %d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0));
    fprintf(out, "-%d -%d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0));
  }
  fprintf(out, "%d 0\n", Abc_ObjId(pObj));
}
