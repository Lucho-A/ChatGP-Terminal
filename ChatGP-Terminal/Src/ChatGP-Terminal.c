/*
 ============================================================================
 Name        : ChatGP-Terminal.c
 Author      : L.
 Version     : 1.0.0
 Copyright   : 
 Description : Main file
 ============================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "../libGPT/libGPT.h"

#define PROGRAM_NAME				"ChatGP-Terminal"
#define PROGRAM_VERSION				"1.0.0"
#define PROGRAM_URL					"www.github.com/lucho-a/chatgp-terminal"
#define PROGRAM_CONTACT				"www.lucho-a.github.io"

#define C_HRED 						"\e[0;91m"
#define C_RED 						"\e[0;31m"
#define C_HGREEN 					"\e[0;92m"
#define C_YELLOW 					"\e[0;33m"
#define C_HCYAN 					"\e[0;96m"
#define C_CYAN 						"\e[0;36m"
#define C_HWHITE 					"\e[0;97m"
#define C_WHITE 					"\e[0;37m"
#define C_DEFAULT 					"\e[0m"

#define PROMPT						C_HCYAN";=exit) -> "C_DEFAULT

static inline char * get_readline(char *prompt, bool addHistory){
	char *lineRead=(char *)NULL;
	if(lineRead){
		free(lineRead);
		lineRead = (char *)NULL;
	}
	lineRead = readline(prompt);
	if (lineRead && *lineRead && addHistory) add_history (lineRead);
	return (lineRead);
}

void print_result(ChatGPTResponse cgptResponse, long int responseVelocity, bool finishReason){
	printf("%s\n",C_HWHITE);
	for(int i=0;cgptResponse.message[i]!=0;i++){
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
	if(finishReason) printf("\n\n%sFinish status: %s%s",C_DEFAULT,C_YELLOW,cgptResponse.finishReason);
	printf("%s\n\n",C_DEFAULT);
}

void usage(char *programName){
	printf("\nUsage: $ %s --apikeyfile string(NULL) | --apikey string(NULL) --role string(\"\") --max-tokens int(256) --temperature double(0.5) --response-velocity int(100000)\n\n"
			"Examples: \n\n"
			"$ %s --apikeyfile \"/home/user/.cgpt_key.key\" --role \"Act as Musician\"\n"
			"$ %s --apikeyfile \"/home/user/.cgpt_key.key\" --role \"Act as IT Professional\" --max-tokens 512\n"
			"$ %s --apikey \"1234567890ABCD\"\n"
			"$ %s --help\n\n",programName,programName,programName,programName,programName);
}

int main(int argc, char *argv[]) {
	char *apikey=NULL, *role=LIBGPT_DEFAULT_ROLE;
	size_t len=0;
	int maxTokens=LIBGPT_DEFAULT_MAX_TOKENS, responseVelocity=LIBGPT_DEFAULT_RESPONSE_VELOCITY;
	double temperature=LIBGPT_DEFAULT_TEMPERATURE;
	for(int i=1;i<argc;i+=2){
		if(strcmp(argv[i],"--version")==0){
			printf("\n%s  %s v%s%s\n",C_HCYAN,PROGRAM_NAME, PROGRAM_VERSION,C_DEFAULT);
			printf("\n%s  URL: %s%s",C_HWHITE,C_DEFAULT, PROGRAM_URL);
			printf("\n%s  Contact: %s%s\n\n",C_HWHITE,C_DEFAULT, PROGRAM_CONTACT);
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
			continue;
		}
		if(strcmp(argv[i],"--temperature")==0){
			temperature=strtod(argv[i+1],NULL);
			continue;
		}
		if(strcmp(argv[i],"--response-velocity")==0){
			responseVelocity=strtod(argv[i+1],NULL);
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
	printf("\n");
	do{
		char *message=get_readline(PROMPT, TRUE);
		if(strcmp(message,";")==0) break;
		if(libGPT_send_chat(cgpt, &cgptResponse, message)<=0){
			perror(cgptResponse.errorMessage);
			exit(EXIT_FAILURE);
		}
		print_result(cgptResponse,responseVelocity, TRUE);
	}while(TRUE);
	printf("\n");
	exit(EXIT_SUCCESS);
}

