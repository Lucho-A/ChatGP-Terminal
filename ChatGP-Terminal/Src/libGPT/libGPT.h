/*
 ============================================================================
 Name        : libGPT.h
 Author      : L. (lucho-a.github.io)
 Version     : 1.0.0
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
#define LIBGPT_MICRO_VERSION				"1"
#define LIBGPT_VERSION						PROGRAM_MAJOR_VERSION"."PROGRAM_MINOR_VERSION"."PROGRAM_MICRO_VERSION
#define LIBGPT_DESCRIPTION					"C Library for ChatGPT"

#define LIBGPT_OPENAI_API_URL				"api.openai.com"

#define LIBGPT_SOCKET_CONNECT_TIMEOUT_S		5
#define LIBGPT_SOCKET_SEND_TIMEOUT_MS		5000
#define LIBGPT_SOCKET_RECV_TIMEOUT_MS		60000

#define	BUFFER_SIZE_512B					512
#define	BUFFER_SIZE_16K						(1024*16)
#define	BUFFER_SIZE_JSON_MESSAGE			(1024*16)
#define	BUFFER_SIZE_MESSAGE					(1024*16)
#define	BUFFER_SIZE_FINISHED_REASON			16
#define	BUFFER_SIZE_TOTAL_TOKENS			8

#define	LIBGPT_DEFAULT_ROLE					""
#define	LIBGPT_DEFAULT_MAX_TOKENS			256
#define	LIBGPT_DEFAULT_TEMPERATURE			0.5
#define	LIBGPT_DEFAULT_RESPONSE_VELOCITY	100000

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

//TODO Dynamically memory allocation
typedef struct ChatGPTResponse{
	char jsonMessage[BUFFER_SIZE_JSON_MESSAGE];
	char message[BUFFER_SIZE_MESSAGE];
	char finishReason[BUFFER_SIZE_FINISHED_REASON];
	char totalTokens[BUFFER_SIZE_TOTAL_TOKENS];
}ChatGPTResponse;

int libGPT_init(ChatGPT *, char *, char *, long int, double);
int libGPT_clean(ChatGPT *);
int libGPT_send_chat(ChatGPT,ChatGPTResponse *, char *);

#endif /* HEADERS_LIBGPT_H_ */
