/*
 ============================================================================
 Name        : libGPT.h
 Author      : L. (lucho-a.github.io)
 Version     : 1.2.0
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
#define LIBGPT_MINOR_VERSION				"2"
#define LIBGPT_MICRO_VERSION				"0"
#define LIBGPT_VERSION						PROGRAM_MAJOR_VERSION"."PROGRAM_MINOR_VERSION"."PROGRAM_MICRO_VERSION
#define LIBGPT_DESCRIPTION					"C Library for ChatGPT"

#define	LIBGPT_DEFAULT_ROLE					""

#define	LIBGPT_MIN_MAX_TOKENS				10
#define	LIBGPT_DEFAULT_MAX_TOKENS			1024

#define	LIBGPT_MIN_FREQ_PENALTY				-2.0
#define	LIBGPT_DEFAULT_FREQ_PENALTY			0.0
#define	LIBGPT_MAX_FREQ_PENALTY				2.0

#define	LIBGPT_MIN_TEMPERATURE				0.0
#define	LIBGPT_DEFAULT_TEMPERATURE			0.5
#define	LIBGPT_MAX_TEMPERATURE				2.0

#define	LIBGPT_MIN_N						1
#define	LIBGPT_DEFAULT_N					1
#define	LIBGPT_MAX_N						20

#define	LIBGPT_MIN_CONTEXT_MSGS				0
#define	LIBGPT_DEFAULT_MAX_CONTEXT_MSGS		3

#define	LIBGPT_PROMPT_COST					0.0015
#define	LIBGPT_COMPLETION_COST				0.002

#define DBG									dbg("WTFFF?!?!");

typedef enum{
	FALSE=0,
	TRUE
}bool;

enum errors{
	LIBGPT_INIT_ERROR=-50,
	LIBGPT_MALLOC_ERROR,
	LIBGPT_REALLOC_ERROR,
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
	LIBGPT_ZEROBYTESRECV_ERROR,
	LIBGPT_OPENING_FILE_ERROR,
	LIBGPT_OPENING_ROLE_FILE_ERROR,
	LIBGPT_NO_HISTORY_CONTEXT_ERROR,
	LIBGPT_UNEXPECTED_JSON_FORMAT_ERROR,
	LIBGPT_FREQ_PENALTY_ERROR,
	LIBGPT_TEMPERATURE_ERROR,
	LIBGPT_N_ERROR,
	LIBGPT_MAX_TOKENS_ERROR,
	LIBGPT_CONTEXT_MSGS_ERROR
};

typedef struct ChatGPT{
	char *apiKey;
	char *systemRole;
	long int maxTokens;
	double frequencyPenalty;
	double temperature;
	int n;
}ChatGPT;

typedef struct ChatGPTResponse{
	long created;
	char *httpResponse;
	char *content;
	char *contentFormatted;
	char *finishReason;
	unsigned int promptTokens;
	unsigned int completionTokens;
	unsigned int totalTokens;
	double cost;
}ChatGPTResponse;

int libGPT_init(ChatGPT *, char *, char *, char *, long int, double, double, int, int);
int libGPT_get_service_status(char **);
int libGPT_send_chat(ChatGPT,ChatGPTResponse *, char *);
int libGPT_flush_history(void);
int libGPT_import_session_file(char *);
int libGPT_export_session_file(char *);
int libGPT_save_message(char *, bool);
int libGPT_clean(ChatGPT *);
int libGPT_clean_response(ChatGPTResponse *);
char * libGPT_error(int);

void dbg(char *);

#endif /* HEADERS_LIBGPT_H_ */
