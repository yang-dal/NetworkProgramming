#include "Common.h"

int main(int argc, char* argv[])
{
	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;
	printf("[알림] 윈속 초기화 성공\n");
	
	printf("Suggested version: %d.%d\n", LOBYTE(wsa.wVersion), HIBYTE(wsa.wVersion));
	printf("High supported version: %d.%d\n", LOBYTE(wsa.wHighVersion), HIBYTE(wsa.wHighVersion));
	
	printf("szDescription:%s\n", wsa.szDescription); //윈속 설명
	printf("szSystemStatus:%s\n", wsa.szSystemStatus); //윈속 상태


	// 윈속 종료
	WSACleanup();
	return 0;
}