#include "Common.h"

#define SERVERPORT 9000
#define BUFSIZE    512

// Ŭ���̾�Ʈ ���� ����ü
typedef struct {
	SOCKET client_sock;
	struct sockaddr_in clientaddr;
} CLIENT_INFO;

// Ŭ���̾�Ʈ�� ������ ���
DWORD WINAPI ProcessClient(LPVOID arg)
{
	int retval;
	CLIENT_INFO* client_info = (CLIENT_INFO*)arg;
	SOCKET client_sock = client_info->client_sock;
	struct sockaddr_in clientaddr = client_info->clientaddr;
	char addr[INET_ADDRSTRLEN];
	char buf[BUFSIZE + 1];

	// Ŭ���̾�Ʈ IP �ּҿ� ��Ʈ ��ȣ�� ���ڿ��� ��ȯ
	inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));

	while (1) {
		// ������ �ޱ�
		retval = recv(client_sock, buf, BUFSIZE, 0);
		if (retval == SOCKET_ERROR) {
			err_display("recv()");
			break;
		}
		else if (retval == 0)
			break;

		// ���� ������ ���
		buf[retval] = '\0';
		printf("[TCP/%s:%d] %s\n", addr, ntohs(clientaddr.sin_port), buf);

		// ������ ������
		retval = send(client_sock, buf, retval, 0);
		if (retval == SOCKET_ERROR) {
			err_display("send()");
			break;
		}
	}

	// ���� �ݱ�
	closesocket(client_sock);
	printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
		addr, ntohs(clientaddr.sin_port));

	// �޸� ����
	free(client_info);

	return 0;
}

int main(int argc, char* argv[])
{
	int retval;

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// ���� ����
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	// ������ ��ſ� ����� ����
	SOCKET client_sock;
	struct sockaddr_in clientaddr;
	int addrlen;
	HANDLE hThread;

	while (1) {
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}

		// Ŭ���̾�Ʈ ���� ���
		char addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
		printf("\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
			addr, ntohs(clientaddr.sin_port));

		// Ŭ���̾�Ʈ ������ ������ �Լ��� �����ϱ� ���� ����ü ���� �� �ʱ�ȭ
		CLIENT_INFO* client_info = (CLIENT_INFO*)malloc(sizeof(CLIENT_INFO));
		if (client_info == NULL) {
			fprintf(stderr, "�޸� �Ҵ� ����\n");
			closesocket(client_sock);
			continue;
		}
		client_info->client_sock = client_sock;
		client_info->clientaddr = clientaddr;

		// ������ ����
		hThread = CreateThread(NULL, 0, ProcessClient,
			(LPVOID)client_info, 0, NULL);
		if (hThread == NULL) {
			closesocket(client_sock);
			free(client_info);
		}
		else {
			CloseHandle(hThread);
		}
	}

	// ���� �ݱ�
	closesocket(listen_sock);

	// ���� ����
	WSACleanup();
	return 0;
}
