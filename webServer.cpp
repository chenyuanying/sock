#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <tchar.h>
#include <process.h>

#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "21194"
#define MAX_CLIENT_NUM 1024


sockaddr_in clientAddrList[MAX_CLIENT_NUM];
int currClientNum = 0;
SOCKET clientSocketList[MAX_CLIENT_NUM];
HANDLE modifyListMutex;

struct addrAndSocket{
	sockaddr_in addr;
	SOCKET sock;
};

void recvSend(void* param){
	int iResult = 0;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;
	memset(recvbuf, 0, recvbuflen);
	int iSendResult = 0;
	SOCKET ClientSocket = (*((addrAndSocket* )param)).sock;
	sockaddr_in addr = (*((addrAndSocket* )param)).addr;
	// Receive until the peer shuts down the connection
	do {
		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
	    if (iResult > 0) {
			char *doubleReturnLoc = strstr(recvbuf, "\r\n\r\n");
			if (doubleReturnLoc != NULL){
				char method[10] = { 0 };
				char pathAndFile[100] = { 0 };
				char host[100] = { 0 };
				char sendBuf[DEFAULT_BUFLEN] = { 0 };
				strncpy_s(method, recvbuf, 3);
				char *space2Loc = strchr(recvbuf + 4, ' ');
				strncpy_s(pathAndFile, recvbuf + 4, space2Loc - recvbuf - 4);

				// GET method
				if (strcmp(method, "GET") == 0){
					if (strcmp(pathAndFile, "/dir/hello.html") == 0){
						printf("Inside hello.html\n");
						// Read the file
						FILE *fp;
						char content[800] = { 0 };
						if (fopen_s(&fp, "C:\\Users\\yy\\Desktop\\dir\\hello.html", "r") == 0){
							char tempBuf[100] = { 0 };
							while (fgets(tempBuf, 100, fp) != NULL){
								strcat_s(content, tempBuf);
							}
							fclose(fp);
							int contentLength = strlen(content);

							// Assemble the message
							sprintf_s(sendBuf, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n%s", contentLength, content);
							// Send the http reply
	                        iSendResult = send(ClientSocket, sendBuf, strlen(sendBuf), 0);

	                        if (iSendResult == SOCKET_ERROR) {
	                            printf("send failed with error: %d\n", WSAGetLastError());
	    	                }
	                        memset(recvbuf, 0, recvbuflen);
						}
						else{
							// todo: 404
							printf("Cannot open the file!\n");
						}
						
					}
					else if (1){
						//todo: request for other files
					}

				}
				// POST method
				else if (strcmp(method, "POST") == 0){
					// todo: Post
				}

			}


	    }
	    else if (iResult < 0){
	   	    printf("recv failed with error: %d\n", WSAGetLastError());
	    }
	} while (1);

}

int main(void)
{
	WSADATA wsaData;
	int iResult;

	SOCKET ListenSocket = INVALID_SOCKET;

	struct addrinfo *result = NULL;
	struct addrinfo hints;
	modifyListMutex = CreateMutex(NULL, FALSE, NULL);

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
	printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);

	// Listen
	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	sockaddr_in clientAddr;
	int clientLen = 32;
    SOCKET ClientSocket = INVALID_SOCKET;
	SOCKET* sockPtr = &ClientSocket;

	while (1){
	    // Accept a client socket

	    ClientSocket = accept(ListenSocket, (sockaddr*)&clientAddr, (socklen_t*)&clientLen);


	    if (ClientSocket == INVALID_SOCKET) {
	    	printf("accept failed with error: %d\n", WSAGetLastError());
	    	closesocket(ListenSocket);
	    	WSACleanup();
	    	return 1;
	    }

		clientAddrList[currClientNum] = clientAddr;
		clientSocketList[currClientNum] = ClientSocket;
		struct addrAndSocket currAddrAndSocket;
		currAddrAndSocket.addr = clientAddr;
		currAddrAndSocket.sock = ClientSocket;
		struct addrAndSocket *ptrCurr = &currAddrAndSocket;

		currClientNum++;
		
		_beginthread(recvSend, 0, (void*)ptrCurr);

	}

	// cleanup
	CloseHandle(modifyListMutex);
	closesocket(ListenSocket);
	WSACleanup();

	return 0;
}