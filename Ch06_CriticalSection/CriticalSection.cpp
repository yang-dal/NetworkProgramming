#include <windows.h>
#include <stdio.h>

#define MAXCNT 100000000
int count = 0;
CRITICAL_SECTION cs;

DWORD WINAPI MyThread1(LPVOID arg)
{
	for (int i = 0; i < MAXCNT; i++) {
		EnterCriticalSection(&cs);
		count += 2;
		LeaveCriticalSection(&cs);
	}
	return 0;
}

DWORD WINAPI MyThread2(LPVOID arg)
{
	for (int i = 0; i < MAXCNT; i++) {
		EnterCriticalSection(&cs);
		count -= 2;
		LeaveCriticalSection(&cs);
	}
	return 0;
}

// 1초에 한번 출력하는 스레드
DWORD WINAPI MyThread3(LPVOID arg)
{
	while (1) {
		EnterCriticalSection(&cs);
		printf("%d\n", count);
		Sleep(1000);
		LeaveCriticalSection(&cs);
	}
}

int main(int argc, char* argv[])
{
	// 임계 영역 초기화
	InitializeCriticalSection(&cs);

	// 스레드 세 개 생성
	HANDLE hThread[3];
	hThread[0] = CreateThread(NULL, 0, MyThread1, NULL, 0, NULL);
	hThread[1] = CreateThread(NULL, 0, MyThread2, NULL, 0, NULL);
	hThread[2] = CreateThread(NULL, 0, MyThread3, NULL, 0, NULL);

	// 스레드 세 개 종료 대기
	WaitForMultipleObjects(3, hThread, TRUE, INFINITE);

	// 임계 영역 삭제
	DeleteCriticalSection(&cs);

	// 결과 출력
	printf("count = %d\n", count);
	return 0;
}