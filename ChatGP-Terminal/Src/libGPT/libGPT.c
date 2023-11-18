/*
 ============================================================================
 Name        : libGPT.c
 Author      : L. (lucho-a.github.io)
 Version     : 1.2.1
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

#define OPENAI_STATUS_URL					"status.openai.com"
#define OPENAI_API_URL						"api.openai.com"
#define OPENAI_API_PORT						443

#define SOCKET_CONNECT_TIMEOUT_S			5
#define SOCKET_SEND_TIMEOUT_MS				5000
#define SOCKET_RECV_TIMEOUT_MS				60000

#define BUFFER_SIZE_16K						(1024*16)

typedef struct Messages{
	char *userMessage;
	char *assistantMessage;
	struct Messages *nextMessage;
}Messages;

static Messages *historyContext=NULL;
static int contHistoryContext=0;
static int maxHistoryContext=0;

static int parse_string(char **stringTo, char *stringFrom){
	int cont=0, contEsc=0;
	for(int i=0;i<strlen(stringFrom);i++){
		switch(stringFrom[i]){
		case '\"':
		case '\\':
		case '\n':
		case '\t':
		case '\r':
			contEsc++;
			break;
		default:
			break;
		}
	}
	*stringTo=malloc(strlen(stringFrom)+contEsc+1);
	memset(*stringTo,0,strlen(stringFrom)+contEsc+1);
	for(int i=0;i<strlen(stringFrom);i++,cont++){
		switch(stringFrom[i]){
		case '\"':
		case '\\':
			(*stringTo)[cont]='\\';
			(*stringTo)[++cont]=stringFrom[i];
			break;
		case '\n':
			(*stringTo)[cont]='\\';
			(*stringTo)[++cont]='n';
			break;
		case '\t':
			(*stringTo)[cont]='\\';
			(*stringTo)[++cont]='t';
			break;
		case '\r':
			(*stringTo)[cont]='\\';
			(*stringTo)[++cont]='r';
			break;
		default:
			(*stringTo)[cont]=stringFrom[i];
			break;
		}
	}
	(*stringTo)[cont]='\0';
	return RETURN_OK;
}

static int format_string(char **stringTo, char *stringFrom){
	*stringTo=malloc(strlen(stringFrom)+1);
	memset(*stringTo,0,strlen(stringFrom)+1);
	int cont=0;
	for(int i=0;stringFrom[i]!=0;i++,cont++){
		if(stringFrom[i]=='\\'){
			switch(stringFrom[i+1]){
			case 'n':
				(*stringTo)[cont]='\n';
				break;
			case 'r':
				(*stringTo)[cont]='\r';
				break;
			case 't':
				(*stringTo)[cont]='\t';
				break;
			case '\\':
				(*stringTo)[cont]='\\';
				break;
			case '"':
				(*stringTo)[cont]='\"';
				break;
			default:
				break;
			}
			i++;
			continue;
		}
		(*stringTo)[cont]=stringFrom[i];
	}
	(*stringTo)[cont]=0;
	return RETURN_OK;
}

static int get_string_from_token(char *text, char *token, char **result, char endChar){
	ssize_t cont=0;
	char *message=strstr(text,token);
	if(message==NULL) return RETURN_ERROR;
	for(int i=strlen(token);(message[i-1]=='\\' || message[i]!=endChar);i++,cont++);
	*result=malloc(cont+1);
	memset(*result,0,cont+1);
	cont=0;
	for(int i=strlen(token);(message[i-1]=='\\' || message[i]!=endChar);i++,cont++) (*result)[cont]=message[i];
	return RETURN_OK;
}

static void create_new_history_message(char *userMessage, char *assistantMessage){
	Messages *newMessage=malloc(sizeof(Messages));
	newMessage->userMessage=malloc(strlen(userMessage)+1);
	snprintf(newMessage->userMessage,strlen(userMessage)+1,"%s",userMessage);
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
	char *buffer=NULL;
	if(strstr(cgptResponse->httpResponse,"\"error\": {")!=NULL){
		get_string_from_token(cgptResponse->httpResponse,"\"message\": \"" ,&buffer,'\"');
		cgptResponse->content=malloc(strlen(buffer)+1);
		memset(cgptResponse->content,0,strlen(buffer)+1);
		snprintf(cgptResponse->content,strlen(buffer)+1,"%s", buffer);
		cgptResponse->contentFormatted=malloc(strlen(buffer)+1);
		memset(cgptResponse->contentFormatted,0,strlen(buffer)+1);
		format_string(&cgptResponse->contentFormatted, buffer);
		free(messageSent);
		return LIBGPT_RESPONSE_MESSAGE_ERROR;
	}
	free(buffer);

	if(get_string_from_token(cgptResponse->httpResponse,"\"model\": \"",&buffer,'\"')==RETURN_ERROR) return LIBGPT_UNEXPECTED_JSON_FORMAT_ERROR;
	cgptResponse->model=malloc(strlen(buffer)+1);
	memset(cgptResponse->model,0,strlen(buffer)+1);
	snprintf(cgptResponse->model,strlen(buffer)+1,"%s", buffer);
	free(buffer);

	if(get_string_from_token(cgptResponse->httpResponse,"\"created\": ",&buffer,'\n')==RETURN_ERROR) return LIBGPT_UNEXPECTED_JSON_FORMAT_ERROR;
	cgptResponse->created=strtol(buffer,NULL,10);
	free(buffer);

	if(get_string_from_token(cgptResponse->httpResponse,"\"content\": \"",&buffer,'\"')==RETURN_ERROR) return LIBGPT_UNEXPECTED_JSON_FORMAT_ERROR;;
	cgptResponse->content=malloc(strlen(buffer)+1);
	memset(cgptResponse->content,0,strlen(buffer)+1);
	snprintf(cgptResponse->content,strlen(buffer)+1,"%s", buffer);
	format_string(&cgptResponse->contentFormatted, buffer);
	if(maxHistoryContext>0) create_new_history_message(messageSent, cgptResponse->content);
	free(messageSent);
	free(buffer);

	if(get_string_from_token(cgptResponse->httpResponse,"\"finish_reason\": \"",&buffer,'\"')==RETURN_ERROR) return LIBGPT_UNEXPECTED_JSON_FORMAT_ERROR;;
	cgptResponse->finishReason=malloc(strlen(buffer)+1);
	memset(cgptResponse->finishReason,0,strlen(buffer)+1);
	snprintf(cgptResponse->finishReason,strlen(buffer)+1,"%s", buffer);
	free(buffer);

	if(get_string_from_token(cgptResponse->httpResponse,"\"prompt_tokens\": ",&buffer,'\n')==RETURN_ERROR) return LIBGPT_UNEXPECTED_JSON_FORMAT_ERROR;;
	cgptResponse->promptTokens=strtol(buffer,NULL,10);
	free(buffer);

	if(get_string_from_token(cgptResponse->httpResponse,"\"completion_tokens\": ",&buffer,'\n')==RETURN_ERROR) return LIBGPT_UNEXPECTED_JSON_FORMAT_ERROR;;
	cgptResponse->completionTokens=strtol(buffer,NULL,10);
	free(buffer);

	if(get_string_from_token(cgptResponse->httpResponse,"\"total_tokens\": ",&buffer,'\n')==RETURN_ERROR) return LIBGPT_UNEXPECTED_JSON_FORMAT_ERROR;;
	cgptResponse->totalTokens=strtol(buffer,NULL,10);
	free(buffer);

	//cgptResponse->cost=(cgptResponse->promptTokens/1000.0) * LIBGPT_PROMPT_COST + (cgptResponse->completionTokens/1000.0) * LIBGPT_COMPLETION_COST;

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

int libGPT_import_session_file(char *sessionFile){
	FILE *f=fopen(sessionFile,"r");
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
			userMessage=malloc(chars+1);
			memset(userMessage,0,chars+1);
			for(i=0;line[i]!='\t';i++) userMessage[i]=line[i];
			int index=0;
			assistantMessage=malloc(chars);
			memset(assistantMessage,0,chars);
			for(i++;line[i]!='\n';i++,index++) assistantMessage[index]=line[i];
			create_new_history_message(userMessage, assistantMessage);
			free(userMessage);
			free(assistantMessage);
		}
		contRows++;
	}
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
		format_string(&buffer,temp->userMessage);
		fprintf(f,"User: %s\n",buffer);
		free(buffer);
		format_string(&buffer,temp->assistantMessage);
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

char * libGPT_error(int error){
	static char libGPTError[1024]="";
	switch(error){
	case LIBGPT_MALLOC_ERROR:
		snprintf(libGPTError, 1024,"Malloc() error. %s", strerror(errno));
		break;
	case LIBGPT_REALLOC_ERROR:
		snprintf(libGPTError, 1024,"Realloc() error. %s", strerror(errno));
		break;
	case LIBGPT_GETTING_HOST_INFO_ERROR:
		snprintf(libGPTError, 1024,"Error getting host info. %s", strerror(errno));
		break;
	case LIBGPT_SOCKET_CREATION_ERROR:
		snprintf(libGPTError, 1024,"Error creating socket. %s", strerror(errno));
		break;
	case LIBGPT_SOCKET_CONNECTION_TIMEOUT_ERROR:
		snprintf(libGPTError, 1024,"Socket connection time out. ");
		break;
	case LIBGPT_SSL_CONTEXT_ERROR:
		snprintf(libGPTError, 1024,"Error creating SSL context. %s", strerror(errno));
		break;
	case LIBGPT_SSL_FD_ERROR:
		snprintf(libGPTError, 1024,"SSL fd error. %s", strerror(errno));
		break;
	case LIBGPT_SSL_CONNECT_ERROR:
		snprintf(libGPTError, 1024,"SSL Connection error. %s", strerror(errno));
		break;
	case LIBGPT_SOCKET_SEND_TIMEOUT_ERROR:
		snprintf(libGPTError, 1024,"Sending packet time out. ");
		break;
	case LIBGPT_SENDING_PACKETS_ERROR:
		snprintf(libGPTError, 1024,"Sending packet error. ");
		break;
	case LIBGPT_POLLIN_ERROR:
		snprintf(libGPTError, 1024,"Pollin error. ");
		break;
	case LIBGPT_SOCKET_RECV_TIMEOUT_ERROR:
		snprintf(libGPTError, 1024,"Receiving packet time out. ");
		break;
	case LIBGPT_RECEIVING_PACKETS_ERROR:
		snprintf(libGPTError, 1024,"Receiving packet error. ");
		break;
	case LIBGPT_RESPONSE_MESSAGE_ERROR:
		snprintf(libGPTError, 1024,"Error message into JSON. ");
		break;
	case LIBGPT_ZEROBYTESRECV_ERROR:
		snprintf(libGPTError, 1024,"Zero bytes received. Try again...");
		break;
	case LIBGPT_OPENING_FILE_ERROR:
		snprintf(libGPTError, 1024,"Error opening file. %s", strerror(errno));
		break;
	case LIBGPT_OPENING_ROLE_FILE_ERROR:
		snprintf(libGPTError, 1024,"Error opening 'Role' file. %s", strerror(errno));
		break;
	case LIBGPT_NO_HISTORY_CONTEXT_ERROR:
		snprintf(libGPTError, 1024,"No message to save. ");
		break;
	case LIBGPT_UNEXPECTED_JSON_FORMAT_ERROR:
		snprintf(libGPTError, 1024,"Unexpected JSON format error. ");
		break;
	case LIBGPT_CONTEXT_MSGS_ERROR:
		snprintf(libGPTError, 1024,"'Max. Context Message' value out-of-boundaries. ");
		break;
	case LIBGPT_NULL_STRUCT_ERROR:
		snprintf(libGPTError, 1024,"ChatGPT structure null. ");
		break;
	default:
		snprintf(libGPTError, 1024,"Error not handled. ");
		break;
	}
	return libGPTError;
}

int libGPT_clean(ChatGPT *cgpt){
	if(cgpt->apiKey!=NULL) free(cgpt->apiKey);
	if(cgpt->systemRole!=NULL && cgpt->systemRole[0]!=0) free(cgpt->systemRole);
	libGPT_flush_history();
	return RETURN_OK;
}

int libGPT_clean_response(ChatGPTResponse *cgptResponse){
	if(cgptResponse->model!=NULL) free(cgptResponse->model);
	if(cgptResponse->httpResponse!=NULL) free(cgptResponse->httpResponse);
	if(cgptResponse->content!=NULL) free(cgptResponse->content);
	if(cgptResponse->contentFormatted!=NULL) free(cgptResponse->contentFormatted);
	if(cgptResponse->finishReason!=NULL) free(cgptResponse->finishReason);
	return RETURN_OK;
}

int libGPT_set_model(ChatGPT *cgpt, char *model){
	if(model==NULL) model="";
	cgpt->model=model;
	return RETURN_OK;
}

int libGPT_set_api(ChatGPT *cgpt, char *apiKey){
	if(apiKey==NULL) apiKey="";
	cgpt->apiKey=apiKey;
	return RETURN_OK;
}

int libGPT_set_max_tokens(ChatGPT *cgpt, long int maxTokens){
	cgpt->maxTokens=maxTokens;
	return RETURN_OK;
}

int libGPT_set_n(ChatGPT *cgpt, int n){
	cgpt->n=n;
	return RETURN_OK;
}

int libGPT_set_frequency_penalty(ChatGPT *cgpt, double fp){
	cgpt->frequencyPenalty=fp;
	return RETURN_OK;
}

int libGPT_set_presence_penalty(ChatGPT *cgpt, double pp){
	cgpt->presencePenalty=pp;
	return RETURN_OK;
}

int libGPT_set_temperature(ChatGPT *cgpt, double temperature){
	cgpt->temperature=temperature;
	return RETURN_OK;
}

int libGPT_init(ChatGPT *cgpt, char *model, char *api, char *systemRole, char *roleFile, long int maxTokens,
		double freqPenalty, double presPenalty, double temperature, int n, int maxContextMessage){
	if(cgpt==NULL) return LIBGPT_NULL_STRUCT_ERROR;
	SSL_library_init();
	if(maxContextMessage<LIBGPT_MIN_CONTEXT_MSGS) return LIBGPT_CONTEXT_MSGS_ERROR;
	int resp=0;
	if((resp=libGPT_set_model(cgpt,model))!=RETURN_OK) return resp;
	if((resp=libGPT_set_api(cgpt,api))!=RETURN_OK) return resp;
	if((resp=libGPT_set_max_tokens(cgpt,maxTokens))!=RETURN_OK) return resp;
	if((resp=libGPT_set_frequency_penalty(cgpt,freqPenalty))!=RETURN_OK) return resp;
	if((resp=libGPT_set_presence_penalty(cgpt,presPenalty))!=RETURN_OK) return resp;
	if((resp=libGPT_set_temperature(cgpt,temperature))!=RETURN_OK) return resp;
	if((resp=libGPT_set_n(cgpt,n))!=RETURN_OK) return resp;
	cgpt->model=malloc(strlen(model)+1);
	if(cgpt->model==NULL) return LIBGPT_MALLOC_ERROR;
	snprintf(cgpt->model,strlen(model)+1,"%s",model);
	cgpt->apiKey=malloc(strlen(api)+1);
	if(cgpt->apiKey==NULL) return LIBGPT_MALLOC_ERROR;
	snprintf(cgpt->apiKey,strlen(api)+1,"%s",api);
	if(roleFile!=NULL){
		FILE *f=fopen(roleFile,"r");
		if(f==NULL) return LIBGPT_OPENING_ROLE_FILE_ERROR;
		char *line=NULL;
		size_t len=0;
		ssize_t chars=0;
		char *buffer=malloc(1);
		memset(buffer,0,1);
		if(buffer==NULL) return LIBGPT_MALLOC_ERROR;
		if(systemRole!=NULL){
			buffer=realloc(buffer,strlen(buffer)+strlen(systemRole)+3);
			if(buffer==NULL) return LIBGPT_REALLOC_ERROR;
			snprintf(buffer,strlen(systemRole)+3,"%s. ",systemRole);
		}
		while((chars=getline(&line, &len, f))!=-1){
			buffer=realloc(buffer,strlen(buffer)+strlen(line)+1);
			if(buffer==NULL){
				free(line);
				free(buffer);
				return LIBGPT_REALLOC_ERROR;
			}
			strcat(buffer,line);
		}
		free(line);
		parse_string(&cgpt->systemRole, buffer);
		free(buffer);
		fclose(f);
	}else{
		if(systemRole!=NULL){
			cgpt->systemRole=malloc(strlen(systemRole)+2);
			if(cgpt->systemRole ==NULL) return LIBGPT_MALLOC_ERROR;
			snprintf(cgpt->systemRole,strlen(systemRole)+2,"%s.",systemRole);
		}else{
			cgpt->systemRole=LIBGPT_DEFAULT_ROLE;
		}
	}
	cgpt->maxTokens=maxTokens;
	cgpt->frequencyPenalty=freqPenalty;
	cgpt->temperature=temperature;
	cgpt->n=n;
	maxHistoryContext=maxContextMessage;
	return RETURN_OK;
}

static void clean_ssl(SSL *ssl){
	SSL_shutdown(ssl);
	SSL_certs_clear(ssl);
	SSL_clear(ssl);
	SSL_free(ssl);
}

static int get_http(char *url,char *payload, char **bufferHTTP){
	*bufferHTTP=malloc(1);
	memset(*bufferHTTP,0,1);
	struct pollfd pfds[1];
	int numEvents=0,pollinHappened=0,bytesSent=0;
	int socketConn=0;
	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family=AF_INET;
	hints.ai_socktype=SOCK_STREAM;
	if(getaddrinfo(url, NULL, &hints, &res)!=0) return LIBGPT_GETTING_HOST_INFO_ERROR;
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
	SSL *sslConn=NULL;
	SSL_CTX *sslCtx=NULL;
	if((sslCtx=SSL_CTX_new(SSLv23_method()))==NULL){
		SSL_CTX_free(sslCtx);
		return LIBGPT_SSL_CONTEXT_ERROR;
	}
	if((sslConn=SSL_new(sslCtx))==NULL){
		clean_ssl(sslConn);
		SSL_CTX_free(sslCtx);
		return LIBGPT_SSL_CONTEXT_ERROR;
	}
	if(!SSL_set_fd(sslConn, socketConn)){
		clean_ssl(sslConn);
		SSL_CTX_free(sslCtx);
		return LIBGPT_SSL_FD_ERROR;
	}
	SSL_set_connect_state(sslConn);
	SSL_set_tlsext_host_name(sslConn, url);
	if(!SSL_connect(sslConn)){
		clean_ssl(sslConn);
		SSL_CTX_free(sslCtx);
		return LIBGPT_SSL_CONNECT_ERROR;
	}
	fcntl(socketConn, F_SETFL, O_NONBLOCK);
	pfds[0].fd=socketConn;
	pfds[0].events=POLLOUT;
	numEvents=poll(pfds,1,SOCKET_SEND_TIMEOUT_MS);
	if(numEvents==0){
		close(socketConn);
		clean_ssl(sslConn);
		SSL_CTX_free(sslCtx);
		return LIBGPT_SOCKET_SEND_TIMEOUT_ERROR;
	}
	pollinHappened=pfds[0].revents & POLLOUT;
	if(pollinHappened){
		int totalBytesSent=0;
		while(totalBytesSent<strlen(payload)){
			bytesSent=SSL_write(sslConn, payload + totalBytesSent, strlen(payload) - totalBytesSent);
			totalBytesSent+=bytesSent;
		}
		if(bytesSent<=0){
			close(socketConn);
			clean_ssl(sslConn);
			SSL_CTX_free(sslCtx);
			return LIBGPT_SENDING_PACKETS_ERROR;
		}
	}else{
		close(socketConn);
		clean_ssl(sslConn);
		SSL_CTX_free(sslCtx);
		return LIBGPT_POLLIN_ERROR;
	}
	ssize_t bytesReceived=0,totalBytesReceived=0;
	pfds[0].events=POLLIN;
	char buffer[BUFFER_SIZE_16K]="";
	memset(buffer,0,BUFFER_SIZE_16K);
	do{
		numEvents=poll(pfds, 1, SOCKET_RECV_TIMEOUT_MS);
		if(numEvents==0){
			close(socketConn);
			clean_ssl(sslConn);
			SSL_CTX_free(sslCtx);
			return LIBGPT_SOCKET_RECV_TIMEOUT_ERROR;
		}
		pollinHappened = pfds[0].revents & POLLIN;
		if (pollinHappened){
			bytesReceived=SSL_read(sslConn,buffer, BUFFER_SIZE_16K);
			if(bytesReceived>0){
				totalBytesReceived+=bytesReceived;
				*bufferHTTP=realloc(*bufferHTTP, totalBytesReceived+1);
				if(*bufferHTTP==NULL){
					clean_ssl(sslConn);
					SSL_CTX_free(sslCtx);
					return LIBGPT_REALLOC_ERROR;
				}
				strcat(*bufferHTTP,buffer);
				memset(buffer,0,strlen(buffer));
				continue;
			}
			if(bytesReceived==0){
				close(socketConn);
				break;
			}
			if(bytesReceived<0 && (errno==EAGAIN || errno==EWOULDBLOCK)) continue;
			if(bytesReceived<0 && (errno!=EAGAIN)){
				close(socketConn);
				clean_ssl(sslConn);
				SSL_CTX_free(sslCtx);
				return LIBGPT_RECEIVING_PACKETS_ERROR;
			}
		}else{
			clean_ssl(sslConn);
			SSL_CTX_free(sslCtx);
			return LIBGPT_POLLIN_ERROR;
		}
	}while(TRUE);
	clean_ssl(sslConn);
	SSL_CTX_free(sslCtx);
	if(totalBytesReceived==0) return LIBGPT_ZEROBYTESRECV_ERROR;
	return totalBytesReceived;
}

int libGPT_get_service_status(char **statusDescription){
	char payload[1024]="";
	snprintf(payload,1024,"GET /api/v2/status.json HTTP/1.1\r\n"
			"Host: %s\r\n"
			"user-agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)\r\n"
			"content-type: application/json\r\n"
			"accept: */*\r\n"
			"connection: close\r\n\r\n",OPENAI_STATUS_URL);
	char *bufferHTTP=NULL;
	int resp=0;
	if((resp=get_http(OPENAI_STATUS_URL, payload,&bufferHTTP))<=0){
		free(bufferHTTP);
		return resp;
	}
	char *buf=strstr(bufferHTTP,"\"description\":\"");
	if(buf==NULL) return LIBGPT_UNEXPECTED_JSON_FORMAT_ERROR;
	*statusDescription=malloc(strlen(buf));
	memset(*statusDescription,0,strlen(buf));
	for(int i=strlen("\"description\":\"");buf[i]!='"';i++) (*statusDescription)[i-strlen("\"description\":\"")]=buf[i];
	free(bufferHTTP);
	return RETURN_OK;
}

