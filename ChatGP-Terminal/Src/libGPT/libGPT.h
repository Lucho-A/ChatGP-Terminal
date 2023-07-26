/*
 ============================================================================
 Name        : libGPT.h
 Author      : L. (lucho-a.github.io)
 Version     : 1.0.2
 Created on	 : 2023/07/18
 Copyright   : GNU General Public License v3.0
 Description : Header file
 ============================================================================
*/

#ifndef HEADERS_LIBGPT_H_
#define HEADERS_LIBGPT_H_

#define RETURN_ERROR 						-1
#define RETURN_OK 							0

#define LIBGPT_NAME 						"libGPT"
#define LIBGPT_MAJOR_VERSION				"1"
#define LIBGPT_MINOR_VERSION				"0"
#define LIBGPT_MICRO_VERSION				"2"
#define LIBGPT_VERSION						PROGRAM_MAJOR_VERSION"."PROGRAM_MINOR_VERSION"."PROGRAM_MICRO_VERSION
#define LIBGPT_DESCRIPTION					"C Library for ChatGPT"

#define	LIBGPT_DEFAULT_ROLE					""
#define	LIBGPT_DEFAULT_MAX_TOKENS			512
#define	LIBGPT_DEFAULT_TEMPERATURE			0.5
#define	LIBGPT_DEFAULT_MAX_CONTEXT_MSGS		3

#define PRINT_DBG							printf("\n\e[0;31mWTFFFF\e[0m\n");

typedef enum{
	FALSE=0,
	TRUE
}bool;

enum errors{
	LIBGPT_INIT_ERROR=-50,
	LIBGPT_GETTING_HOST_INFO_ERROR,
	LIBGPT_SOCKET_CREATION_ERROR,
	LIBGPT_SOCKET_CONNECTION_TIMEOUT_ERROR,
	LIBGPT_SSL_CONTEXT_ERROR,
	LIBGPT_SSL_FD_ERROR,
	LIBGPT_SSL_CONNECT_ERROR,
	LIBGPT_SOCKET_SEND_TIMEOUT_ERROR,
	LIBGPT_SENDING_PACKETS_ERROR,
	LIBGPT_POLLIN_ERROR,
	LIBGPT_SOCKET_RECV_TIMEOUT_ERROR,
	LIBGPT_RECEIVING_PACKETS_ERROR,
	LIBGPT_RESPONSE_MESSAGE_ERROR,
	LIBGPT_BUFFERSIZE_OVERFLOW,
	LIBGPT_ZEROBYTESRECV_ERROR
};

typedef struct ChatGPT{
	char *api;
	char *systemRole;
	long int maxTokens;
	double temperature;
}ChatGPT;

typedef struct ChatGPTResponse{
	char *httpResponse;
	char *message;
	char *finishReason;
	unsigned int promptTokens;
	unsigned int completionTokens;
	unsigned int totalTokens;
}ChatGPTResponse;

int libGPT_init(ChatGPT *, char *, char *, long int, double, int);
int libGPT_send_chat(ChatGPT,ChatGPTResponse *, char *, bool);
void libGPT_flush_history(void);
int libGPT_clean(ChatGPT *);
int libGPT_clean_response(ChatGPTResponse *);

#endif /* HEADERS_LIBGPT_H_ */
