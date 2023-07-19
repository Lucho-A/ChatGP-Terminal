/*
 ============================================================================
 Name        : ChatGP-Terminal.c
 Author      : L. (lucho-a.github.io)
 Version     : 1.0.2
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
#define PROGRAM_VERSION				"1.0.2"
#define PROGRAM_URL					"https://github.com/lucho-a/chatgp-terminal"
#define PROGRAM_CONTACT				"<https://lucho-a.github.io/>"

#define C_HRED 						"\033[0;91m"
#define C_RED 						"\033[0;31m"
#define C_HGREEN 					"\033[0;92m"
#define C_YELLOW 					"\033[0;33m"
#define C_HCYAN 					"\033[0;96m"
#define C_CYAN 						"\033[0;36m"
#define C_HWHITE 					"\033[0;97m"
#define C_WHITE 					"\033[0;37m"
#define C_DEFAULT 					"\033[0m"

#define PROMPT						C_HCYAN";=exit) -> "C_DEFAULT
#define BANNER 						printf("\n%s%s v%s by L. %s%s\n\n",C_HWHITE,PROGRAM_NAME, PROGRAM_VERSION,PROGRAM_CONTACT,C_DEFAULT);

int cancel=FALSE;

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

void print_result(ChatGPTResponse cgptResponse, long int responseVelocity, bool finishReason){
	printf("%s\n",C_HWHITE);
	for(int i=0;cgptResponse.message[i]!=0 && !cancel;i++){
		usleep(rand()%responseVelocity + 20000);
		if(cgptResponse.message[i]=='\\'){
			switch(cgptResponse.message[i+1]){
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
		if(cgptResponse.message[i-1]=='%' && cgptResponse.message[i-1]=='%'){
			printf("%%");
			i++;
			continue;
		}
		printf("%c",cgptResponse.message[i]);
		fflush(stdout);
	}
	if(finishReason && !cancel) printf("\n\n%sFinish status: %s%s",C_DEFAULT,C_YELLOW,cgptResponse.finishReason);
	if(finishReason && cancel) printf("\n\n%sFinish status: %scanceled by user",C_DEFAULT,C_YELLOW);
	printf("%s\n\n",C_DEFAULT);
}

void usage(char *programName){
	BANNER;
	char cwd[512]="", cmd[1024]="";
	getcwd(cwd, sizeof(cwd));
	snprintf(cmd, sizeof(cmd),"man %s/chatgp-terminal.1", cwd);
	system(cmd);
}

void signal_handler(int signalType){
	printf("\b\b  %s\n",C_DEFAULT);
	switch(signalType){
	case SIGINT:
		printf("\n");
		cancel=TRUE;
		break;
	default:
		break;
	}
}

void send_chat(ChatGPT cgpt, char *message, long int responseVelocity, bool showFinishedStatus){
	ChatGPTResponse cgptResponse;
	int resp=0;
	if((resp=libGPT_send_chat(cgpt, &cgptResponse, message))<=0){
		if(resp==0){
			printf("\n%sOps, zero bytes received. Try again...%s\n\n",C_HRED,C_DEFAULT);
			return;
		}
		switch(resp){
		case LIBGPT_RESPONSE_MESSAGE_ERROR:
			printf("\n%s%s%s\n\n",C_HRED,cgptResponse.errorMessage,C_DEFAULT);
			break;
		default:
			printf("\n%sDEBUG: Return error: %d. Errno: %d (%s).%s\n\n",C_HRED,resp, errno,strerror(errno),C_DEFAULT);
			break;
		}
	}else{
		print_result(cgptResponse,responseVelocity, showFinishedStatus);
	}
}

int main(int argc, char *argv[]) {
	signal(SIGINT, signal_handler);
	char *apikey=NULL, *role=LIBGPT_DEFAULT_ROLE, *message=NULL;
	size_t len=0;
	int maxTokens=LIBGPT_DEFAULT_MAX_TOKENS, responseVelocity=LIBGPT_DEFAULT_RESPONSE_VELOCITY, showFinishedStatus=FALSE;
	double temperature=LIBGPT_DEFAULT_TEMPERATURE;
	for(int i=1;i<argc;i+=2){
		if(strcmp(argv[i],"--version")==0){
			BANNER;
			exit(EXIT_SUCCESS);
		}
		if(strcmp(argv[i],"--apikeyfile")==0){
			FILE *f=fopen(argv[i+1],"r");
			if(f==NULL){
				perror("fopen");
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
			maxTokens=strtoul(argv[i+1],NULL,10);
			if(maxTokens<=0){
				printf("\n%sMax.Tokens value not valid.%s\n\n",C_HRED,C_DEFAULT);
				exit(EXIT_FAILURE);
			}
			continue;
		}
		if(strcmp(argv[i],"--temperature")==0){
			temperature=strtod(argv[i+1],NULL);
			if(temperature<=0){
				printf("\n%sTemperature value not valid.%s\n\n",C_HRED,C_DEFAULT);
				exit(EXIT_FAILURE);
			}
			continue;
		}
		if(strcmp(argv[i],"--response-velocity")==0){
			responseVelocity=strtod(argv[i+1],NULL);
			if(responseVelocity<=0){
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
		if(strcmp(argv[i],"--help")==0){
			usage(argv[0]);
			exit(EXIT_FAILURE);
		}
		printf("\n%sArgument '%s' not recognized.%s\n",C_HRED,argv[i],C_DEFAULT);
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}
	ChatGPT cgpt;
	if(libGPT_init(&cgpt, apikey, role, maxTokens, temperature)==LIBGPT_INIT_ERROR){
		printf("\n%sError init structure. %s%s\n\n",C_HRED,strerror(errno),C_DEFAULT);
		exit(EXIT_FAILURE);
	}
	if(message!=NULL){
		send_chat(cgpt, message, responseVelocity, showFinishedStatus);
		exit(EXIT_SUCCESS);
	}
	printf("\n");
	do{
		cancel=FALSE;
		char *message=get_readline(PROMPT, TRUE);
		if(strcmp(message,";")==0) break;
		send_chat(cgpt, message, responseVelocity, showFinishedStatus);
		free(message);
	}while(TRUE);
	printf("\n");
	exit(EXIT_SUCCESS);
}
