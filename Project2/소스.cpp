#include "Common.h"

int main(int argc, char* argv[])
{
	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;
	printf("[�˸�] ���� �ʱ�ȭ ����\n");
	
	printf("Suggested version: %d.%d\n", LOBYTE(wsa.wVersion), HIBYTE(wsa.wVersion));
	printf("High supported version: %d.%d\n", LOBYTE(wsa.wHighVersion), HIBYTE(wsa.wHighVersion));
	
	printf("szDescription:%s\n", wsa.szDescription); //���� ����
	printf("szSystemStatus:%s\n", wsa.szSystemStatus); //���� ����


	// ���� ����
	WSACleanup();
	return 0;
}