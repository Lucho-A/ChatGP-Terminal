/*
 ============================================================================
 Name        : ChatGP-Terminal.c
 Author      : L. (lucho-a.github.io)
 Version     : 1.2.0
 Created on	 : 2023/07/18 (v1.0.0)
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
#include <pthread.h>
#include <espeak-ng/speak_lib.h>

#include "libGPT/libGPT.h"

#define PROGRAM_NAME				"ChatGP-Terminal"
#define PROGRAM_VERSION				"1.2.0"
#define PROGRAM_URL					"https://github.com/lucho-a/chatgp-terminal"
#define PROGRAM_CONTACT				"<https://lucho-a.github.io/>"

#define C_YELLOW 					"\e[0;33m"
#define C_HGREEN 					"\e[0;92m"
#define C_HRED 						"\e[0;91m"
#define C_HCYAN 					"\e[0;96m"
#define C_HWHITE 					"\e[0;97m"
#define C_DEFAULT 					"\e[0m"

#define PROMPT_DEFAULT				"-> "
#define BANNER 						printf("\n%s%s v%s by L. <%s>%s\n\n",C_HWHITE,PROGRAM_NAME, PROGRAM_VERSION,PROGRAM_URL,C_DEFAULT);

#define	DEFAULT_RESPONSE_VELOCITY	10000

bool canceled=FALSE;
bool exitProgram=FALSE;
int prevInput=0;

int readline_input(FILE *stream){
	if(exitProgram) return 13;
	int c=fgetc(stream);
	if(c==13 && prevInput==27){
		if(strlen(rl_line_buffer)!=0){
			rl_insert_text("\n");
			prevInput=0;
			return 0;
		}
		exitProgram=TRUE;
		return 13;
	}
	if(c==9) rl_insert_text("\t");
	if(c==-1 || c==4) return 13;
	prevInput=c;
	return c;
}

static char *readline_get(const char *prompt, bool addHistory){
	char *lineRead=(char *)NULL;
	if(lineRead){
		free(lineRead);
		lineRead=(char *)NULL;
	}
	lineRead=readline(prompt);
	if(lineRead && *lineRead && addHistory) add_history(lineRead);
	return(lineRead);
}

int log_response(ChatGPTResponse cgptResponse, char *logFile){
	time_t timestamp = cgptResponse.created;
	struct tm tm = *localtime(&timestamp);
	FILE *f=fopen(logFile,"a");
	if(f==NULL) return RETURN_ERROR;
	fprintf(f,"\"%d/%02d/%02d\"\t\"%02d:%02d:%02d\"\t",tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	fprintf(f,"%s\t%d\t%d\t%d\t%.6f\n",cgptResponse.finishReason, cgptResponse.promptTokens, cgptResponse.completionTokens,cgptResponse.totalTokens,cgptResponse.cost);
	fclose(f);
	return RETURN_OK;
}

void *speech_response(void *args){
	char* message=(char*)args;
	espeak_Synth(message, strlen(message)+1, 0, POS_CHARACTER, 0, espeakCHARS_AUTO, NULL, NULL);
	espeak_Synchronize();
	pthread_exit(NULL);
}

void print_response(ChatGPTResponse *cgptResponse, long int responseVelocity, bool showFinishReason, bool showPromptTokens,
		bool showCompletionTokens, bool showTotalTokens, bool showCost){
	char *defColor=(responseVelocity==0)?"":C_DEFAULT, *respColor=(responseVelocity==0)?"":C_YELLOW;
	if(responseVelocity==0){
		printf("%s",cgptResponse->contentFormatted);
	}else{
		printf("%s\n",C_HWHITE);
		for(int i=0;cgptResponse->contentFormatted[i]!=0 && !canceled;i++){
			usleep(rand() % responseVelocity + 20000);
			printf("%c",cgptResponse->contentFormatted[i]);
			fflush(stdout);
		}
	}
	printf("\n");
	if(showFinishReason && !canceled) printf("\n%sFinish status: %s%s\n",defColor,respColor,cgptResponse->finishReason);
	if(showFinishReason && canceled) printf("%s\nFinish status: %scanceled by user\n",defColor,respColor);
	if(showPromptTokens) printf("%s\nPrompt tokens: %s%d\n",defColor,respColor, cgptResponse->promptTokens);
	if(showCompletionTokens) printf("%s\nCompletion tokens: %s%d\n",defColor,respColor, cgptResponse->completionTokens);
	if(showTotalTokens) printf("%s\nTotal tokens: %s%d\n",defColor,respColor, cgptResponse->totalTokens);
	if(showCost) printf("%s\nCost approx.: %s%.6f\n",defColor,respColor, cgptResponse->cost);
}

void usage(char *programName){
	BANNER;
	printf("See: man chatgp-terminal or refer to the documentation <https://rb.gy/75nbf>\n\n");
}

void signal_handler(int signalType){
	switch(signalType){
	case SIGINT:
		canceled=TRUE;
		if(espeak_IsPlaying()) espeak_Cancel();
		break;
	default:
		break;
	}
}

void check_service_status(){
	char *serviceStatus=NULL;
	int resp=0;
	if((resp=libGPT_get_service_status(&serviceStatus))!= RETURN_OK){
		printf("\n%s%s%s\n",C_HRED, libGPT_error(resp),C_DEFAULT);
		return;
	}
	printf("\nService Status: %s'%s'%s\n",C_HWHITE,serviceStatus,C_DEFAULT);
	free(serviceStatus);
	return;
}

void send_chat(ChatGPT cgpt, char *message, long int responseVelocity, bool showFinishedStatus, bool showPromptTokens,
		bool showCompletionTokens, bool showTotalTokens, bool showCost, bool tts, char *logFile, bool checkStatus){
	ChatGPTResponse cgptResponse;
	int resp=0;
	if((resp=libGPT_send_chat(cgpt, &cgptResponse, message))!=RETURN_OK){
		switch(resp){
		case LIBGPT_RESPONSE_MESSAGE_ERROR:
			printf("\n%s%s%s\n",C_HRED,cgptResponse.contentFormatted,C_DEFAULT);
			break;
		default:
			if(canceled){
				printf("\n");
				break;
			}
			printf("\n%s%s%s\n",C_HRED, libGPT_error(resp),C_DEFAULT);
			break;
		}
	}else{
		if(tts){
			pthread_t threadSpeechMessage;
			pthread_create(&threadSpeechMessage, NULL, speech_response, (void*)cgptResponse.contentFormatted);
		}
		print_response(&cgptResponse, responseVelocity, showFinishedStatus, showPromptTokens, showCompletionTokens,showTotalTokens,showCost);
		if(logFile!=NULL){
			if(log_response(cgptResponse, logFile)!=RETURN_OK) printf("\n%sError logging file. Error %d: %s%s\n\n",C_HRED,errno,strerror(errno),C_DEFAULT);;
		}
	}
	libGPT_clean_response(&cgptResponse);
}

int main(int argc, char *argv[]) {
	signal(SIGINT, signal_handler);
	char *apikey="", *role=NULL, *message=NULL, *saveMessagesTo="", *sessionFile=NULL, *roleFile=NULL,
			*logFile=NULL;
	size_t len=0;
	int maxTokens=LIBGPT_DEFAULT_MAX_TOKENS, responseVelocity=DEFAULT_RESPONSE_VELOCITY,
			maxContextMessages=LIBGPT_DEFAULT_MAX_CONTEXT_MSGS, n=LIBGPT_DEFAULT_N;
	bool checkStatus=FALSE, showFinishedStatus=FALSE,showPromptTokens=FALSE,showCompletionTokens=FALSE,
			showTotalTokens=FALSE,textToSpeech=FALSE, csv=FALSE,showCost=FALSE;
	double freqPenalty=LIBGPT_DEFAULT_FREQ_PENALTY, temperature=LIBGPT_DEFAULT_TEMPERATURE;
	for(int i=1;i<argc;i+=2){
		char *tail=NULL;
		if(strcmp(argv[i],"--version")==0){
			BANNER;
			exit(EXIT_SUCCESS);
		}
		if(strcmp(argv[i],"--apikeyfile")==0){
			if(i+1>=argc){
				printf("\n%s--apikeyfile: option argument missing.%s\n\n",C_HRED,C_DEFAULT);
				exit(EXIT_FAILURE);
			}
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
			if(i+1>=argc){
				printf("\n%s--apikey: option argument missing.%s\n\n",C_HRED,C_DEFAULT);
				exit(EXIT_FAILURE);
			}
			apikey=argv[i+1];
			continue;
		}
		if(strcmp(argv[i],"--role")==0){
			if(i+1>=argc){
				printf("\n%s--role: option argument missing.%s\n\n",C_HRED,C_DEFAULT);
				exit(EXIT_FAILURE);
			}
			role=argv[i+1];
			continue;
		}
		if(strcmp(argv[i],"--max-tokens")==0){
			if(i+1>=argc){
				printf("\n%s--max-tokens: option argument missing.%s\n\n",C_HRED,C_DEFAULT);
				exit(EXIT_FAILURE);
			}
			maxTokens=strtoul(argv[i+1],&tail,10);
			if(*tail!='\0'){
				printf("\n%sMax.Tokens value not valid.%s\n\n",C_HRED,C_DEFAULT);
				exit(EXIT_FAILURE);
			}
			continue;
		}
		if(strcmp(argv[i],"--freq-penalty")==0){
			if(i+1>=argc){
				printf("\n%s--freq-penalty: option argument missing.%s\n\n",C_HRED,C_DEFAULT);
				exit(EXIT_FAILURE);
			}
			freqPenalty=strtod(argv[i+1],&tail);
			if(*tail!='\0'){
				printf("\n%sFrequency Penalty value not valid.%s\n\n",C_HRED,C_DEFAULT);
				exit(EXIT_FAILURE);
			}
			continue;
		}
		if(strcmp(argv[i],"--temperature")==0){
			if(i+1>=argc){
				printf("\n%s--temperature: option argument missing.%s\n\n",C_HRED,C_DEFAULT);
				exit(EXIT_FAILURE);
			}
			temperature=strtod(argv[i+1],&tail);
			if(*tail!='\0'){
				printf("\n%sTemperature value not valid.%s\n\n",C_HRED,C_DEFAULT);
				exit(EXIT_FAILURE);
			}
			continue;
		}
		if(strcmp(argv[i],"--n")==0){
			if(i+1>=argc){
				printf("\n%s--n: option argument missing.%s\n\n",C_HRED,C_DEFAULT);
				exit(EXIT_FAILURE);
			}
			n=strtoul(argv[i+1],&tail,10);
			if(*tail!='\0'){
				printf("\n%sN value not valid.%s\n\n",C_HRED,C_DEFAULT);
				exit(EXIT_FAILURE);
			}
			continue;
		}
		if(strcmp(argv[i],"--response-velocity")==0){
			if(i+1>=argc){
				printf("\n%s--response-velocity: option argument missing.%s\n\n",C_HRED,C_DEFAULT);
				exit(EXIT_FAILURE);
			}
			responseVelocity=strtod(argv[i+1],&tail);
			if(responseVelocity<=0 || *tail!='\0'){
				printf("\n%sResponse velocity value not valid.%s\n\n",C_HRED,C_DEFAULT);
				exit(EXIT_FAILURE);
			}
			continue;
		}
		if(strcmp(argv[i],"--max-context-messages")==0){
			if(i+1>=argc){
				printf("\n%s--max-context-messages: option argument missing.%s\n\n",C_HRED,C_DEFAULT);
				exit(EXIT_FAILURE);
			}
			maxContextMessages=strtod(argv[i+1],&tail);
			if(*tail!='\0'){
				printf("\n%sMax. Context Messages value not valid.%s\n\n",C_HRED,C_DEFAULT);
				exit(EXIT_FAILURE);
			}
			continue;
		}
		if(strcmp(argv[i],"--session-file")==0){
			if(i+1>=argc){
				printf("\n%s--session-file: option argument missing.%s\n\n",C_HRED,C_DEFAULT);
				exit(EXIT_FAILURE);
			}
			FILE *f=fopen(argv[i+1],"a");
			if(f==NULL){
				printf("\n%sError opening/creating session file. Error %d: %s%s\n\n",C_HRED,errno,strerror(errno),C_DEFAULT);
				exit(EXIT_FAILURE);
			}
			fclose(f);
			sessionFile=argv[i+1];
			continue;
		}
		if(strcmp(argv[i],"--role-file")==0){
			if(i+1>=argc){
				printf("\n%s--role-file: option argument missing.%s\n\n",C_HRED,C_DEFAULT);
				exit(EXIT_FAILURE);
			}
			FILE *f=fopen(argv[i+1],"r");
			if(f==NULL){
				printf("\n%sError opening role file. Error %d: %s%s\n\n",C_HRED,errno,strerror(errno),C_DEFAULT);
				exit(EXIT_FAILURE);
			}
			fclose(f);
			roleFile=argv[i+1];
			continue;
		}
		if(strcmp(argv[i],"--log-file")==0){
			if(i+1>=argc){
				printf("\n%s--log-file: option argument missing.%s\n\n",C_HRED,C_DEFAULT);
				exit(EXIT_FAILURE);
			}
			FILE *f=fopen(argv[i+1],"a");
			if(f==NULL){
				printf("\n%sError opening/creating log file. Error %d: %s%s\n\n",C_HRED,errno,strerror(errno),C_DEFAULT);
				exit(EXIT_FAILURE);
			}
			fclose(f);
			logFile=argv[i+1];
			continue;
		}
		if(strcmp(argv[i],"--save-messages-to")==0){
			if(i+1>=argc){
				printf("\n%s--save-messages-to: option argument missing.%s\n\n",C_HRED,C_DEFAULT);
				exit(EXIT_FAILURE);
			}
			FILE *f=fopen(argv[i+1],"a");
			if(f==NULL){
				printf("\n%sError opening/creating save-messages file. Error %d: %s%s\n\n",C_HRED,errno,strerror(errno),C_DEFAULT);
				exit(EXIT_FAILURE);
			}
			fclose(f);
			saveMessagesTo=argv[i+1];
			continue;
		}
		if(strcmp(argv[i],"--csv-format")==0){
			csv=TRUE;
			i--;
			continue;
		}
		if(strcmp(argv[i],"--message")==0){
			if(i+1>=argc){
				printf("\n%s--message: option argument missing.%s\n\n",C_HRED,C_DEFAULT);
				exit(EXIT_FAILURE);
			}
			message=argv[i+1];
			continue;
		}
		if(strcmp(argv[i],"--check-status")==0){
			checkStatus=TRUE;
			i--;
			continue;
		}
		if(strcmp(argv[i],"--show-finished-status")==0){
			showFinishedStatus=TRUE;
			i--;
			continue;
		}
		if(strcmp(argv[i],"--show-prompt-tokens")==0){
			showPromptTokens=TRUE;
			i--;
			continue;
		}
		if(strcmp(argv[i],"--show-completion-tokens")==0){
			showCompletionTokens=TRUE;
			i--;
			continue;
		}
		if(strcmp(argv[i],"--show-total-tokens")==0){
			showTotalTokens=TRUE;
			i--;
			continue;
		}
		if(strcmp(argv[i],"--show-cost")==0){
			showCost=TRUE;
			i--;
			continue;
		}
		if(strcmp(argv[i],"--tts")==0){
			if(i+1>=argc){
				printf("\n%s--tts: option argument missing.%s\n\n",C_HRED,C_DEFAULT);
				exit(EXIT_FAILURE);
			}
			if(espeak_Initialize(AUDIO_OUTPUT_PLAYBACK, 0, NULL, 0)!=-1){
				textToSpeech=TRUE;
				char setVoice[64]="";
				snprintf(setVoice,64,"%s+f3",argv[i+1]);
				if(espeak_SetVoiceByName(setVoice)==EE_OK){
					textToSpeech=TRUE;
				}else{
					printf("%s\nTTS language selection not valid. %s\n\n",C_HRED,C_DEFAULT);
					exit(EXIT_FAILURE);
				}
			}else{
				printf("%s\nEspeak initialization failed.%s\n",C_HRED,C_DEFAULT);
				exit(EXIT_FAILURE);
			}
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
	int resp=0;
	if((resp=libGPT_init(&cgpt, apikey, role, roleFile, maxTokens, freqPenalty, temperature,
			n, maxContextMessages))!=RETURN_OK){
		printf("\n%sInitialization error. %s%s\n\n",C_HRED,libGPT_error(resp),C_DEFAULT);
		exit(EXIT_FAILURE);
	}
	if(sessionFile!=NULL){
		if((resp=libGPT_import_session_file(sessionFile))!=RETURN_OK) printf("\n%sError importing session file. %s%s\n\n",C_HRED,libGPT_error(resp),C_DEFAULT);
	}
	if(message!=NULL){
		send_chat(cgpt, message, 0, showFinishedStatus, showPromptTokens, showCompletionTokens,
				showTotalTokens, showCost, FALSE, logFile, checkStatus);
		libGPT_clean(&cgpt);
		exit(EXIT_SUCCESS);
	}
	if(checkStatus) check_service_status();
	rl_getc_function=readline_input;
	char *messagePrompted=NULL;
	do{
		exitProgram=FALSE;
		canceled=FALSE;
		free(messagePrompted);
		printf("%s\n",C_HCYAN);
		messagePrompted=readline_get(PROMPT_DEFAULT, FALSE);
		printf("%s",C_DEFAULT);
		if(exitProgram && strcmp(messagePrompted,"")==0){
			free(messagePrompted);
			break;
		}
		if(exitProgram || canceled || strcmp(messagePrompted,"")==0) continue;
		if(strcmp(messagePrompted,"status;")==0){
			check_service_status();
			continue;
		}
		if(strcmp(messagePrompted,"flush;")==0){
			libGPT_flush_history();
			continue;
		}
		if(strcmp(messagePrompted,"save;")==0){
			if(saveMessagesTo==NULL || saveMessagesTo[0]==0){
				printf("\n%sNo file defined (see: '--save-message-to' option)%s\n",C_HRED,C_DEFAULT);
				continue;
			}
			if((resp=libGPT_save_message(saveMessagesTo, csv))!=RETURN_OK) printf("\n%sError saving file: %s%s\n",C_HRED,libGPT_error(resp),C_DEFAULT);
			continue;
		}
		send_chat(cgpt, messagePrompted, responseVelocity, showFinishedStatus, showPromptTokens,
				showCompletionTokens, showTotalTokens, showCost, textToSpeech,logFile, checkStatus);
		add_history(messagePrompted);
	}while(TRUE);
	if(sessionFile!=NULL){
		if(libGPT_export_session_file(sessionFile)!=RETURN_OK) printf("\n%sError dumping session. %s%s\n",C_HRED,libGPT_error(resp),C_DEFAULT);
	}
	if(textToSpeech) espeak_Terminate();
	libGPT_clean(&cgpt);
	rl_clear_history();
	printf("\n");
	exit(EXIT_SUCCESS);
}
