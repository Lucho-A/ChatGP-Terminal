/*
 ============================================================================
 Name        : libGPT.c
 Author      : L. (lucho-a.github.io)
 Version     : 1.1.3
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

#define MAX_ROLES_SIZE						(1024*128)
#define BUFFER_SIZE_16K						(1024*16)

typedef struct Messages{
	char *userMessage;
	char *assistantMessage;
	struct Messages *nextMessage;
}Messages;

static Messages *historyContext=NULL;
static int contHistoryContext=0;
static int maxHistoryContext=0;

int format_string(char *text, char **result){
	*result=malloc(strlen(text)+1);
	memset(*result,0,strlen(text)+1);
	int cont=0;
	for(int i=0;text[i]!=0;i++,cont++){
		if(text[i]=='\\'){
			switch(text[i+1]){
			case 'n':
				(*result)[cont]='\n';
				break;
			case 'r':
				(*result)[cont]='\r';
				break;
			case 't':
				(*result)[cont]='\t';
				break;
			case '\\':
				(*result)[cont]='\\';
				break;
			case '"':
				(*result)[cont]='\"';
				break;
			default:
				break;
			}
			i++;
			continue;
		}
		(*result)[cont]=text[i];
	}
	(*result)[cont]=0;
	return RETURN_OK;
}

int get_string_from_token(char *text, char *token, char *result, char endChar){
	int cont=0;
	char *message=strstr(text,token);
	for(int i=strlen(token);(message[i-1]=='\\' || message[i]!=endChar) && cont<BUFFER_SIZE_16K;i++,cont++) result[cont]=message[i];
	result[cont]='\0';
	if(cont>=BUFFER_SIZE_16K) return LIBGPT_BUFFERSIZE_OVERFLOW;
	return RETURN_OK;
}
void create_new_history_message(char *userMessage, char *assistantMessage){
	Messages *newMessage=malloc(sizeof(Messages));
	newMessage->userMessage=malloc(strlen(userMessage)+1);
	snprintf(newMessage->userMessage,strlen(userMessage)+1,"%s",userMessage);
	free(userMessage);
	newMessage->assistantMessage=malloc(strlen(assistantMessage)+1);
	snprintf(newMessage->assistantMessage,strlen(assistantMessage)+1,"%s",assistantMessage);
	if(historyContext!=NULL){
		if(contHistoryContext>=maxHistoryContext){
			Messages *temp=historyContext->nextMessage;
			if(historyContext->userMessage!=NULL) free(historyContext->userMessage);
			if(historyContext->assistantMessage!=NULL) free(historyContext->assistantMessage);
			free(historyContext);
			historyContext=temp;
		}
		Messages *temp=historyContext;
		if(temp!=NULL){
			while(temp->nextMessage!=NULL) temp=temp->nextMessage;
			temp->nextMessage=newMessage;
		}else{
			historyContext=newMessage;
		}
	}else{
		historyContext=newMessage;
	}
	newMessage->nextMessage=NULL;
	contHistoryContext++;
}

static int parse_result(char *messageSent, ChatGPTResponse *cgptResponse){
	char buffer[BUFFER_SIZE_16K]="";
	if(strstr(cgptResponse->httpResponse,"\"error\": {")!=NULL){
		if(get_string_from_token(cgptResponse->httpResponse,"\"message\": \"" ,buffer,'\"')==LIBGPT_BUFFERSIZE_OVERFLOW) return LIBGPT_BUFFERSIZE_OVERFLOW;
		cgptResponse->content=malloc(strlen(buffer)+1);
		snprintf(cgptResponse->content,strlen(buffer)+1,"%s", buffer);
		cgptResponse->contentFormatted=malloc(strlen(buffer)+1);
		format_string(buffer, &cgptResponse->contentFormatted);
		return LIBGPT_RESPONSE_MESSAGE_ERROR;
	}

	if(get_string_from_token(cgptResponse->httpResponse,"\"created\": ",buffer,'\n')==LIBGPT_BUFFERSIZE_OVERFLOW) return LIBGPT_BUFFERSIZE_OVERFLOW;
	cgptResponse->created=strtol(buffer,NULL,10);

	if(get_string_from_token(cgptResponse->httpResponse,"\"content\": \"",buffer,'\"')==LIBGPT_BUFFERSIZE_OVERFLOW) return LIBGPT_BUFFERSIZE_OVERFLOW;
	cgptResponse->content=malloc(strlen(buffer)+1);
	snprintf(cgptResponse->content,strlen(buffer)+1,"%s", buffer);
	cgptResponse->contentFormatted=malloc(strlen(buffer)+1);
	format_string(buffer, &cgptResponse->contentFormatted);

	if(maxHistoryContext>0) create_new_history_message(messageSent, cgptResponse->content);

	if(get_string_from_token(cgptResponse->httpResponse,"\"finish_reason\": \"",buffer,'\"')==LIBGPT_BUFFERSIZE_OVERFLOW) return LIBGPT_BUFFERSIZE_OVERFLOW;
	cgptResponse->finishReason=malloc(strlen(buffer)+1);
	snprintf(cgptResponse->finishReason,strlen(buffer)+1,"%s", buffer);

	if(get_string_from_token(cgptResponse->httpResponse,"\"prompt_tokens\": ",buffer,'\n')==LIBGPT_BUFFERSIZE_OVERFLOW) return LIBGPT_BUFFERSIZE_OVERFLOW;
	cgptResponse->promptTokens=strtol(buffer,NULL,10);

	if(get_string_from_token(cgptResponse->httpResponse,"\"completion_tokens\": ",buffer,'\n')==LIBGPT_BUFFERSIZE_OVERFLOW) return LIBGPT_BUFFERSIZE_OVERFLOW;
	cgptResponse->completionTokens=strtol(buffer,NULL,10);

	if(get_string_from_token(cgptResponse->httpResponse,"\"total_tokens\": ",buffer,'\n')==LIBGPT_BUFFERSIZE_OVERFLOW) return LIBGPT_BUFFERSIZE_OVERFLOW;
	cgptResponse->totalTokens=strtol(buffer,NULL,10);

	cgptResponse->cost=(cgptResponse->promptTokens/1000.0) * 0.0015 + (cgptResponse->completionTokens/1000.0) * 0.002;

	return RETURN_OK;
}

int libGPT_export_session_file(char *exportSessionFile){
	FILE *f=fopen(exportSessionFile,"w");
	if(f==NULL) return LIBGPT_OPENING_FILE_ERROR;
	Messages *temp=historyContext;
	while(temp!=NULL){
		fprintf(f,"%s\t%s\n",temp->userMessage,temp->assistantMessage);
		temp=temp->nextMessage;
	}
	fclose(f);
	free(temp);
	return RETURN_OK;
}

int libGPT_import_session_file(char *importSessionFile){
	FILE *f=fopen(importSessionFile,"r");
	if(f==NULL) return LIBGPT_OPENING_FILE_ERROR;
	libGPT_flush_history();
	size_t len=0, i=0, rows=0, initPos=0;
	ssize_t chars=0;
	char *line=NULL, *userMessage=NULL,*assistantMessage=NULL;
	while((getline(&line, &len, f))!=-1) rows++;
	if(rows>maxHistoryContext) initPos=rows-maxHistoryContext;
	int contRows=0;
	rewind(f);
	while((chars=getline(&line, &len, f))!=-1){
		if(contRows>=initPos){
			userMessage=malloc(chars);
			memset(userMessage,0,chars);
			for(i=0;line[i]!='\t';i++) userMessage[i]=line[i];
			int index=0;
			assistantMessage=malloc(chars);
			memset(assistantMessage,0,chars);
			for(i++;line[i]!='\n';i++,index++) assistantMessage[index]=line[i];
			create_new_history_message(userMessage, assistantMessage);
		}
		contRows++;
	}
	free(assistantMessage);
	free(line);
	fclose(f);
	return RETURN_OK;
}


int libGPT_save_message(char *saveMessagesTo, bool csvFormat){
	if(historyContext==NULL) return LIBGPT_NO_HISTORY_CONTEXT_ERROR;
	FILE *f=fopen(saveMessagesTo,"a");
	if(f==NULL) return LIBGPT_OPENING_FILE_ERROR;
	time_t timestamp = time(NULL);
	struct tm tm = *localtime(&timestamp);
	Messages *temp=historyContext;
	while(temp->nextMessage!=NULL) temp=temp->nextMessage;
	if(csvFormat){
		fprintf(f,"\"%d/%02d/%02d\"\t\"%02d:%02d:%02d\"\t",tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		fprintf(f,"\"%s\"\t",temp->userMessage);
		fprintf(f,"\"%s\"\n",temp->assistantMessage);
	}else{
		char strTimeStamp[50]="";
		snprintf(strTimeStamp,sizeof(strTimeStamp),"%d/%02d/%02d %02d:%02d:%02d UTC:%s",tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_zone);
		fprintf(f,"\n%s\n",strTimeStamp);
		char *buffer=NULL;
		format_string(temp->userMessage,&buffer);
		fprintf(f,"User: %s\n",buffer);
		free(buffer);
		format_string(temp->assistantMessage,&buffer);
		fprintf(f,"Assistant: %s\n",buffer);
		free(buffer);
	}
	fclose(f);
	return RETURN_OK;
}

int libGPT_flush_history(void){
	while(historyContext!=NULL){
		Messages *temp=historyContext;
		historyContext=temp->nextMessage;
		if(temp->userMessage!=NULL) free(temp->userMessage);
		if(temp->assistantMessage!=NULL) free(temp->assistantMessage);
		free(temp);
	}
	contHistoryContext=0;
	return RETURN_OK;
}

int libGPT_clean(ChatGPT *cgpt){
	if(cgpt->api!=NULL) free(cgpt->api);
	if(cgpt->systemRole!=NULL) free(cgpt->systemRole);
	libGPT_flush_history();
	return RETURN_OK;
}

int libGPT_clean_response(ChatGPTResponse *cgptResponse){
	if(cgptResponse->httpResponse!=NULL) free(cgptResponse->httpResponse);
	if(cgptResponse->content!=NULL) free(cgptResponse->content);
	if(cgptResponse->contentFormatted!=NULL) free(cgptResponse->contentFormatted);
	if(cgptResponse->finishReason!=NULL) free(cgptResponse->finishReason);
	return RETURN_OK;
}

int libGPT_init(ChatGPT *cgtp, char *api, char *systemRole, long int maxTokens, double temperature, int maxContextMessage){
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
	maxHistoryContext=maxContextMessage;
	return RETURN_OK;
}

int libGPT_send_chat(ChatGPT cgpt, ChatGPTResponse *cgptResponse, char *message){
	cgptResponse->created=0;
	cgptResponse->httpResponse=NULL;
	cgptResponse->content=NULL;
	cgptResponse->contentFormatted=NULL;
	cgptResponse->finishReason=NULL;
	cgptResponse->promptTokens=0;
	cgptResponse->completionTokens=0;
	cgptResponse->totalTokens=0;
	cgptResponse->cost=0.0;
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
		case '\t':
			messageParsed[cont]='\\';
			messageParsed[++cont]='t';
			break;
		case '\r':
			messageParsed[cont]='\\';
			messageParsed[++cont]='r';
			break;
		default:
			messageParsed[cont]=message[i];
		}
	}
	messageParsed[cont]='\0';
	char roles[MAX_ROLES_SIZE]="", buf[BUFFER_SIZE_16K]="";
	Messages *temp=historyContext;
	while(temp!=NULL){
		snprintf(buf,BUFFER_SIZE_16K,"{\"role\":\"user\",\"content\":\"%s\"},",temp->userMessage);
		strcat(roles,buf);
		snprintf(buf,BUFFER_SIZE_16K,"{\"role\":\"assistant\",\"content\":\"%s\"},",temp->assistantMessage);
		strcat(roles,buf);
		temp=temp->nextMessage;
	}
	snprintf(buf,BUFFER_SIZE_16K,"{\"role\":\"user\",\"content\":\"%s\"}",messageParsed);
	strcat(roles,buf);
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
	memset(&hints, 0, sizeof(hints));
	hints.ai_family=AF_INET;
	hints.ai_socktype=SOCK_STREAM;
	if(getaddrinfo(OPENAI_API_URL, NULL, &hints, &res)!=0) return LIBGPT_GETTING_HOST_INFO_ERROR;
	struct sockaddr_in *ipv4=(struct sockaddr_in *)res->ai_addr;
	void *addr=&(ipv4->sin_addr);
	char chatGptIp[INET_ADDRSTRLEN];
	inet_ntop(res->ai_family, addr, chatGptIp, sizeof(chatGptIp));
	freeaddrinfo(res);
	struct sockaddr_in serverAddress;
	serverAddress.sin_family=AF_INET;
	serverAddress.sin_port=htons(OPENAI_API_PORT);
	serverAddress.sin_addr.s_addr=inet_addr(chatGptIp);
	if((socketConn=socket(AF_INET, SOCK_STREAM, 0))<0) return LIBGPT_SOCKET_CREATION_ERROR;
	int socketFlags=fcntl(socketConn, F_GETFL, 0);
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
	return parse_result(messageParsed, cgptResponse);
}
