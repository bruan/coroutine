#include <stdio.h>

#include "coroutine.h"
#include <thread>

void fun1(uint64_t nContext)
{
	printf("AAAAA111\n");
	coroutine::yield();
	printf("BBBBB111\n");
}

void fun2(uint64_t nContext)
{
	printf("AAAAA222\n");
	coroutine::yield();
	printf("BBBBB222\n");
}

void f()
{
	coroutine::init(1024 * 1024);
	char nDummy;
	printf("thread 0x%x\n", (uintptr_t)&nDummy);
	uint64_t nCoroutineID1 = coroutine::create(10 * 4096, std::bind(&fun1, std::placeholders::_1));
	uint64_t nCoroutineID2 = coroutine::create(0, std::bind(&fun2, std::placeholders::_1));
	printf("CCCCCC222\n");
	coroutine::resume(nCoroutineID1, 0);
	coroutine::resume(nCoroutineID2, 0);
	coroutine::resume(nCoroutineID2, 0);
	printf("DDDDD222\n");
	coroutine::resume(nCoroutineID1, 0);
	uint64_t nCoroutineID3 = coroutine::create(1024*1024, std::bind(&fun1, std::placeholders::_1));
	printf("CCCCCC333\n");
	coroutine::resume(nCoroutineID3, 0);
	coroutine::resume(nCoroutineID3, 0);
	printf("EEEEEE333\n");
}

int main()
{
	std::thread t0(f);
 	std::thread t1(f);
 	std::thread t2(f);
	t0.join();
 	t1.join();
 	t2.join();

#ifdef _WIN32
	system("pause");
#endif

	return 0;
}