int libGPT_send_chat(ChatGPT cgpt, ChatGPTResponse *cgptResponse, char *message){
	cgptResponse->model=NULL;
	cgptResponse->created=0;
	cgptResponse->httpResponse=NULL;
	cgptResponse->content=NULL;
	cgptResponse->contentFormatted=NULL;
	cgptResponse->finishReason=NULL;
	cgptResponse->promptTokens=0;
	cgptResponse->completionTokens=0;
	cgptResponse->totalTokens=0;
	char *messageParsed=NULL;
	parse_string(&messageParsed, message);
	char *context=malloc(1), *buf=NULL;
	memset(context,0,1);
	Messages *temp=historyContext;
	char *template="{\"role\":\"user\",\"content\":\"%s\"},{\"role\":\"assistant\",\"content\":\"%s\"},";
	ssize_t len=0;
	while(temp!=NULL){
		len=strlen(template)+strlen(temp->userMessage)+strlen(temp->assistantMessage);
		buf=malloc(len);
		if(buf==NULL){
			free(messageParsed);
			free(context);
			return LIBGPT_MALLOC_ERROR;
		}
		snprintf(buf,len,template,temp->userMessage,temp->assistantMessage);
		context=realloc(context, strlen(context)+strlen(buf)+1);
		if(context==NULL){
			free(messageParsed);
			free(context);
			free(buf);
			return LIBGPT_REALLOC_ERROR;
		}
		strcat(context,buf);
		temp=temp->nextMessage;
		free(buf);
	}
	template="{\"role\":\"user\",\"content\":\"%s\"}";
	len=strlen(template) + strlen(messageParsed);
	buf=malloc(len);
	if(buf==NULL){
		free(messageParsed);
		free(context);
		return LIBGPT_MALLOC_ERROR;
	}
	snprintf(buf,len,template,messageParsed);
	context=realloc(context, strlen(context)+strlen(buf)+1);
	if(context==NULL){
		free(messageParsed);
		return LIBGPT_REALLOC_ERROR;
	}
	strcat(context,buf);
	free(buf);
	template="{"
			"\"model\":\"%s\","
			"\"messages\":"
			"["
			"{\"role\":\"system\",\"content\":\"%s\"},"
			"%s"
			"],"
			"\"max_tokens\": %ld,"
			"\"n\": %d,"
			"\"frequency_penalty\": %.2f,"
			"\"presence_penalty\": %.2f,"
			"\"temperature\": %.2f"
			"}";
	len=strlen(template)+strlen(cgpt.model)+strlen(cgpt.systemRole)+strlen(context)+sizeof(cgpt.maxTokens)
				+sizeof(cgpt.n) + sizeof(cgpt.frequencyPenalty)+ sizeof(cgpt.presencePenalty)+sizeof(cgpt.temperature);
	char *payload=malloc(len);
	if(payload==NULL){
		free(messageParsed);
		return LIBGPT_MALLOC_ERROR;
	}
	snprintf(payload,len,template,cgpt.model,cgpt.systemRole,context,cgpt.maxTokens,
			cgpt.n,cgpt.frequencyPenalty,cgpt.presencePenalty,cgpt.temperature);
	free(context);
	template="POST /v1/chat/completions HTTP/1.1\r\n"
			"Host: %s\r\n"
			"user-agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)\r\n"
			"accept: */*\r\n"
			"content-type: application/json\r\n"
			"authorization: Bearer %s\r\n"
			"connection: close\r\n"
			"content-length: %ld\r\n\r\n"
			"%s";
	len=strlen(template)+strlen(OPENAI_API_URL)+strlen(cgpt.apiKey)+sizeof(size_t)+strlen(payload);
	char *httpMsg=malloc(len);
	if(httpMsg==NULL) return LIBGPT_MALLOC_ERROR;
	snprintf(httpMsg,len,template,OPENAI_API_URL,cgpt.apiKey,strlen(payload),payload);
	free(payload);
	char *bufferHTTP=NULL;
	int resp=get_http(OPENAI_API_URL, httpMsg,&bufferHTTP);
	free(httpMsg);
	if(resp<=0){
		free(messageParsed);
		free(bufferHTTP);
		return resp;
	}
	cgptResponse->httpResponse=malloc(strlen(bufferHTTP)+1);
	memset(cgptResponse->httpResponse,0,strlen(bufferHTTP)+1);
	snprintf(cgptResponse->httpResponse,strlen(bufferHTTP)+1,"%s",bufferHTTP);
	free(bufferHTTP);
	return parse_result(messageParsed, cgptResponse);
}

//TODO to be removed (for debugging/performance-evaluation)
void dbg(char *msg){
	time_t timestamp = time(NULL);
	struct tm tm = *localtime(&timestamp);
	char strTimeStamp[50]="";
	snprintf(strTimeStamp,sizeof(strTimeStamp),"%02d:%02d:%02d",tm.tm_hour, tm.tm_min, tm.tm_sec);
	printf("\n\e[0;31m%s: \n\n%s\e[0m\n",strTimeStamp,msg);
}





