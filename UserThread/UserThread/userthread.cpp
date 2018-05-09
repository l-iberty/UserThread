#include "userthread.h"
#include <stddef.h>


int nr_threads = 0; // how many threads are created?
int thread_idx = 0; // pointer to thread which will be scheduled

PTHD_CONTROL_BLOCK p_current_thread = NULL;

U_CONTEXT main_context; // context of `main`

// varibles used by asm to access the members of struct
u32 context_size = sizeof(U_CONTEXT);
u32 dummy_esp_offet = offsetof(REGULAR_REGS, dummy_esp);
u32 esp_offset = offsetof(U_CONTEXT, esp);
u32 eip_offset = offsetof(U_CONTEXT, eip);
u32 param_offset = offsetof(THD_CONTROL_BLOCK, param);
u32 hang_up_offset = offsetof(THD_CONTROL_BLOCK, hang_up);

THD_CONTROL_BLOCK threads[MAX_THD_NUM]; // ready queue of user-threads
void* p_thread_stack[MAX_THD_NUM]; // mark the address of allocated stack for each user-thread

//////////////////////////////////////////////////////////////


// save context of current user-thread
__declspec(naked) void save_context()
{
	__asm
	{
		// save ESP
		mov eax, p_current_thread;
		mov edx, esp_offset;
		add eax, edx; // EAX points to p_current_thread->context.esp

		//      call yield_thread             call save_context
		// ESP -------------------> ESP -= 4 -------------------> ESP -= 4
		// original ESP = current ESP + 8
		mov esi, esp;
		add esi, 8
		mov [eax], esi; // p_current_thread->regs.esp = original ESP

		// save EIP
		mov ecx, [esp + 4]; // ECX <- return address of caller `yield_thread`
		mov eax, p_current_thread;
		mov edx, eip_offset;
		mov [eax + edx], ecx; // p_current_thread->context.eip = return address of `yield_thread`

		// save other registers
		mov edx, esp_offset;
		add eax, edx;
		mov esp, eax;
		pushad;

		// restore ESP and return
		mov eax, [eax];
		sub eax, 8;
		mov esp, eax; // ESP points to return address of `save_context`
		ret;
	}
}

// restore context of current user-thread
__declspec(naked) void restore_context()
{
	__asm
	{
		// save ESP
		mov eax, p_current_thread;
		mov edx, dummy_esp_offet;
		mov [eax + edx], esp;

		// switch ESP
		mov eax, p_current_thread;
		mov esp, eax;

		popad;
		
		// retore ESP
		mov eax, p_current_thread;
		mov edx, dummy_esp_offet;
		mov eax, [eax + edx];
		mov esp, eax;

		ret;
	}
}

// schedule user-threads in the ready queue
void schedule()
{
	thread_idx = (thread_idx + 1) % nr_threads;
	p_current_thread = threads + thread_idx;
}

__declspec(naked) u32 get_thread_param()
{
	__asm
	{
		mov eax, p_current_thread;
		mov edx, param_offset;
		add eax, edx;
		mov eax, [eax];
		ret;
	}
}

__declspec(naked) u32 get_thread_breakpoint()
{
	__asm
	{
		mov eax, p_current_thread;
		mov edx, eip_offset;
		add eax, edx;
		mov eax, [eax];
		ret;
	}
}


__declspec(naked) void set_hang_up()
{
	__asm
	{
		mov eax, p_current_thread;
		mov edx, hang_up_offset;
		mov [eax + edx], 1;
		ret;
	}
}

__declspec(naked) u32 get_hang_up()
{
	__asm
	{
		mov eax, p_current_thread;
		mov edx, hang_up_offset;
		mov eax, [eax + edx];
		ret;
	}
}

// save context of `main`.
// NOTE: this function must be called at beginning of `start_threads`
__declspec(naked) void save_main_context()
{
	__asm
	{
		lea eax, main_context;
		mov edx, context_size;
		add eax, edx;

		mov edx, [esp + 4]; // EDX <- return address of `start_threads`
		mov ecx, esp;
		add ecx, 8 // EDI <- ESP before calling `start_threads`

		mov esp, eax;
		
		push edx;
		push ecx;
		pushad;

		sub ecx, 8; // ECX <- current ESP
		mov esp, ecx;
		ret;
	}
}

// restore context of `main`.
// NOTE: this function is called when one of user-thread in the ready queue ends
//       in order to return to `main` correctly.
__declspec(naked) void restore_main_context()
{
	__asm
	{
		// restore regular registers
		lea eax, main_context;
		mov esp, eax;
		popad;

		// restore ESP
		lea eax, main_context;
		mov edx, esp_offset;
		mov edx, [eax + edx];
		mov esp, edx;

		mov edx, eip_offset;
		mov eax, [eax + edx];
		jmp eax;
	}
}

//////////////////////////////////////////////////////////////


void* create_thread(UserThreadProc thread_start_address, void* param)
{
	if (nr_threads >= MAX_THD_NUM)
		return NULL;

	// Allocate memory for stack used by usr-thread
	char* p_stack = (char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, STACK_SIZE);
	if (p_stack == NULL)
		return NULL;
	
	p_thread_stack[nr_threads] = p_stack;
	p_stack += STACK_SIZE;

	// Initialize user-thread control block
	ZeroMemory(threads + nr_threads, sizeof(threads[nr_threads]));

	threads[nr_threads].context.esp = (u32)p_stack; // ESP
	threads[nr_threads].context.eip = (u32)thread_start_address; // EIP

	threads[nr_threads].param = param;

	p_current_thread = threads;

	nr_threads++;

	return thread_start_address;
}

__declspec(naked) void yield_thread()
{
	__asm
	{
		// save context of current thread
		call save_context;

		// hang up current thread
		call set_hang_up;

		// switch to next thread and push its parameter
		call schedule;
		mov eax, p_current_thread;
		mov edx, esp_offset;
		mov esp, [eax + edx];

		call get_hang_up;
		test eax, eax;
		jnz LABLE_RESTART;

		// thread hasn't been started, we need to set its
		// paramter and return address.

		call get_thread_param;
		push eax;

		// push return address of next thread
		mov eax, restore_main_context;
		push eax;

		LABLE_RESTART:
		// restore context of next thread and execute it
		call restore_context;

		call get_thread_breakpoint;
		jmp eax;
	}
}

__declspec(naked) void start_threads()
{
	__asm
	{
		call save_main_context;

		// initialize stack of user-thread
		mov eax, p_current_thread;
		mov edx, esp_offset;
		mov esp, [eax + edx];

		// param of user-thread
		call get_thread_param;
		push eax;
		
		// return address of user-thread
		mov eax, restore_main_context;
		push eax;

		// start address of user-thread
		call get_thread_breakpoint;
		jmp eax;
	}
}

void clean_threads()
{
	for (int i = 0; i < MAX_THD_NUM; i++)
	{
		if (p_thread_stack[i])
			HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, p_thread_stack[i]);
	}
}
