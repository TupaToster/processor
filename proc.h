#ifndef PROC_H
#define PROC_H

typedef int    arg_t;
typedef int    cmd_t;
typedef arg_t Elem_t;


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include "stack.h"


const int VERSION = 9;
const int SIGNATURE = 0x54ABC228;

const size_t BUFLEN = 128;

const size_t MAX_NUM_OF_ARGS = 2;
const size_t  ARG_SIZE = sizeof (arg_t);
const size_t  CMD_SIZE = sizeof (cmd_t);
const size_t INFO_SIZE = sizeof (SIGNATURE) + sizeof (VERSION) + sizeof (int);

const int CODE_SHIFT = 3;

const size_t ERROR_MSG_SIZE = 100;
FILE *ERROR_STREAM = stdout;

const int CMD_MASK = 0x000000FF;

const size_t NUM_OF_REGS = 5;
const size_t RAM_SIZE = 1024;


struct Text
{
    size_t    len;
    size_t buflen;
    char  *buffer; 
    char  **lines;
};

struct Cpu_t
{
    int ip;
    cmd_t *code;
    Stack_t stk;
    size_t code_size;

    arg_t regs [NUM_OF_REGS];
    arg_t ram  [RAM_SIZE];
};

struct Label
{
    char *name;
    int ip;
};

enum REGISTERS
{
    RAX = 1, 
    RBX = 2, 
    RCX = 3,
    RDX = 4,
};

enum ERRORS
{
    OK                   =  0,
    NULLPTR_ARG          =  1,
    FOPEN_ERROR          =  2,
    ALLOC_ERROR          =  3,
    COMP_ERROR           =  4,
    FWRITE_ERROR         =  5,
    WRONG_SIGNATURE      =  6,
    WRONG_VERSION        =  7,
    WRONG_CODESIZE       =  8,
    DIV_BY_ZERO          =  9,
    EMPTY_STACK          = 10,
    UNKNOWN_CMD          = 11,
    INCORRECT_REG        = 12,
    INCORRECT_RAM_ADRESS = 13,
    INCORRECT_ARG_TYPE   = 14,
    INCORRECT_JMP_IP     = 15,
};


#define DEF_CMD(name, num, arg, ...) CMD_##name = num,

enum CMDS
{
    #include "cmd.h"
};

#undef DEF_CMD


enum ARG_TYPES
{
    ARG_IM  = 0x100,
    ARG_REG = 0x200,
    ARG_MEM = 0x400,
};

// asm funcs ----------------------------------------------------------------------------------------------------------

int ReadText (const char *input_file_name, struct Text *txt);

size_t GetSize (FILE *inp_file);

size_t CharReplace (char *str, char ch1, char ch2);

void FreeText (struct Text *txt);

int SetLines (struct Text *txt);

int Compile (struct Text *txt, cmd_t **cmds_p);

int WriteCmds (const char *output_file_name, cmd_t *cmds);

int PutArgs (char *args, cmd_t **cmd_ptr_p, size_t line);

char *DeleteSpaces (char *str);

void AsmErr (int err, FILE *stream);

//---------------------------------------------------------------------------------------------------------------------
// proc funcs ---------------------------------------------------------------------------------------------------------

int ReadCode (const char *input_file_name, Cpu_t *cpu);

int InfoCheck (Cpu_t *cpu);

int RunCode (Cpu_t *cpu);

int GetArgs (Cpu_t *cpu, cmd_t cmd, arg_t *arg);

void FreeCpu (Cpu_t *cpu);

int PrintArg (arg_t arg);

int ScanArg (arg_t *arg);

void CpuErr (Cpu_t *cpu, int err, FILE *stream);

void PrintCode (Cpu_t *cpu, FILE *stream);

void PrintRegs (Cpu_t *cpu, FILE *stream);
//---------------------------------------------------------------------------------------------------------------------

#endif