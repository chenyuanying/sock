#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define DEFAULT_BUFLEN 512

#include<Windows.h>
#include<WinSock2.h>
#include<WS2tcpip.h>
#include<IPHlpApi.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<process.h>


#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")

char recvbuf[DEFAULT_BUFLEN];
int recvbuflen = DEFAULT_BUFLEN;

SOCKET ConnectSocket = INVALID_SOCKET;

void receive(void *param){
	int iResult;
    do{
	    iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0){
			printf("%s\n", recvbuf);
		}
	    else if (iResult < 0)
	        printf("recv failed: %d\n", WSAGetLastError());
	   } while (ConnectSocket != INVALID_SOCKET);
	_endthread();

}

int main(void){

    WSADATA wsaData;
    int iResult;

	printf("Enter quit to quit the program or enter connect<ip>port to connect the server\n");

	char input[100];
	char addr[100];
	char port[100];
	
	memset(addr, 0, sizeof(addr));
	while (1){
	    while (1){
	    	scanf_s("%s", input, 100);
	    	if (strcmp(input, "quit") == 0){
	    		return 0;
	    	}
	    	else if (strlen(input) > 7){
	    		char prefix7[100];
	    		strncpy_s(prefix7, input, 8);
	    		if (strcmp(prefix7, "connect<") == 0){
	    			if (strchr(input, '>') != NULL){
	    			    int rightAngleBracketLoc = strchr(input, '>') - input;
	    			    strncpy_s(addr, input + 8, rightAngleBracketLoc - 8);
	    			    strncpy_s(port, input + rightAngleBracketLoc + 1, strlen(input) - rightAngleBracketLoc - 1);
	    				break;
	    			}
	    		}
	    	}
	    	printf("Please input validate command, e.g., connect<192.168.1.1>10086\n");
	    }

	    // initialize the winsock
        iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
        if(iResult != 0){
            printf("WSAStartup failed: %d\n", iResult);
            return 1;
        }

        struct addrinfo *result = NULL,
                        *ptr = NULL,
                        hints;
    
        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
    
	    // Resolve the server address and port
        iResult = getaddrinfo(addr, port, &hints, &result);
        if(iResult != 0){
            printf("getaddinfo failed: %d\n", iResult);
            WSACleanup();
            return 1;
        }


        // Attempt to connect the first address returned by
        // the call to getaddrinfo
	    for (ptr = result; ptr != NULL; ptr = ptr->ai_next){

            // Create a SOCKET for connecting to server
            ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

	    	if (ConnectSocket == INVALID_SOCKET){
	    		printf("Error at socket(): %ld\n", WSAGetLastError());
	    		WSACleanup();
	    		return 1;
	    	}

	      // Connect to server
	        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
	        if (iResult == SOCKET_ERROR){
	    	    closesocket(ConnectSocket);
	    	    ConnectSocket = INVALID_SOCKET;
	    	    continue;
	        }

	      break;
	    	
	    }
	
	    freeaddrinfo(result);

	    if (ConnectSocket == INVALID_SOCKET){
	    	printf("Unable to connect to server!\n");
	    	WSACleanup();
	    	return 1;
	    }

	    // Connect successfully
	    printf("Connect sucessfully!\n\n");
	    printf("disconn: disconnect with server\n"
	    	"time: get server time\n"
	    	"name: get server name\n"
	    	"list: get the information of all clients that connected to server\n"
	    	"send<id>info: send the information to another client\n"
	    	"quit: quit the program\n");

		// Create a thread to receive data from server

        _beginthread(receive, 0, NULL);

	    // Read the command and sending&receiving data on the client

	    while (1){

	        char sendbuf[100];
	    	memset(sendbuf, 0, sizeof(sendbuf));
			memset(recvbuf, 0, recvbuflen);

	    	scanf_s("%s", input, 100);
	    	if (strcmp(input, "time") == 0 || strcmp(input, "name") == 0 || strcmp(input, "list") == 0){
	    		strcpy_s(sendbuf, input);

				// Send data
	            iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
	            if (iResult == SOCKET_ERROR){
	            	printf("send failed: %d\n", WSAGetLastError());
	            	closesocket(ConnectSocket);
	            	WSACleanup();
	            	return 1;
	            }

	    	}
	    	else if (strcmp(input, "disconnect") == 0){
	            // Shutdown the connection for sending since no more data will be sent
	            // The client can still use the ConnectSocket for receiving data

				strcpy_s(sendbuf, "disconnect");
	            iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);

	            if (iResult == SOCKET_ERROR){
	            	printf("send failed: %d\n", WSAGetLastError());
	            	closesocket(ConnectSocket);
	            	WSACleanup();
	            	return 1;
	            }

	            iResult = shutdown(ConnectSocket, SD_SEND);

	            if (iResult == SOCKET_ERROR){
	            	printf("shutdown failed: %d\n", WSAGetLastError());
	            	closesocket(ConnectSocket);
	            	WSACleanup();
	            	return 1;
	            }

	           	closesocket(ConnectSocket);
                ConnectSocket = INVALID_SOCKET;

	            printf("Connection closed...\n");
	            printf("Enter quit to quit the program or enter connect<ip>port to connect the server\n");
	    		break;
	    		
	    	}
			else if (strcmp(input, "quit") == 0){
				//quit
				strcpy_s(sendbuf, "disconnect");
	            iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
	            if (iResult == SOCKET_ERROR){
	            	printf("send failed: %d\n", WSAGetLastError());
	            	closesocket(ConnectSocket);
	            	WSACleanup();
	            	return 1;
	            }

	            iResult = shutdown(ConnectSocket, SD_SEND);
	            if (iResult == SOCKET_ERROR){
	            	printf("shutdown failed: %d\n", WSAGetLastError());
	            	closesocket(ConnectSocket);
	            	WSACleanup();
	            	return 1;
	            }

	            // Cleanup
	            closesocket(ConnectSocket);
	            WSACleanup();
                return 0;

			}
	    	else if (strlen(input) > 5){
	    		char prefix7[100];
	    		strncpy_s(prefix7, input, 5);
	    		if (strcmp(prefix7, "send<") == 0){
	    			if (strchr(input, '>') != NULL){
	    				strcpy_s(sendbuf, input);

				        // Send data
	                    iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
	                    if (iResult == SOCKET_ERROR){
	                    	printf("send failed: %d\n", WSAGetLastError());
	                    	closesocket(ConnectSocket);
	                    	WSACleanup();
	                    	return 1;
	                    }

	    		    }
	    		}

	    	}
	    	else{
	    	    printf("Please input validate command\n");
	    	}
	    }

	}
	
	return 1;
}
