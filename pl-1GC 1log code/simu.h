
#ifndef DWORD                   
#define _CRT_SECURE_NO_DEPRECATE
#define __int64 long long       //不加 __int64 會報錯

#include "stdio.h"
#include "stdlib.h"
#include "assert.h"
#include "math.h"
#include "time.h"
// #include "conio.h"
#include "memory.h"
#include "math.h"
#include "string.h"
#include <errno.h>

typedef unsigned long	DWORD;	//4B, 通常用來放 addr 或 ptr
typedef unsigned short	WORD;	//2B
typedef unsigned char	BYTE;	//1B
typedef __int64			I64;

#define TRUE			1
#define FALSE			0
#endif


#define PAU_FREE		0xffffffff
#define PAU_DEAD		0xfffffffe
#define LAU_NULL		0xffffffff
#define	Lgranularity	1024
#define	Sgranularity	8
#define SDRAMsize		65536	//	how many KB does SDRAM have