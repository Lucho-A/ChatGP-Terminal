/*
 ============================================================================
 Name        : ChatGP-Terminal.c
 Author      : L. (lucho-a.github.io)
 Version     : 1.0.1
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

#include "libGPT/libGPT.h"

#define PROGRAM_NAME				"ChatGP-Terminal"
#define PROGRAM_VERSION				"1.0.1"
#define PROGRAM_URL					"https://github.com/lucho-a/chatgp-terminal"
#define PROGRAM_CONTACT				"(https://lucho-a.github.io/)"

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

static inline char * get_readline(char *prompt, bool addHistory){
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
	if(cancel) printf("\n\n%sFinish status: %scancel by user",C_DEFAULT,C_YELLOW);
	printf("%s\n\n",C_DEFAULT);
}

void usage(char *programName){
	BANNER;
	printf("Usage: \n\n$ %s --apikeyfile string(NULL) | --apikey string(NULL) --role string(\"\") --max-tokens int(256) "
			"--temperature double(0.5) --response-velocity int(100000) "
			"--message string(NULL)\n\n"
			"Examples: \n\n"
			"$ %s --apikeyfile \"/home/user/.cgpt_key.key\" --role \"Act as Musician\"\n"
			"$ %s --apikeyfile \"/home/user/.cgpt_key.key\" --role \"Act as IT Professional\" --max-tokens 512\n"
			"$ %s --apikey \"1234567890ABCD\"\n"
			"$ %s --apikey \"1234567890ABCD\" --message \"Who was Chopin\"\n"
			"$ %s --help\n\n",programName,programName,programName,programName,programName,programName);
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

int main(int argc, char *argv[]) {
	signal(SIGINT, signal_handler);
	char *apikey=NULL, *role=LIBGPT_DEFAULT_ROLE, *message=NULL;
	size_t len=0;
	int maxTokens=LIBGPT_DEFAULT_MAX_TOKENS, responseVelocity=LIBGPT_DEFAULT_RESPONSE_VELOCITY;
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
		if(strcmp(argv[i],"--help")==0){
			usage(argv[0]);
			exit(EXIT_FAILURE);
		}
		printf("\n%sArgument '%s' not recognized.%s\n",C_HRED,argv[i],C_DEFAULT);
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}
	ChatGPT cgpt;
	ChatGPTResponse cgptResponse;
	if(libGPT_init(&cgpt, apikey, role, maxTokens, temperature)==LIBGPT_INIT_ERROR){
		perror("Init error");
		exit(EXIT_FAILURE);
	}
	if(message!=NULL){
		if(libGPT_send_chat(cgpt, &cgptResponse, message)<=0){
			printf("\n%s%s%s\n",C_HRED,cgptResponse.errorMessage,C_DEFAULT);
		}else{
			print_result(cgptResponse,responseVelocity, TRUE);
		}
		exit(EXIT_SUCCESS);
	}
	printf("\n");
	do{
		cancel=FALSE;
		char *message=get_readline(PROMPT, TRUE);
		if(strcmp(message,";")==0) break;
		if(libGPT_send_chat(cgpt, &cgptResponse, message)<=0){
			printf("\n%s%s%s\n\n",C_HRED,cgptResponse.errorMessage,C_DEFAULT);
		}else{
			print_result(cgptResponse,responseVelocity, TRUE);
		}
	}while(TRUE);
	printf("\n");
	exit(EXIT_SUCCESS);
}

