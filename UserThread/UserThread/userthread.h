#ifndef USR_THD_H
#define USR_THD_H

#include <Windows.h>

#define MAX_THD_NUM		16

// stack size of each user-thread.
// NOTE: stack size must be large enough to avoid memory exception.
#define STACK_SIZE		1024 << 4

typedef unsigned long u32;

typedef void(*UserThreadProc)(void* param);

typedef struct {
	u32 edi;
	u32 esi;
	u32 ebp;
	u32 dummy_esp;
	u32 ebx;
	u32 edx;
	u32 ecx;
	u32 eax;
} REGULAR_REGS, *PREGULAR_REGS;

typedef struct {
	REGULAR_REGS regs;
	u32 esp;
	u32 eip;
} U_CONTEXT, *PU_CONTEXT;

typedef struct {
	U_CONTEXT context;
	void* param;
	u32 hang_up;
} THD_CONTROL_BLOCK, *PTHD_CONTROL_BLOCK;


void* create_thread(UserThreadProc thread_start_address, void* param);

void yield_thread();

void start_threads();

void clean_threads();


#endif // USR_THD_H
