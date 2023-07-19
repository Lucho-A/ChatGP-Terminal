/*
 ============================================================================
 Name        : libGPT.c
 Author      : L. (lucho-a.github.io)
 Version     : 1.0.0
 Created on	 : 2023/07/18
 Copyright   : GNU General Public License v3.0
 Description : C file
 ============================================================================
 */

#include "libGPT.h"

#include <stdio.h>
#include <sys/socket.h>
#include <poll.h>
#include <openssl/ssl.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int libGPT_init(ChatGPT *cgtp, char *userApi, char *systemRole, long int maxTokens, double temperature){
	snprintf(cgtp->api,sizeof(cgtp->api),"%s",userApi);
	snprintf(cgtp->systemRole,sizeof(cgtp->systemRole),"%s",systemRole);
	cgtp->maxTokens=maxTokens;
	cgtp->temperature=temperature;
	return RETURN_OK;
}

int libGPT_send_chat(ChatGPT cgtp, ChatGPTResponse *cgptResponse, char *message){
	struct pollfd pfds[1];
	int numEvents=0,pollinHappened=0,bytesSent=0;
	SSL *sslConn=NULL;
	SSL_CTX *sslCtx=NULL;
	int socketConn=0;
	struct hostent *he;
	struct in_addr **addr_list;
	if((he=gethostbyname(LIBGPT_OPENAI_API_URL))==NULL) return -1;
	addr_list=(struct in_addr **) he->h_addr_list;
	char *chatGptIp=inet_ntoa(*addr_list[0]);
	struct sockaddr_in serverAddress;
	serverAddress.sin_family=AF_INET;
	serverAddress.sin_port=htons(443);
	serverAddress.sin_addr.s_addr=inet_addr(chatGptIp);
	if((socketConn=socket(AF_INET, SOCK_STREAM, 0))<0) return LIBGPT_SOCKET_CREATION_ERROR;
	int socketFlags = fcntl(socketConn, F_GETFL, 0);
	fcntl(socketConn, F_SETFL, socketFlags | O_NONBLOCK);
	connect(socketConn, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
	fd_set rFdset, wFdset;
	struct timeval tv;
	FD_ZERO(&rFdset);
	FD_SET(socketConn, &rFdset);
	wFdset=rFdset;
	tv.tv_sec=LIBGPT_SOCKET_CONNECT_TIMEOUT_S;
	tv.tv_usec=0;
	if(select(socketConn+1,&rFdset,&wFdset,NULL,&tv)<=0) return LIBGPT_SOCKET_CONNECTION_TIMEOUT_ERROR;
	fcntl(socketConn, F_SETFL, socketFlags);
	if((sslCtx = SSL_CTX_new(SSLv23_method()))==NULL) return LIBGPT_SSL_CONTEXT_ERROR;
	if((sslConn = SSL_new(sslCtx))==NULL) return LIBGPT_SSL_CONTEXT_ERROR;
	if(!SSL_set_fd(sslConn, socketConn)) return LIBGPT_SSL_FD_ERROR;
	SSL_set_connect_state(sslConn);
	SSL_set_tlsext_host_name(sslConn, LIBGPT_OPENAI_API_URL);
	if(!SSL_connect(sslConn)) return LIBGPT_SSL_CONNECT_ERROR;
	fcntl(socketConn, F_SETFL, O_NONBLOCK);
	pfds[0].fd=socketConn;
	pfds[0].events=POLLOUT;
	numEvents=poll(pfds,1,LIBGPT_SOCKET_SEND_TIMEOUT_MS);
	if(numEvents==0) return LIBGPT_SOCKET_SEND_TIMEOUT_ERROR;
	pollinHappened=pfds[0].revents & POLLOUT;
	char payload[BUFFER_SIZE_1K]="";
	char httpMsg[BUFFER_SIZE_8K]="";
	snprintf(payload,BUFFER_SIZE_1K,
			"{"
			"\"model\":\"gpt-3.5-turbo\","
			"\"messages\":["
			"{\"role\":\"system\",\"content\":\"%s\"},"
			"{\"role\":\"user\",\"content\":\"%s\"}],"
			"\"max_tokens\": %ld,"
			"\"temperature\": %.2f"
			"}\r\n\r\n",cgtp.systemRole,message,cgtp.maxTokens,cgtp.temperature);
	snprintf(httpMsg,BUFFER_SIZE_8K,
			"POST /v1/chat/completions HTTP/1.1\r\n"
			"Host: %s\r\n"
			"user-agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)\r\n"
			"content-type: application/json\r\n"
			"authorization: Bearer %s\r\n"
			"content-length: %ld\r\n\r\n"
			"%s \r\n\r\n",LIBGPT_OPENAI_API_URL,cgtp.api,strlen(payload),payload);
	if(pollinHappened){
		bytesSent=SSL_write(sslConn, httpMsg, strlen(httpMsg));
		if(bytesSent<=0){
			close(socketConn);
			return LIBGPT_SENDING_PACKETS_ERROR;
		}
	}else{
		close(socketConn);
		return LIBGPT_POLLIN_ERROR;
	}
	int bytesReceived=0,contI=0, totalBytesReceived=0;;
	pfds[0].events=POLLIN;
	char buffer[BUFFER_SIZE_16K]="";
	do{
		numEvents=poll(pfds, 1, LIBGPT_SOCKET_RECV_TIMEOUT_MS);
		if(numEvents==0){
			close(socketConn);
			break;
		}
		pollinHappened = pfds[0].revents & POLLIN;
		if (pollinHappened){
			bytesReceived=SSL_read(sslConn,buffer, BUFFER_SIZE_16K);
			if(bytesReceived>0){
				totalBytesReceived+=bytesReceived;
				for(int i=0; i<bytesReceived; i++, contI++) cgptResponse->jsonMessage[contI]=buffer[i];
				continue;
			}
			if(bytesReceived==0){
				close(socketConn);
				break;
			}
			if(bytesReceived<0 && (errno==EAGAIN || errno==EWOULDBLOCK)) continue;
			if(bytesReceived<0 && (errno!=EAGAIN)){
				close(socketConn);
				return LIBGPT_RECEIVING_PACKETS_ERROR;
			}
		}else{
			return LIBGPT_POLLIN_ERROR;
		}
	}while(TRUE);

	//TODO
	char *token="\"error\": {";
	if(strstr(cgptResponse->jsonMessage,token)!=NULL){
		token="\"message\": \"";
		char *message=strstr(cgptResponse->jsonMessage,token);
		int i=0,cont=0;
		for(i=strlen(token);message[i-1]=='\\' || message[i]!='\"';i++,cont++) cgptResponse->errorMessage[cont]=message[i];
		cgptResponse->errorMessage[cont]=0;
		return LIBGPT_RESPONSE_MESSAGE_ERROR;
	}
	if(totalBytesReceived>0){
		token="\"content\": \"";
		char *message=strstr(cgptResponse->jsonMessage,token);
		int i=0,cont=0;
		for(i=strlen(token);message[i-1]=='\\' || message[i]!='\"';i++,cont++) cgptResponse->message[cont]=message[i];
		cgptResponse->message[cont]=0;
		token="\"finish_reason\": \"";
		message=strstr(cgptResponse->jsonMessage,token);
		cont=0;
		for(i=strlen(token);message[i-1]=='\\' || message[i]!='\"';i++,cont++) cgptResponse->finishReason[cont]=message[i];
		cgptResponse->finishReason[cont]=0;
	}
	return totalBytesReceived;
}



