#include "Common.h"

#define SERVERPORT 9000
#define BUFSIZE    512

// 소켓 정보 저장을 위한 구조체와 변수
struct SOCKETINFO
{
	SOCKET sock;
	char buf[BUFSIZE + 1];
	int recvbytes;
	int sendbytes;
};
int nTotalSockets = 0;
SOCKETINFO* SocketInfoArray[FD_SETSIZE];

// 소켓 정보 관리 함수
bool AddSocketInfo(SOCKET sock);
void RemoveSocketInfo(int nIndex);

int main(int argc, char* argv[])
{
	int retval;

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// 소켓 생성
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

	// 넌블로킹 소켓으로 전환
	u_long on = 1;
	retval = ioctlsocket(listen_sock, FIONBIO, &on);
	if (retval == SOCKET_ERROR) err_display("ioctlsocket()");

	// 데이터 통신에 사용할 변수
	fd_set rset, wset;
	int nready;
	SOCKET client_sock;
	struct sockaddr_in clientaddr;
	int addrlen;

	while (1) {
		// 소켓 셋 초기화
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		FD_SET(listen_sock, &rset);
		for (int i = 0; i < nTotalSockets; i++) {
			// 받은 데이터가 보낸 데이터보다 많으면 (에코 서버 특성 상 데이터를 보내야하기 때문에)
			// 쓰기 셋에 소켓 추가
			if (SocketInfoArray[i]->recvbytes > SocketInfoArray[i]->sendbytes)
				FD_SET(SocketInfoArray[i]->sock, &wset);
			// 받은 데이터가 보낸 데이터와 같거나 작으면, 읽기 셋에 소켓 추가
			else
				FD_SET(SocketInfoArray[i]->sock, &rset);
		}

		// select()
		nready = select(0, &rset, &wset, NULL, NULL); // 소켓 이벤트를 기다림 (타임아웃 null이기 때문에 무한 대기)
		if (nready == SOCKET_ERROR) err_quit("select()");

		// 소켓 셋 검사(1): 클라이언트 접속 수용
		if (FD_ISSET(listen_sock, &rset)) { // 읽기 셋에 연결 대기 소켓이 있음 -> 접속한 클라이언트가 있다는 뜻
			addrlen = sizeof(clientaddr);
			client_sock = accept(listen_sock, // 클라이언트 소켓도 넌블로킹 소켓으로 만들어짐
				(struct sockaddr*)&clientaddr, &addrlen);
			if (client_sock == INVALID_SOCKET) {
				err_display("accept()");
				break;
			}
			else {
				// 클라이언트 정보 출력
				char addr[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
				printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",
					addr, ntohs(clientaddr.sin_port));
				// 소켓 정보 추가: 실패 시 소켓 닫음
				if (!AddSocketInfo(client_sock)) // 추후 소켓에 대해 읽고 쓰기를 하기 위해 소켓을 별도로 저장
					closesocket(client_sock);
			}
			if (--nready <= 0) // 읽기나 쓰기 작업이 필요한 소켓이 없음을 의미
				continue;
		}

		// 소켓 셋 검사(2): 데이터 통신
		for (int i = 0; i < nTotalSockets; i++) { // 현재 작업이 필요한 모든 소켓에 대해서
			SOCKETINFO* ptr = SocketInfoArray[i];
			if (FD_ISSET(ptr->sock, &rset)) { // 읽기 셋에 있는 소켓 처리
				// 데이터 받기
				retval = recv(ptr->sock, ptr->buf, BUFSIZE, 0);
				if (retval == SOCKET_ERROR) {
					err_display("recv()");
					RemoveSocketInfo(i); // 에러 발생한 소켓에 대해서 제거
				}
				else if (retval == 0) {
					RemoveSocketInfo(i); // 연결이 종료된 소켓에 대해서 제거
				}
				else {
					ptr->recvbytes = retval; // 받은 데이터 크기 초기화
					// 클라이언트 정보 얻기
					addrlen = sizeof(clientaddr);
					getpeername(ptr->sock, (struct sockaddr*)&clientaddr, &addrlen);
					// 받은 데이터 출력
					ptr->buf[ptr->recvbytes] = '\0'; // 받은 데이터 끝에 null 추가
					char addr[INET_ADDRSTRLEN];
					inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
					printf("[TCP/%s:%d] %s\n", addr,
						ntohs(clientaddr.sin_port), ptr->buf);
				}
			}
			else if (FD_ISSET(ptr->sock, &wset)) { // 쓰기 셋에 있는 소켓 처리
				// 데이터 보내기
				retval = send(ptr->sock, ptr->buf + ptr->sendbytes, // 버퍼 첫 위치에서 보낸 만큼 이후의 데이터를 send 함수로 보냄
					ptr->recvbytes - ptr->sendbytes, 0);
				if (retval == SOCKET_ERROR) {
					err_display("send()");
					RemoveSocketInfo(i); // 에러 발생한 소켓에 대해서 제거
				}
				else {
					ptr->sendbytes += retval; // 보낸 데이터만큼 sendbytes를 업데이트
					if (ptr->recvbytes == ptr->sendbytes) { // 받은 데이터만큼 보냈으면
						ptr->recvbytes = ptr->sendbytes = 0; // 0으로 초기화
					}
				}
			}
		} /* end of for */
	} /* end of while (1) */

	// 소켓 닫기
	closesocket(listen_sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}

// 소켓 정보 추가
bool AddSocketInfo(SOCKET sock)
{
	if (nTotalSockets >= FD_SETSIZE) {
		printf("[오류] 소켓 정보를 추가할 수 없습니다!\n");
		return false;
	}
	SOCKETINFO* ptr = new SOCKETINFO;
	if (ptr == NULL) {
		printf("[오류] 메모리가 부족합니다!\n");
		return false;
	}
	ptr->sock = sock;
	ptr->recvbytes = 0;
	ptr->sendbytes = 0;
	SocketInfoArray[nTotalSockets++] = ptr; // 마지막에 추가
	return true;
}

// 소켓 정보 삭제
void RemoveSocketInfo(int nIndex)
{
	SOCKETINFO* ptr = SocketInfoArray[nIndex];

	// 클라이언트 정보 얻기
	struct sockaddr_in clientaddr;
	int addrlen = sizeof(clientaddr);
	getpeername(ptr->sock, (struct sockaddr*)&clientaddr, &addrlen);

	// 클라이언트 정보 출력
	char addr[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
	printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n",
		addr, ntohs(clientaddr.sin_port));

	// 소켓 닫기
	closesocket(ptr->sock);
	delete ptr;

	// 지워지는 소켓 정보가 마지막이 아니라면
	if (nIndex != (nTotalSockets - 1))
		// 지워지는 위치에 마지막 소켓 정보를 가져옴
		SocketInfoArray[nIndex] = SocketInfoArray[nTotalSockets - 1];
	--nTotalSockets;
}