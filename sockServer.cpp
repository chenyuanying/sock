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
	            if (GetComputerName(infoBuf, &bufCharCount)){
	            	for (i = 0; i<150; i++){
	            		namebuf[i] = infoBuf[i];
	            	}
	            }
	            else{
	            	strcpy_s(namebuf, "Unknown_Host_Name\n");
	            }
				strcat_s(namebuf,"\n");

	            iSendResult = send(ClientSocket, namebuf, strlen(namebuf), 0);
	            if (iSendResult == SOCKET_ERROR) {
	                printf("send failed with error: %d\n", WSAGetLastError());
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
	    			sprintf_s(record, "%d    %s:%d\n", i, clientAddrStr, ntohs(clientAddrList[i].sin_port));
	    			strcat_s(listbuf, record);
	    		}
	    		iSendResult = send(ClientSocket, listbuf, strlen(listbuf), 0);
	    	    if (iSendResult == SOCKET_ERROR) {
	    		    printf("send failed with error: %d\n", WSAGetLastError());
	    	    }
	            memset(recvbuf, 0, recvbuflen);
	    		continue;
	    	}

	    	// case 4: request for sending message
			// assumption: list has not been changed
	    	char messageInbuf[1024];
			char messageOutbuf[1024];
	    	char destNo[10];
			int sourceNoInt = -1;
			int destNoInt = -1;
	        if (strlen(recvbuf) > 5){
	        	char prefix5[100];
	        	strncpy_s(prefix5, recvbuf, 5);
	        	if (strcmp(prefix5, "send<") == 0){
	        		if (strchr(recvbuf, '>') != NULL){
						// Find the number of source
				        int i;
				        for (i = 0; i < currClientNum; ++i){
				        	if (addr.sin_addr.S_un.S_addr == clientAddrList[i].sin_addr.s_addr && addr.sin_port == clientAddrList[i].sin_port){
				        		break;
				        	}
				        }
						sourceNoInt = i;

	        		    int rightAngleBracketLoc = strchr(recvbuf, '>') - recvbuf;
	        		    strncpy_s(destNo, recvbuf + 5, rightAngleBracketLoc - 5);
	        		    strncpy_s(messageInbuf, recvbuf + rightAngleBracketLoc + 1, strlen(recvbuf) - rightAngleBracketLoc - 1);
	    			    sprintf_s(messageOutbuf, "From[%d]:%s\n", sourceNoInt, messageInbuf);

						destNoInt = atoi(destNo); 
	    		        iSendResult = send(clientSocketList[destNoInt], messageOutbuf, strlen(messageOutbuf), 0);
	    	            if (iSendResult == SOCKET_ERROR) {
	    		            printf("send failed with error: %d\n", WSAGetLastError());
	    	            }
	                    memset(recvbuf, 0, recvbuflen);
	    		        continue;
	        		}
	        	}
	        }

			// case 5: request for disconnecting
	    	if (strcmp(recvbuf, "disconnect") == 0){
	            // shutdown the connection since we're done
	            iResult = shutdown(ClientSocket, SD_SEND);
	            if (iResult == SOCKET_ERROR) {
	            	printf("shutdown failed with error: %d\n", WSAGetLastError());
	            }
	            closesocket(ClientSocket);
				WaitForSingleObject(modifyListMutex, INFINITE);
				int i;
				for (i = 0; i < currClientNum; ++i){
					if (addr.sin_addr.S_un.S_addr == clientAddrList[i].sin_addr.s_addr && addr.sin_port == clientAddrList[i].sin_port){
						break;
					}
				}
				i++;
				for (; i < currClientNum; ++i){
					clientAddrList[i - 1] = clientAddrList[i];
					clientSocketList[i - 1] = clientSocketList[i];
				}

				currClientNum--;

				ReleaseMutex(modifyListMutex);
				_endthread();
	    		break;
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