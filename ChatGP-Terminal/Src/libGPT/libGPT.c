/*
 ============================================================================
 Name        : libGPT.c
 Author      : L. (lucho-a.github.io)
 Version     : 1.0.2
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

#define OPENAI_API_URL						"api.openai.com"
#define OPENAI_API_PORT						443

#define SOCKET_CONNECT_TIMEOUT_S			5
#define SOCKET_SEND_TIMEOUT_MS				5000
#define SOCKET_RECV_TIMEOUT_MS				60000

#define MAX_CHAT_HISTORY					4096
#define MAX_ROLES_SIZE						(1024*256)
#define BUFFER_SIZE_16K						(1024*16)

static char *assistantContext[MAX_CHAT_HISTORY]={""};
static char *userContext[MAX_CHAT_HISTORY]={""};
static int contContext=0;

int libGPT_clean(ChatGPT *cgpt){
	if(cgpt->api!=NULL) free(cgpt->api);
	if(cgpt->systemRole!=NULL) free(cgpt->systemRole);
	return RETURN_OK;
}

int libGPT_clean_response(ChatGPTResponse *cgptResponse){
	if(cgptResponse->httpResponse!=NULL) free(cgptResponse->httpResponse);
	if(cgptResponse->message!=NULL) free(cgptResponse->message);
	if(cgptResponse->finishReason!=NULL) free(cgptResponse->finishReason);
	return RETURN_OK;
}

int libGPT_init(ChatGPT *cgtp, char *api, char *systemRole, long int maxTokens, double temperature){
	SSL_library_init();
	if(systemRole==NULL) systemRole=LIBGPT_DEFAULT_ROLE;
	if(maxTokens==0) maxTokens=LIBGPT_DEFAULT_MAX_TOKENS;
	if(temperature==0) temperature=LIBGPT_DEFAULT_TEMPERATURE;
	cgtp->api=malloc(strlen(api)+1);
	if(cgtp->api ==NULL) return LIBGPT_INIT_ERROR;
	snprintf(cgtp->api,strlen(api)+1,"%s",api);
	cgtp->systemRole=malloc(strlen(systemRole)+1);
	if(cgtp->systemRole ==NULL) return LIBGPT_INIT_ERROR;
	snprintf(cgtp->systemRole,strlen(systemRole)+1,"%s",systemRole);
	cgtp->maxTokens=maxTokens;
	cgtp->temperature=temperature;
	return RETURN_OK;
}

static int libGPT_parse_result(ChatGPTResponse *cgptResponse, bool createContext){
	char buffer[BUFFER_SIZE_16K]="";
	int i=0,cont=0;
	char *message=strstr(cgptResponse->httpResponse,"\"error\": {");
	if(message!=NULL){
		message=strstr(cgptResponse->httpResponse,"\"message\": \"");
		for(i=strlen("\"message\": \"");(message[i-1]=='\\' || message[i]!='\"') && cont<BUFFER_SIZE_16K;i++,cont++) buffer[cont]=message[i];
		if(cont>=BUFFER_SIZE_16K) return LIBGPT_BUFFERSIZE_OVERFLOW;
		cgptResponse->message=malloc(strlen(buffer)+1);
		snprintf(cgptResponse->message,strlen(buffer)+1,"%s", buffer);
		return LIBGPT_RESPONSE_MESSAGE_ERROR;
	}

	memset(buffer,0,BUFFER_SIZE_16K);
	cont=0;
	message=strstr(cgptResponse->httpResponse,"\"content\": \"");
	for(i=strlen("\"content\": \"");(message[i-1]=='\\' || message[i]!='\"') && cont<BUFFER_SIZE_16K;i++,cont++) buffer[cont]=message[i];
	if(cont>=BUFFER_SIZE_16K) return LIBGPT_BUFFERSIZE_OVERFLOW;
	cgptResponse->message=malloc(strlen(buffer)+1);
	snprintf(cgptResponse->message,strlen(buffer)+1,"%s", buffer);
	if(createContext){
		assistantContext[contContext]=malloc(strlen(cgptResponse->message)+1);
		snprintf(assistantContext[contContext],strlen(cgptResponse->message)+1,"%s",cgptResponse->message);
		contContext++;
	}

	memset(buffer,0,BUFFER_SIZE_16K);
	cont=0;
	message=strstr(cgptResponse->httpResponse,"\"finish_reason\": \"");
	for(i=strlen("\"finish_reason\": \"");(message[i-1]=='\\' || message[i]!='\"') && cont<BUFFER_SIZE_16K;i++,cont++) buffer[cont]=message[i];
	if(cont>=BUFFER_SIZE_16K) return LIBGPT_BUFFERSIZE_OVERFLOW;
	cgptResponse->finishReason=malloc(strlen(buffer)+1);
	snprintf(cgptResponse->finishReason,strlen(buffer)+1,"%s", buffer);

	memset(buffer,0,BUFFER_SIZE_16K);
	cont=0;
	message=strstr(cgptResponse->httpResponse,"\"total_tokens\": ");
	for(i=strlen("\"total_tokens\": ");cont<BUFFER_SIZE_16K-1 && message[i]!='\n';i++,cont++) buffer[cont]=message[i];
	if(cont>=BUFFER_SIZE_16K-1) return LIBGPT_BUFFERSIZE_OVERFLOW;
	cgptResponse->totalTokens=strtol(buffer,NULL,10);

	return RETURN_OK;
}

int libGPT_send_chat(ChatGPT cgpt, ChatGPTResponse *cgptResponse, char *message, bool createContext){
	cgptResponse->httpResponse=NULL;
	cgptResponse->message=NULL;
	cgptResponse->finishReason=NULL;

	char *messageParsed=malloc(strlen(message)*2);
	memset(messageParsed,0,strlen(message)*2);
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
	char roles[MAX_ROLES_SIZE]="", buf[BUFFER_SIZE_16K]="";
	for(int i=0;i<contContext;i++){
		snprintf(buf,BUFFER_SIZE_16K,"{\"role\":\"user\",\"content\":\"%s\"},",userContext[i]);
		strcat(roles,buf);
		snprintf(buf,BUFFER_SIZE_16K,"{\"role\":\"assistant\",\"content\":\"%s\"},",assistantContext[i]);
		strcat(roles,buf);
	}
	snprintf(buf,BUFFER_SIZE_16K,"{\"role\":\"user\",\"content\":\"%s\"}",messageParsed);
	strcat(roles,buf);
	if(createContext){
		userContext[contContext]=malloc(strlen(messageParsed)+1);
		snprintf(userContext[contContext],strlen(messageParsed)+1,"%s",messageParsed);
	}
	char *payload=malloc(strlen(roles)+512);
	memset(payload,0,strlen(roles)+512);
	snprintf(payload,strlen(roles)+512,
			"{"
			"\"model\":\"gpt-3.5-turbo\","
			"\"messages\":"
			"["
			"{\"role\":\"system\",\"content\":\"%s\"},"
			"%s"
			"],"
			"\"max_tokens\": %ld,"
			"\"temperature\": %.2f"
			"}",cgpt.systemRole,roles,cgpt.maxTokens,cgpt.temperature);
	free(messageParsed);
	char *httpMsg=malloc(strlen(payload)+512);
	memset(httpMsg,0,strlen(payload)+512);
	snprintf(httpMsg,strlen(payload)+512,
			"POST /v1/chat/completions HTTP/1.1\r\n"
			"Host: %s\r\n"
			"user-agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)\r\n"
			"accept: */*\r\n"
			"content-type: application/json\r\n"
			"authorization: Bearer %s\r\n"
			"connection: close\r\n"
			"content-length: %ld\r\n\r\n"
			"%s",OPENAI_API_URL,cgpt.api,strlen(payload),payload);
	free(payload);

	struct pollfd pfds[1];
	int numEvents=0,pollinHappened=0,bytesSent=0;
	SSL *sslConn=NULL;
	SSL_CTX *sslCtx=NULL;
	int socketConn=0;
	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
    int status;
	if ((status=getaddrinfo(OPENAI_API_URL, NULL, &hints, &res)) != 0) return LIBGPT_GETTING_HOST_INFO_ERROR;
	struct sockaddr_in *ipv4 = (struct sockaddr_in *)res->ai_addr;
	void *addr = &(ipv4->sin_addr);
	char chatGptIp[INET6_ADDRSTRLEN];
	inet_ntop(res->ai_family, addr, chatGptIp, sizeof chatGptIp);
    freeaddrinfo(res);
	struct sockaddr_in serverAddress;
	serverAddress.sin_family=AF_INET;
	serverAddress.sin_port=htons(OPENAI_API_PORT);
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

	tv.tv_sec=SOCKET_CONNECT_TIMEOUT_S;
	tv.tv_usec=0;
	if(select(socketConn+1,&rFdset,&wFdset,NULL,&tv)<=0) return LIBGPT_SOCKET_CONNECTION_TIMEOUT_ERROR;
	fcntl(socketConn, F_SETFL, socketFlags);
	if((sslCtx = SSL_CTX_new(SSLv23_method()))==NULL) return LIBGPT_SSL_CONTEXT_ERROR;
	if((sslConn = SSL_new(sslCtx))==NULL) return LIBGPT_SSL_CONTEXT_ERROR;
	if(!SSL_set_fd(sslConn, socketConn)) return LIBGPT_SSL_FD_ERROR;
	SSL_set_connect_state(sslConn);
	SSL_set_tlsext_host_name(sslConn, OPENAI_API_URL);

	if(!SSL_connect(sslConn)) return LIBGPT_SSL_CONNECT_ERROR;
	fcntl(socketConn, F_SETFL, O_NONBLOCK);
	pfds[0].fd=socketConn;
	pfds[0].events=POLLOUT;
	numEvents=poll(pfds,1,SOCKET_SEND_TIMEOUT_MS);
	if(numEvents==0){
		close(socketConn);
		return LIBGPT_SOCKET_SEND_TIMEOUT_ERROR;
	}

	pollinHappened=pfds[0].revents & POLLOUT;
	if(pollinHappened){
		int totalBytesSent=0;
		while(totalBytesSent<strlen(httpMsg)){
			bytesSent=SSL_write(sslConn, httpMsg + totalBytesSent, strlen(httpMsg) - totalBytesSent);
			totalBytesSent+=bytesSent;
		}
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
	char buffer[BUFFER_SIZE_16K]="", bufferHTTP[BUFFER_SIZE_16K]="";
	do{
		numEvents=poll(pfds, 1, SOCKET_RECV_TIMEOUT_MS);
		if(numEvents==0){
			close(socketConn);
			return LIBGPT_SOCKET_RECV_TIMEOUT_ERROR;
		}
		pollinHappened = pfds[0].revents & POLLIN;
		if (pollinHappened){
			bytesReceived=SSL_read(sslConn,buffer, BUFFER_SIZE_16K);
			if(bytesReceived>0){
				totalBytesReceived+=bytesReceived;
				for(int i=0; i<bytesReceived && cont < BUFFER_SIZE_16K; i++, contI++) bufferHTTP[contI]=buffer[i];
				if(cont>=BUFFER_SIZE_16K) return LIBGPT_BUFFERSIZE_OVERFLOW;
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
	cgptResponse->httpResponse=malloc(strlen(bufferHTTP)+1);
	snprintf(cgptResponse->httpResponse,strlen(bufferHTTP)+1,"%s",bufferHTTP);
	return libGPT_parse_result(cgptResponse, createContext);
}
