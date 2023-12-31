/*
 ============================================================================
 Name        : libGPT.h
 Author      : L. (lucho-a.github.io)
 Version     : 1.2.3
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
#define LIBGPT_MICRO_VERSION				"3"
#define LIBGPT_VERSION						PROGRAM_MAJOR_VERSION"."PROGRAM_MINOR_VERSION"."PROGRAM_MICRO_VERSION
#define LIBGPT_DESCRIPTION					"C Library for ChatGPT"

#define LIBGPT_DEFAULT_MODEL				"gpt-3.5-turbo"
#define	LIBGPT_DEFAULT_ROLE					""
#define	LIBGPT_DEFAULT_MAX_TOKENS			1024
#define	LIBGPT_DEFAULT_FREQ_PENALTY			0.0
#define	LIBGPT_DEFAULT_PRES_PENALTY			0.0
#define	LIBGPT_DEFAULT_TEMPERATURE			0.5
#define	LIBGPT_DEFAULT_TOP_P				1.0
#define	LIBGPT_DEFAULT_N					1

#define	LIBGPT_MIN_CONTEXT_MSGS				0
#define	LIBGPT_DEFAULT_MAX_CONTEXT_MSGS		3

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
	LIBGPT_RECV_TIMEOUT_ERROR,
	LIBGPT_RECEIVING_PACKETS_ERROR,
	LIBGPT_RESPONSE_MESSAGE_ERROR,
	LIBGPT_ZEROBYTESRECV_ERROR,
	LIBGPT_OPENING_FILE_ERROR,
	LIBGPT_OPENING_ROLE_FILE_ERROR,
	LIBGPT_NO_HISTORY_CONTEXT_ERROR,
	LIBGPT_UNEXPECTED_JSON_FORMAT_ERROR,
	LIBGPT_CONTEXT_MSGS_ERROR,
	LIBGPT_NULL_STRUCT_ERROR
};

typedef struct _chatgpt ChatGPT;
typedef struct _chatgptresponse ChatGPTResponse;

ChatGPT * libGPT_getChatGPT_instance();
int libGPT_init();
int libGPT_set_timeout(long int);
int libGPT_set_max_context_msgs(long int);

int libGPT_set_model(ChatGPT *, char *);
int libGPT_set_api(ChatGPT *, char *);
int libGPT_set_max_tokens(ChatGPT *, long int);
int libGPT_set_n(ChatGPT *, int);
int libGPT_set_frequency_penalty(ChatGPT *, double);
int libGPT_set_presence_penalty(ChatGPT *, double);
int libGPT_set_temperature(ChatGPT *, double);
int libGPT_set_top_p(ChatGPT *, double);
int libGPT_set_role(ChatGPT *, char *, char *);

char * libGPT_get_model(ChatGPT *);
long int libGPT_get_max_tokens(ChatGPT *);
int libGPT_get_n(ChatGPT *);
double libGPT_get_frequency_penalty(ChatGPT *);
double libGPT_get_presence_penalty(ChatGPT *);
double libGPT_get_temperature(ChatGPT *);
double libGPT_get_top_p(ChatGPT *);

int libGPT_get_service_status(char **);
int libGPT_send_chat(ChatGPT *,ChatGPTResponse *, char *);
int libGPT_flush_history(void);
int libGPT_import_session_file(char *);
int libGPT_export_session_file(char *);
int libGPT_save_message(char *, bool);
int libGPT_free(ChatGPT *);

char * libGPT_error(int);

ChatGPTResponse * libGPT_getChatGPTResponse_instance();
long libGPT_get_response_creation(ChatGPTResponse *);
unsigned int libGPT_get_response_responses(ChatGPTResponse *);
char * libGPT_get_response_model(ChatGPTResponse *);
char * libGPT_get_response_contentFormatted(ChatGPTResponse *, int);
char * libGPT_get_response_finishReason(ChatGPTResponse *, int);
unsigned int libGPT_get_response_promptTokens(ChatGPTResponse *);
unsigned int libGPT_get_response_completionTokens(ChatGPTResponse *);
unsigned int libGPT_get_response_totalTokens(ChatGPTResponse *);
int libGPT_clean_response(ChatGPTResponse *);
int libGPT_free_response(ChatGPTResponse *);

void dbg(char *);

#endif /* HEADERS_LIBGPT_H_ */
