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

#define DEFAULT_BUFLEN 7512
#define DEFAULT_PORT "21194"
#define MAX_CLIENT_NUM 1024
#define MAX_NAME_OR_PASSWORD_LEN 50


HANDLE modifyListMutex;


static char* not_found_response_template =
"HTTP/1.1 404 Not Found\r\n"
"Content-Length: 116\r\n"
"Content-Type: text/html\r\n"
"\r\n"
"<html>\r\n"
" <body>\r\n"
"  <h1>Not Found</h1>\r\n"
"  <p>The requested URL was not found on this server.</p>\r\n"
" </body>\r\n"
"</html>\r\n";

static char* verify_success_template =
"HTTP/1.1 200 OK\r\n"
"Content-Length: 58\r\n"
"Content-Type:text/html\r\n"
"\r\n"
"<html>\r\n"
" <body>\r\n"
"  Login Successfully\r\n"
" </body>\r\n"
"</html>\r\n";

static char* verify_fail_template =
"HTTP/1.1 200 OK\r\n"
"Content-Length: 52\r\n"
"Content-Type:text/html\r\n"
"\r\n"
"<html>\r\n"
" <body>\r\n"
"  Login failed\r\n"
" </body>\r\n"
"</html>\r\n";


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
				strncpy_s(method, recvbuf, 4);

				// GET method
				if (strcmp(method, "GET ") == 0){
				    char *space2Loc = strchr(recvbuf + 4, ' ');
				    strncpy_s(pathAndFile, recvbuf + 4, space2Loc - recvbuf - 4);

					// Case1: request for noimg.html

					if (strcmp(pathAndFile, "/dir/noimg.html") == 0){
						// Read the file
						FILE *fp;
						char content[800] = { 0 };
						if (fopen_s(&fp, "C:\\Users\\yy\\Desktop\\dir\\noimg.html", "r") == 0){
							char tempBuf[100] = { 0 };
							while (fgets(tempBuf, 100, fp) != NULL){
								strcat_s(content, tempBuf);
							}
							fclose(fp);
							int contentLength = strlen(content);

							// Assemble the message
							sprintf_s(sendBuf, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n"
								"Content-Type: text/html; charset=UTF-8\r\n\r\n%s", contentLength, content);
							// Send the http reply
	                        iSendResult = send(ClientSocket, sendBuf, strlen(sendBuf), 0);

	                        if (iSendResult == SOCKET_ERROR) {
	                            printf("send failed with error: %d\n", WSAGetLastError());
	                            memset(recvbuf, 0, recvbuflen);
								break;
	    	                }
						}
						// Cannot find the file, return http 404
						else{
							sprintf_s(sendBuf, "%s", not_found_response_template);
	                        iSendResult = send(ClientSocket, sendBuf, strlen(sendBuf), 0);
	                        if (iSendResult == SOCKET_ERROR) {
	                            printf("send failed with error: %d\n", WSAGetLastError());
	                            memset(recvbuf, 0, recvbuflen);
								break;
	    	                }
	                        closesocket(ClientSocket);
				            _endthread();
						}
					}
					
					// Case 2: request for logo.jpg
					else if (strcmp(pathAndFile, "/dir/img/logo.jpg") == 0){
						FILE *fp = NULL;
						char *content = NULL;
						if (fopen_s(&fp, "C:\\Users\\yy\\Desktop\\dir\\img\\logo.jpg", "rb") == 0){
							fseek(fp, 0, SEEK_END);
							int picSize = ftell(fp);
							rewind(fp);
							
							content = (char *)malloc(sizeof(char)*picSize);
							if (content == NULL){
								printf("Memory Error\n");
								break;
							}

							if (fread(content, 1, picSize, fp) != picSize){
								printf("Read Error\n");
								break;
							}
							fclose(fp);

							char header[DEFAULT_BUFLEN];
							sprintf_s(header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n"
								"Content-Type: image/jpeg; charset=UTF-8\r\n\r\n", picSize);
							int headerSize = strlen(header);
							memcpy(sendBuf, header, headerSize);
							memcpy(sendBuf + headerSize, content, picSize);

	                        iSendResult = send(ClientSocket, sendBuf, headerSize + picSize, 0);

							free(content);
							content = NULL;

	                        if (iSendResult == SOCKET_ERROR) {
	                            printf("send failed with error: %d\n", WSAGetLastError());
	                            memset(recvbuf, 0, recvbuflen);
								break;
	    	                }
						}
						else{
							sprintf_s(sendBuf, "%s", not_found_response_template);
	                        iSendResult = send(ClientSocket, sendBuf, strlen(sendBuf), 0);
	                        if (iSendResult == SOCKET_ERROR) {
	                            printf("send failed with error: %d\n", WSAGetLastError());
	                            memset(recvbuf, 0, recvbuflen);
								break;
	    	                }
	                        closesocket(ClientSocket);
				            _endthread();
						}

					}
					// Case 3: request for test.html
					else if (strcmp(pathAndFile, "/dir/test.html") == 0){
						// Read the file
						FILE *fp;
						char content[800] = { 0 };
						if (fopen_s(&fp, "C:\\Users\\yy\\Desktop\\dir\\test.html", "r") == 0){
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
	                            memset(recvbuf, 0, recvbuflen);
								break;
	    	                }
						}
						else{
							sprintf_s(sendBuf, "%s", not_found_response_template);
	                        iSendResult = send(ClientSocket, sendBuf, strlen(sendBuf), 0);
	                        if (iSendResult == SOCKET_ERROR) {
	                            printf("send failed with error: %d\n", WSAGetLastError());
	                            memset(recvbuf, 0, recvbuflen);
								break;
	    	                }
	                        closesocket(ClientSocket);
				            _endthread();
						}
					}
					else{
						sprintf_s(sendBuf, "%s", not_found_response_template);
						iSendResult = send(ClientSocket, sendBuf, strlen(sendBuf), 0);
						if (iSendResult == SOCKET_ERROR){
							printf("send failed with error: %d\n", WSAGetLastError());
							memset(recvbuf, 0, recvbuflen);
							break;
						}
	                    closesocket(ClientSocket);
				        _endthread();
					}
				}
				// POST method
				else if (strcmp(method, "POST") == 0){
				    char *space2Loc = strchr(recvbuf + 5, ' ');
				    strncpy_s(pathAndFile, recvbuf + 5, space2Loc - recvbuf - 5);
					if (strcmp(pathAndFile, "/dir/dopost") == 0){
						char form[DEFAULT_BUFLEN] = { 0 };
						strncpy_s(form, doubleReturnLoc + 4, strlen(recvbuf) - (doubleReturnLoc + 4 - recvbuf));
						char *andSplit = strchr(form, '&');
						char loginStr[MAX_NAME_OR_PASSWORD_LEN] = { 0 };
						char passwordStr[MAX_NAME_OR_PASSWORD_LEN] = { 0 };
						strncpy_s(loginStr, form, andSplit - form);
						strncpy_s(passwordStr, andSplit + 1, strlen(form) - (andSplit - form) - 1);
						char username[MAX_NAME_OR_PASSWORD_LEN];
						char password[MAX_NAME_OR_PASSWORD_LEN];
						sscanf_s(loginStr, "login=%s", username, MAX_NAME_OR_PASSWORD_LEN);
						sscanf_s(passwordStr, "pass=%s", password, MAX_NAME_OR_PASSWORD_LEN);
						if (strcmp(username, "21521194") == 0 && strcmp(password, "1194") == 0){
							// Verify sucessfully
							strcpy_s(sendBuf, verify_success_template);
						}
						else{
							// Verify failed
							strcpy_s(sendBuf, verify_fail_template);
						}
						iSendResult = send(ClientSocket, sendBuf, strlen(sendBuf), 0);
						if (iSendResult == SOCKET_ERROR){
							printf("send failed with error: %d\n", WSAGetLastError());
							memset(recvbuf, 0, recvbuflen);
							break;
						}
						// "Post" need to clear the recvBuf
						memset(recvbuf, 0, recvbuflen);
					}
					else {
						sprintf_s(sendBuf, "%s", not_found_response_template);
						iSendResult = send(ClientSocket, sendBuf, strlen(sendBuf), 0);
						if (iSendResult == SOCKET_ERROR){
							printf("send failed with error: %d\n", WSAGetLastError());
							memset(recvbuf, 0, recvbuflen);
							break;
						}
	                    closesocket(ClientSocket);
				        _endthread();
					}

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

		struct addrAndSocket currAddrAndSocket;
		currAddrAndSocket.addr = clientAddr;
		currAddrAndSocket.sock = ClientSocket;
		struct addrAndSocket *ptrCurr = &currAddrAndSocket;

		_beginthread(recvSend, 0, (void*)ptrCurr);

	}

	// cleanup
	CloseHandle(modifyListMutex);
	closesocket(ListenSocket);
	WSACleanup();

	return 0;
}