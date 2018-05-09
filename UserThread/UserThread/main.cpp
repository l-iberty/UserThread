#include <stdio.h>
#include "userthread.h"

//#define ENDLESS_LOOP


void T1(void* param)
{
#ifndef ENDLESS_LOOP
	for (int i = 0; i < 12; i++)
	{
		printf("T1 : %d, %c\n", i, *(char*)param);
		yield_thread();
	}
	printf("T1 ends\n");
#else
	int i = 0;
	while (1)
	{
		Sleep(100);
		printf("T1 : %d, %c\n", i++, *(char*)param);
		yield_thread();
	}
#endif
}

void T2(void* param)
{
#ifndef ENDLESS_LOOP
	for (int i = 10; i > 0; i--)
	{
		printf("  T2 : %d, %c\n", i, *(char*)param);
		yield_thread();
	}
	printf("T2 ends\n");
#else
	int i = 100;
	while (1)
	{
		Sleep(100);
		printf("  T2 : %d, %c\n", i++, *(char*)param);
		yield_thread();
	}
#endif
}

void T3(void* param)
{
#ifndef ENDLESS_LOOP
	for (int i = 0; i < 15; i++)
	{
		printf("    T3 : %d, %c\n", i, *(char*)param);
		yield_thread();
	}
	printf("T3 ends\n");
#else
	int i = 200;
	while (1)
	{
		Sleep(100);
		printf("    T3 : %d, %c\n", i++, *(char*)param);
		yield_thread();
	}
#endif
}

void T4(void* param)
{
#ifndef ENDLESS_LOOP
	for (int i = 20; i > 0; i--)
	{
		printf("      T4 : %d, %c\n", i, *(char*)param);
		yield_thread();
	}
	printf("T4 ends\n");
#else
	int i = 300;
	while (1)
	{
		Sleep(100);
		printf("      T4 : %d, %c\n", i++, *(char*)param);
		yield_thread();
	}
#endif
}

int main()
{
	char a = 'A';
	char b = 'B';
	char c = 'C';
	char d = 'D';

	create_thread(T1, (void*)&a);
	create_thread(T2, (void*)&b);
	create_thread(T3, (void*)&c);
	create_thread(T4, (void*)&d);

	start_threads();

	clean_threads();
}