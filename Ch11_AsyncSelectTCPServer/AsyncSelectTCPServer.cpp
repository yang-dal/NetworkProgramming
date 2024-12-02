#include "..\..\Common.h"

#define SERVERPORT 9000
#define BUFSIZE    512
#define WM_SOCKET  (WM_USER+1)

// ���� ���� ������ ���� ����ü�� ����
struct SOCKETINFO
{
	SOCKET sock;
	char buf[BUFSIZE + 1];
	int recvbytes;
	int sendbytes;
	bool recvdelayed;
	SOCKETINFO* next;
};
SOCKETINFO* SocketInfoList;

// ������ �޽��� ó�� �Լ�
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void ProcessSocketMessage(HWND, UINT, WPARAM, LPARAM);
// ���� ���� ���� �Լ�
bool AddSocketInfo(SOCKET sock);
SOCKETINFO* GetSocketInfo(SOCKET sock);
void RemoveSocketInfo(SOCKET sock);

int main(int argc, char* argv[])
{
	int retval;

	// ������ Ŭ���� ���
	WNDCLASS wndclass;
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = NULL;
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = _T("MyWndClass");
	if (!RegisterClass(&wndclass)) return 1;

	// ������ ����
	HWND hWnd = CreateWindow(_T("MyWndClass"), _T("TCP ����"),
		WS_OVERLAPPEDWINDOW, 0, 0, 600, 200, NULL, NULL, NULL, NULL);
	if (hWnd == NULL) return 1;
	ShowWindow(hWnd, SW_SHOWNORMAL);
	UpdateWindow(hWnd);

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

	// WSAAsyncSelect()
	// ���� ��� ���� (listen_sock)�� ���� �̺�Ʈ(FD_ACCEPT or FD_CLOSE)��
	// ������ �޽��� ����(WM_SOCKET)���� �ް�� ���
	retval = WSAAsyncSelect(listen_sock, hWnd, WM_SOCKET, FD_ACCEPT | FD_CLOSE);
	if (retval == SOCKET_ERROR) err_quit("WSAAsyncSelect()");

	// �޽��� ����
	MSG msg;
	while (GetMessage(&msg, 0, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// ���� ����
	WSACleanup();
	return msg.wParam;
}

// ������ �޽��� ó��
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_SOCKET: // ���� ���� ������ �޽���
		ProcessSocketMessage(hWnd, uMsg, wParam, lParam);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// ���� ���� �޽��� ó��
void ProcessSocketMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// ������ ��ſ� ����� ����
	int retval;
	SOCKET client_sock;
	struct sockaddr_in clientaddr;
	int addrlen;
	SOCKETINFO* ptr;

	// ���� �߻� ���� Ȯ��
	if (WSAGETSELECTERROR(lParam)) {
		err_display(WSAGETSELECTERROR(lParam));
		RemoveSocketInfo(wParam);
		return;
	}

	// �޽��� ó��
	switch (WSAGETSELECTEVENT(lParam)) {
	case FD_ACCEPT:
		addrlen = sizeof(clientaddr);
		client_sock = accept(wParam, (struct sockaddr*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			return;
		}
		printf("\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
			inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
		AddSocketInfo(client_sock);

		// Ŭ���̾�Ʈ ����(client_sock)�� ���� �̺�Ʈ(FD_READ or FD_WRITE)��
		// ������ �޽��� ����(WM_SOCKET)���� �޵��� ���
		retval = WSAAsyncSelect(client_sock, hWnd,
			WM_SOCKET, FD_READ | FD_WRITE | FD_CLOSE);
		if (retval == SOCKET_ERROR) {
			err_display("WSAAsyncSelect()");
			RemoveSocketInfo(client_sock);
		}
		break;
	case FD_READ:
		// ������ ����� ���� ������ ����
		ptr = GetSocketInfo(wParam);

		// ������ �޾����� ���� �����͸� ������ ���� ��� ��ٷ� �����ϰ�
		// ���� ���� �Ŀ� �ٽ� FD_READ �̺�Ʈ�� �߻��ϵ��� recvdelayed flag ����
		if (ptr->recvbytes > 0) {
			ptr->recvdelayed = true;
			return;
		}
		// ������ �ޱ�
		retval = recv(ptr->sock, ptr->buf, BUFSIZE, 0);
		if (retval == SOCKET_ERROR) {
			err_display("recv()");
			RemoveSocketInfo(wParam);
			return;
		}
		ptr->recvbytes = retval;
		// ���� ������ ���
		ptr->buf[retval] = '\0';
		addrlen = sizeof(clientaddr);
		getpeername(wParam, (struct sockaddr*)&clientaddr, &addrlen);
		printf("[TCP/%s:%d] %s\n", inet_ntoa(clientaddr.sin_addr),
			ntohs(clientaddr.sin_port), ptr->buf);
	
	case FD_WRITE:
		// ������ ����� ���� ������ ����
		ptr = GetSocketInfo(wParam);

		// ���� �����Ͱ� �������� ����
		if (ptr->recvbytes <= ptr->sendbytes)
			return;
		// ������ ������
		retval = send(ptr->sock, ptr->buf + ptr->sendbytes,
			ptr->recvbytes - ptr->sendbytes, 0);
		if (retval == SOCKET_ERROR) {
			err_display("send()");
			RemoveSocketInfo(wParam);
			return;
		}
		// ���� ������ ũ�� ����
		ptr->sendbytes += retval;
		// ���� �����͸� ��� ���´��� üũ
		if (ptr->recvbytes == ptr->sendbytes) {
			ptr->recvbytes = ptr->sendbytes = 0;
			// ������ FD_READ�� �߻����Ѽ�
			// ������ ���������� ó������ ���� �����͸� ����
			if (ptr->recvdelayed) {
				ptr->recvdelayed = false;
				PostMessage(hWnd, WM_SOCKET, wParam, FD_READ);
			}
		}
		break;
	case FD_CLOSE:
		RemoveSocketInfo(wParam);
		break;
	}
}

// ���� ���� �߰�
bool AddSocketInfo(SOCKET sock)
{
	SOCKETINFO* ptr = new SOCKETINFO;
	if (ptr == NULL) {
		printf("[����] �޸𸮰� �����մϴ�!\n");
		return false;
	}
	// ���� ���� ����Ʈ�� ���� �߰�
	ptr->sock = sock;
	ptr->recvbytes = 0;
	ptr->sendbytes = 0;
	ptr->recvdelayed = false;
	ptr->next = SocketInfoList;
	SocketInfoList = ptr;
	return true;
}

// ���� ���� ���
SOCKETINFO* GetSocketInfo(SOCKET sock)
{
	SOCKETINFO* ptr = SocketInfoList;
	while (ptr) {
		if (ptr->sock == sock)
			return ptr;
		ptr = ptr->next;
	}
	return NULL;
}

// ���� ���� ����
void RemoveSocketInfo(SOCKET sock)
{
	// Ŭ���̾�Ʈ ���� ���
	struct sockaddr_in clientaddr;
	int addrlen = sizeof(clientaddr);
	getpeername(sock, (struct sockaddr*)&clientaddr, &addrlen);
	printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
		inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

	// ���� ���� ����Ʈ���� ���� ����
	SOCKETINFO* curr = SocketInfoList;
	SOCKETINFO* prev = NULL;
	while (curr) {
		if (curr->sock == sock) {
			if (prev)
				prev->next = curr->next;
			else
				SocketInfoList = curr->next;
			closesocket(curr->sock); // ���� �ݱ�
			delete curr;
			return;
		}
		prev = curr;
		curr = curr->next;
	}
}