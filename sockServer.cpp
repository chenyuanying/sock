#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <tchar.h>

#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "21194"
#define MAX_CLIENT_NUM 1024

int main(void)
{
	WSADATA wsaData;
	int iResult;

	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	int iSendResult;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;

	sockaddr_in clientAddrList[MAX_CLIENT_NUM];
	int currClientNum = 0;





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

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	while (1){
	    // Accept a client socket
	    sockaddr_in clientAddr;
	    int clientLen = 32;

	    ClientSocket = accept(ListenSocket, (sockaddr*)&clientAddr, (socklen_t*)&clientLen);
	    if (ClientSocket == INVALID_SOCKET) {
	    	printf("accept failed with error: %d\n", WSAGetLastError());
	    	closesocket(ListenSocket);
	    	WSACleanup();
	    	return 1;
	    }

	    // clientAddrList[currClientNum++] = clientAddr;
	    clientAddrList[0] = clientAddr;
		currClientNum = 0;

	    memset(recvbuf, 0, recvbuflen);
	    // Receive until the peer shuts down the connection
	    do {

	    	iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
	    	if (iResult > 0) {

	    		// case 1: request for server time
	    		
	    		if (strcmp(recvbuf, "time") == 0){
            	    // Get system time
            	    // Output: timebuf
            	    time_t rawtime;
            	    struct tm timeCurr;
            	    struct tm *timeinfo = &timeCurr;
            	    char timebuf[50];
            	    memset(timebuf, 0, 50);
            	    time(&rawtime);
            	    localtime_s(timeinfo, &rawtime);
            	    asctime_s(timebuf, timeinfo);

	    			iSendResult = send(ClientSocket, timebuf, strlen(timebuf), 0);
	    		    if (iSendResult == SOCKET_ERROR) {
	    			    printf("send failed with error: %d\n", WSAGetLastError());
	    			    closesocket(ClientSocket);
	    			    WSACleanup();
	    			    return 1;
	    		    }
	                memset(recvbuf, 0, recvbuflen);
	    			continue;
	    		}

	    		// case 2: request for server name

	    		if (strcmp(recvbuf, "name") == 0){
	                // Get computer name
	                // Output: namebuf
	                char namebuf[50];
	                int i = 0;
	                TCHAR infoBuf[50];
	                DWORD bufCharCount = 50;
	                memset(namebuf, 0, 50);
	                if (GetComputerName(infoBuf, &bufCharCount))
	                {
	                	for (i = 0; i<150; i++)
	                	{
	                		namebuf[i] = infoBuf[i];
	                	}
	                }
	                else
	                {
	                	strcpy_s(namebuf, "Unknown_Host_Name");
	                }

	    			iSendResult = send(ClientSocket, namebuf, strlen(namebuf), 0);
	    		    if (iSendResult == SOCKET_ERROR) {
	    			    printf("send failed with error: %d\n", WSAGetLastError());
	    			    closesocket(ClientSocket);
	    			    WSACleanup();
	    			    return 1;
	    		    }
	                memset(recvbuf, 0, recvbuflen);
	    			continue;
	    		}

	    		// case 3: request for listing all the clients
	    		if (strcmp(recvbuf, "list") == 0){
	    			char listbuf[1024];
	    			memset(listbuf, 0, sizeof(listbuf));
	    			for (int i = 0; i < currClientNum; ++i){
	    				char record[100];
	                    char clientAddrStr[20];
	    				memset(record, 0, sizeof(record));
	    				memset(clientAddrStr, 0, sizeof(clientAddrStr));
	    				int clientip = clientAddrList[i].sin_addr.s_addr;
	    				sprintf_s(clientAddrStr, "%d.%d.%d.%d", clientip & 255, (clientip >> 8) & 255, (clientip >> 16) & 255, (clientip >> 24) & 255);
	    				sprintf_s(record, "%d    %s:%d\n", i, clientAddrStr, clientAddrList[i].sin_port);
	    				strcat_s(listbuf, record);
	    			}
	    			iSendResult = send(ClientSocket, listbuf, strlen(listbuf), 0);
	    		    if (iSendResult == SOCKET_ERROR) {
	    			    printf("send failed with error: %d\n", WSAGetLastError());
	    			    closesocket(ClientSocket);
	    			    WSACleanup();
	    			    return 1;
	    		    }
	                memset(recvbuf, 0, recvbuflen);
	    			continue;
	    		}

	    		// case 4: request for sending message
	    		char messagebuf[1024];
	    		char clientNo[10];
	        	if (strlen(recvbuf) > 5){
	        		char prefix5[100];
	        		strncpy_s(prefix5, recvbuf, 5);
	        		if (strcmp(prefix5, "send<") == 0){
	        			if (strchr(recvbuf, '>') != NULL){
	        			    int rightAngleBracketLoc = strchr(recvbuf, '>') - recvbuf;
	        			    strncpy_s(clientNo, recvbuf + 5, rightAngleBracketLoc - 5);
	        			    strncpy_s(messagebuf, recvbuf + rightAngleBracketLoc + 1, strlen(recvbuf) - rightAngleBracketLoc - 1);
	    			        iSendResult = send(ClientSocket, messagebuf, strlen(messagebuf), 0);
	    		            if (iSendResult == SOCKET_ERROR) {
	    			            printf("send failed with error: %d\n", WSAGetLastError());
	    			            closesocket(ClientSocket);
	    			            WSACleanup();
	    			            return 1;
	    		            }
	                        memset(recvbuf, 0, recvbuflen);
	    			        continue;
	        			}
	        		}
	        	}

	    		if (strcmp(recvbuf, "disconnect") == 0){
	                // shutdown the connection since we're done
	                iResult = shutdown(ClientSocket, SD_SEND);
	                if (iResult == SOCKET_ERROR) {
	                	printf("shutdown failed with error: %d\n", WSAGetLastError());
	                	closesocket(ClientSocket);
	                	WSACleanup();
	                	return 1;
	                }
	                closesocket(ClientSocket);
	    			break;
	    		}

	    	}
	    	else if (iResult < 0){
	    		printf("recv failed with error: %d\n", WSAGetLastError());
	    		closesocket(ClientSocket);
	    		WSACleanup();
	    		return 1;
	    	}

	    } while (1);

	}

	// cleanup
	closesocket(ListenSocket);
	WSACleanup();

	return 0;
}