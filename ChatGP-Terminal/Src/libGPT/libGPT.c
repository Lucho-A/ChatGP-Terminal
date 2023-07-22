/*
 ============================================================================
 Name        : libGPT.c
 Author      : L. (lucho-a.github.io)
 Version     : 1.0.1
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

int libGPT_init(ChatGPT *cgtp, char *api, char *systemRole, long int maxTokens, double temperature){
	cgtp->api=malloc(strlen(api)*sizeof(char)+1);
	if(cgtp->api ==NULL) return LIBGPT_INIT_ERROR;
	snprintf(cgtp->api,strlen(api)+1,"%s",api);
	cgtp->systemRole=malloc(strlen(systemRole)*sizeof(char)+1);
	if(cgtp->systemRole ==NULL) return LIBGPT_INIT_ERROR;
	snprintf(cgtp->systemRole,strlen(systemRole)+1,"%s",systemRole);
	cgtp->maxTokens=maxTokens;
	cgtp->temperature=temperature;
	return RETURN_OK;
}

int libGPT_clean(ChatGPT *cgpt){
	free(cgpt->api);
	free(cgpt->systemRole);
	return RETURN_OK;
}

static int libGPT_parse_result(ChatGPTResponse *cgptResponse){
	//TODO Dynamically memory allocation
	int i=0,cont=0;
	char *message=strstr(cgptResponse->jsonMessage,"\"error\": {");
	if(message!=NULL){
		message=strstr(cgptResponse->jsonMessage,"\"message\": \"");
		for(i=strlen("\"message\": \"");(message[i-1]=='\\' || message[i]!='\"') && cont<BUFFER_SIZE_JSON_MESSAGE-1;i++,cont++) cgptResponse->message[cont]=message[i];
		cgptResponse->message[cont]=0;
		if(cont>=BUFFER_SIZE_JSON_MESSAGE-1) return LIBGPT_BUFFERSIZE_OVERFLOW;
		return LIBGPT_RESPONSE_MESSAGE_ERROR;
	}

	cont=0;
	message=strstr(cgptResponse->jsonMessage,"\"content\": \"");
	for(i=strlen("\"content\": \"");(message[i-1]=='\\' || message[i]!='\"') && cont<BUFFER_SIZE_MESSAGE-1;i++,cont++) cgptResponse->message[cont]=message[i];
	cgptResponse->message[cont]=0;
	if(cont>=BUFFER_SIZE_MESSAGE-1) return LIBGPT_BUFFERSIZE_OVERFLOW;

	cont=0;
	message=strstr(cgptResponse->jsonMessage,"\"finish_reason\": \"");
	for(i=strlen("\"finish_reason\": \"");(message[i-1]=='\\' || message[i]!='\"') && cont<BUFFER_SIZE_FINISHED_REASON-1;i++,cont++) cgptResponse->finishReason[cont]=message[i];
	cgptResponse->finishReason[cont]=0;
	if(cont>=BUFFER_SIZE_FINISHED_REASON-1) return LIBGPT_BUFFERSIZE_OVERFLOW;

	cont=0;
	message=strstr(cgptResponse->jsonMessage,"\"total_tokens\": ");
	for(i=strlen("\"total_tokens\": ");cont<BUFFER_SIZE_TOTAL_TOKENS-1 && message[i]!='\n';i++,cont++) cgptResponse->totalTokens[cont]=message[i];
	cgptResponse->totalTokens[cont]=0;
	if(cont>=BUFFER_SIZE_TOTAL_TOKENS-1) return LIBGPT_BUFFERSIZE_OVERFLOW;
	return RETURN_OK;
}

int libGPT_send_chat(ChatGPT cgpt, ChatGPTResponse *cgptResponse, char *message){
	memset(cgptResponse->jsonMessage,0,BUFFER_SIZE_JSON_MESSAGE);
	memset(cgptResponse->message,0,BUFFER_SIZE_MESSAGE);
	memset(cgptResponse->finishReason,0,BUFFER_SIZE_FINISHED_REASON);
	memset(cgptResponse->totalTokens,0,BUFFER_SIZE_TOTAL_TOKENS);
	struct pollfd pfds[1];
	int numEvents=0,pollinHappened=0,bytesSent=0;
	SSL *sslConn=NULL;
	SSL_CTX *sslCtx=NULL;
	int socketConn=0;
	struct hostent *he;
	struct in_addr **addr_list;
	if((he=gethostbyname(LIBGPT_OPENAI_API_URL))==NULL) return LIBGPT_GETTING_HOST_INFO_ERROR;
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
	if(numEvents==0){
		close(socketConn);
		return LIBGPT_SOCKET_SEND_TIMEOUT_ERROR;
	}
	pollinHappened=pfds[0].revents & POLLOUT;
	unsigned int bufferSize=strlen(message)*sizeof(char)*2;
	char *messageParsed=malloc(bufferSize);
	memset(messageParsed,0,bufferSize);
	int cont=0;
	for(int i=0;i<strlen(message);i++,cont++){
		switch(message[i]){
		case '\"':
		case '\\':
			messageParsed[cont]='\\';
			messageParsed[++cont]=message[i];
			break;
		case '\n':
			messageParsed[cont]='\\';
			messageParsed[++cont]='n';
			break;
		default:
			messageParsed[cont]=message[i];
		}
	}
	messageParsed[cont]='\0';
	bufferSize=strlen(messageParsed)*sizeof(char)+BUFFER_SIZE_512B;
	char *payload=malloc(bufferSize);
	memset(payload,0,bufferSize);
	snprintf(payload,bufferSize,
			"{"
			"\"model\":\"gpt-3.5-turbo\","
			"\"messages\":["
			"{\"role\":\"system\",\"content\":\"%s\"},"
			"{\"role\":\"user\",\"content\":\"%s\"}],"
			"\"max_tokens\": %ld,"
			"\"temperature\": %.2f"
			"}",cgpt.systemRole,messageParsed,cgpt.maxTokens,cgpt.temperature);
	free(messageParsed);
	bufferSize=strlen(payload)*sizeof(char)+BUFFER_SIZE_512B;
	char *httpMsg=malloc(bufferSize);
	memset(httpMsg,0,bufferSize);
	snprintf(httpMsg,bufferSize,
			"POST /v1/chat/completions HTTP/1.1\r\n"
			"Host: %s\r\n"
			"user-agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)\r\n"
			"accept: */*\r\n"
			"content-type: application/json\r\n"
			"authorization: Bearer %s\r\n"
			"connection: close\r\n"
			"content-length: %ld\r\n\r\n"
			"%s",LIBGPT_OPENAI_API_URL,cgpt.api,strlen(payload),payload);
	free(payload);
	if(pollinHappened){
		bytesSent=SSL_write(sslConn, httpMsg, strlen(httpMsg));
		free(httpMsg);
		if(bytesSent<=0){
			close(socketConn);
			return LIBGPT_SENDING_PACKETS_ERROR;
		}
	}else{
		free(httpMsg);
		close(socketConn);
		return LIBGPT_POLLIN_ERROR;
	}
	int bytesReceived=0,contI=0, totalBytesReceived=0;
	pfds[0].events=POLLIN;
	char buffer[BUFFER_SIZE_16K]="";
	do{
		numEvents=poll(pfds, 1, LIBGPT_SOCKET_RECV_TIMEOUT_MS);
		if(numEvents==0){
			close(socketConn);
			return LIBGPT_SOCKET_RECV_TIMEOUT_ERROR;
		}
		pollinHappened = pfds[0].revents & POLLIN;
		if (pollinHappened){
			bytesReceived=SSL_read(sslConn,buffer, BUFFER_SIZE_16K);
			if(bytesReceived>0){
				totalBytesReceived+=bytesReceived;
				for(int i=0; i<bytesReceived && cont < BUFFER_SIZE_JSON_MESSAGE; i++, contI++) cgptResponse->jsonMessage[contI]=buffer[i];
				if(cont>=BUFFER_SIZE_JSON_MESSAGE) return LIBGPT_BUFFERSIZE_OVERFLOW;
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
	if(totalBytesReceived==0) return LIBGPT_ZEROBYTESRECV_ERROR;
	return libGPT_parse_result(cgptResponse);
}
