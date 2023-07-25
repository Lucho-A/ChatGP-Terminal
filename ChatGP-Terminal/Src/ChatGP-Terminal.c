/*
 ============================================================================
 Name        : ChatGP-Terminal.c
 Author      : L. (lucho-a.github.io)
 Version     : 1.0.8
 Created on	 : 2023/07/18
 Copyright   : GNU General Public License v3.0
 Description : Main file
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>
#include <errno.h>

#include "libGPT/libGPT.h"

#define PROGRAM_NAME				"ChatGP-Terminal"
#define PROGRAM_VERSION				"1.0.8"
#define PROGRAM_URL					"https://github.com/lucho-a/chatgp-terminal"
#define PROGRAM_CONTACT				"<https://lucho-a.github.io/>"

#define C_HRED 						"\e[0;91m"
#define C_RED 						"\e[0;31m"
#define C_HGREEN 					"\e[0;92m"
#define C_YELLOW 					"\e[0;33m"
#define C_HCYAN 					"\e[0;96m"
#define C_CYAN 						"\e[0;36m"
#define C_HWHITE 					"\e[0;97m"
#define C_WHITE 					"\e[0;37m"
#define C_DEFAULT 					"\e[0m"

#define PROMPT						";=exit) -> "
#define BANNER 						printf("\n%s%s v%s by L. <%s>%s\n\n",C_HWHITE,PROGRAM_NAME, PROGRAM_VERSION,PROGRAM_URL,C_DEFAULT);

#define	DEFAULT_RESPONSE_VELOCITY	100000

bool cancel=FALSE;

char * get_readline(char *prompt, bool addHistory){
	char *lineRead=(char *)NULL;
	if(lineRead){
		free(lineRead);
		lineRead=(char *)NULL;
	}
	lineRead = readline(prompt);
	if(lineRead && *lineRead && addHistory) add_history(lineRead);
	return(lineRead);
}

void print_result(ChatGPTResponse *cgptResponse, long int responseVelocity, bool showFinishReason, bool showTotalTokens){
	printf("%s\n",C_HWHITE);
	for(int i=0;cgptResponse->message[i]!=0 && !cancel;i++){
		usleep(rand()%responseVelocity + 20000);
		if(cgptResponse->message[i]=='\\'){
			switch(cgptResponse->message[i+1]){
			case 'n':
				printf("\n");
				break;
			case 'r':
				printf("\r");
				break;
			case 't':
				printf("\t");
				break;
			case '\\':
				printf("\\");
				break;
			case '"':
				printf("\"");
				break;
			default:
				break;
			}
			i++;
			continue;
		}
		printf("%c",cgptResponse->message[i]);
		fflush(stdout);
	}
	printf("%s\n",C_DEFAULT);
	if(showFinishReason && !cancel) printf("\n%sFinish status: %s%s\n",C_DEFAULT,C_YELLOW,cgptResponse->finishReason);
	if(showFinishReason && cancel) printf("%s\nFinish status: %scanceled by user\n",C_DEFAULT,C_YELLOW);
	if(showTotalTokens) printf("%s\nTotal tokens: %s%d\n",C_DEFAULT,C_YELLOW, cgptResponse->totalTokens);
	printf("%s\n",C_DEFAULT);
}

void usage(char *programName){
	BANNER;
	printf("See: man chatgp-terminal or refer to the documentation <https://rb.gy/75nbf>\n\n");
}

void signal_handler(int signalType){
	printf("\b\b  %s",C_DEFAULT);
	switch(signalType){
	case SIGINT:
		cancel=TRUE;
		break;
	default:
		break;
	}
}

void send_chat(ChatGPT cgpt, char *message, long int responseVelocity, bool showFinishedStatus, bool showTotalTokens, bool createContext){
	ChatGPTResponse cgptResponse;
	int resp=0;
	if((resp=libGPT_send_chat(cgpt, &cgptResponse, message,createContext))!=RETURN_OK){
		switch(resp){
		case LIBGPT_ZEROBYTESRECV_ERROR:
			printf("\n%sOps, zero bytes received. Try again...%s\n\n",C_HRED,C_DEFAULT);
			break;
		case LIBGPT_BUFFERSIZE_OVERFLOW:
			printf("\n%sBuffer size exceeded. Dynamically memory allocation for responses vars under development."
					" Pls, let me know <%s>.%s\n\n",C_HRED,PROGRAM_URL,C_DEFAULT);
			break;
		case LIBGPT_RESPONSE_MESSAGE_ERROR:
			printf("\n%s%s%s\n\n",C_HRED,cgptResponse.message,C_DEFAULT);
			break;
		case LIBGPT_SOCKET_RECV_TIMEOUT_ERROR:
			printf("\n%sTime out. Try again...%s\n\n",C_HRED,C_DEFAULT);
			break;
		default:
			if(cancel){
				printf("\n");
				break;
			}
			printf("\n%sDEBUG: Return error: %d. Errno: %d (%s).%s\n\n",C_HRED,resp, errno,strerror(errno),C_DEFAULT);
			break;
		}
	}else{
		print_result(&cgptResponse,responseVelocity, showFinishedStatus, showTotalTokens);
	}
	libGPT_clean_response(&cgptResponse);
}

int main(int argc, char *argv[]) {
	signal(SIGINT, signal_handler);
	char *apikey="", *role=LIBGPT_DEFAULT_ROLE, *message=NULL;
	size_t len=0;
	int maxTokens=LIBGPT_DEFAULT_MAX_TOKENS, responseVelocity=DEFAULT_RESPONSE_VELOCITY;
	bool showFinishedStatus=FALSE,showTotalTokens=FALSE, createContext=TRUE;
	double temperature=LIBGPT_DEFAULT_TEMPERATURE;
	for(int i=1;i<argc;i+=2){
		char *tail=NULL;
		if(strcmp(argv[i],"--version")==0){
			BANNER;
			exit(EXIT_SUCCESS);
		}
		if(strcmp(argv[i],"--apikeyfile")==0){
			FILE *f=fopen(argv[i+1],"r");
			if(f==NULL){
				printf("\n%sError opening API key file. Error %d: %s%s\n\n",C_HRED,errno,strerror(errno),C_DEFAULT);
				exit(EXIT_FAILURE);
			}
			while(getline(&apikey, &len, f)!=-1);
			apikey[strlen(apikey)-1]=0;
			fclose(f);
			continue;
		}
		if(strcmp(argv[i],"--apikey")==0){
			apikey=argv[i+1];
			continue;
		}
		if(strcmp(argv[i],"--role")==0){
			role=argv[i+1];
			continue;
		}
		if(strcmp(argv[i],"--max-tokens")==0){
			maxTokens=strtoul(argv[i+1],&tail,10);
			if(maxTokens<=0 || *tail!='\0'){
				printf("\n%sMax.Tokens value not valid.%s\n\n",C_HRED,C_DEFAULT);
				exit(EXIT_FAILURE);
			}
			continue;
		}
		if(strcmp(argv[i],"--temperature")==0){
			temperature=strtod(argv[i+1],&tail);
			if(temperature<=0.0 || *tail!='\0'){
				printf("\n%sTemperature value not valid.%s\n\n",C_HRED,C_DEFAULT);
				exit(EXIT_FAILURE);
			}
			continue;
		}
		if(strcmp(argv[i],"--response-velocity")==0){
			responseVelocity=strtod(argv[i+1],&tail);
			if(responseVelocity<=0 || *tail!='\0'){
				printf("\n%sResponse velocity value not valid.%s\n\n",C_HRED,C_DEFAULT);
				exit(EXIT_FAILURE);
			}
			continue;
		}
		if(strcmp(argv[i],"--message")==0){
			message=argv[i+1];
			continue;
		}
		if(strcmp(argv[i],"--show-finished-status")==0){
			showFinishedStatus=TRUE;
			i--;
			continue;
		}
		if(strcmp(argv[i],"--show-total-tokens")==0){
			showTotalTokens=TRUE;
			i--;
			continue;
		}
		if(strcmp(argv[i],"--no-create-context")==0){
			createContext=FALSE;
			i--;
			continue;
		}
		if(strcmp(argv[i],"--help")==0){
			usage(argv[0]);
			exit(EXIT_FAILURE);
		}
		usage(argv[0]);
		printf("%sArgument '%s' not recognized.%s\n\n",C_HRED,argv[i],C_DEFAULT);
		exit(EXIT_FAILURE);
	}
	ChatGPT cgpt;
	if(libGPT_init(&cgpt, apikey, role, maxTokens, temperature)==LIBGPT_INIT_ERROR){
		printf("\n%sError init structure. %s%s\n\n",C_HRED,strerror(errno),C_DEFAULT);
		exit(EXIT_FAILURE);
	}
	if(message!=NULL){
		send_chat(cgpt, message, responseVelocity, showFinishedStatus, showTotalTokens, FALSE);
		libGPT_clean(&cgpt);
		exit(EXIT_SUCCESS);
	}
	rl_initialize();
	printf("\n");
	do{
		cancel=FALSE;
		printf("%s",C_HCYAN);
		char *message=get_readline(PROMPT, TRUE);
		printf("%s",C_DEFAULT);
		if(strcmp(message,";")==0) break;
		if(strcmp(message,"")==0){
			printf("\n");
			continue;
		}
		send_chat(cgpt, message, responseVelocity, showFinishedStatus, showTotalTokens, createContext);
	}while(TRUE);
	libGPT_clean(&cgpt);
	printf("\n");
	exit(EXIT_SUCCESS);
}
