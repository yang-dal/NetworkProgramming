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

// 1�ʿ� �ѹ� ����ϴ� ������
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
	// �Ӱ� ���� �ʱ�ȭ
	InitializeCriticalSection(&cs);

	// ������ �� �� ����
	HANDLE hThread[3];
	hThread[0] = CreateThread(NULL, 0, MyThread1, NULL, 0, NULL);
	hThread[1] = CreateThread(NULL, 0, MyThread2, NULL, 0, NULL);
	hThread[2] = CreateThread(NULL, 0, MyThread3, NULL, 0, NULL);

	// ������ �� �� ���� ���
	WaitForMultipleObjects(3, hThread, TRUE, INFINITE);

	// �Ӱ� ���� ����
	DeleteCriticalSection(&cs);

	// ��� ���
	printf("count = %d\n", count);
	return 0;
}