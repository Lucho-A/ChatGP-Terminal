/*
 ============================================================================
 Name        : ChatGP-Terminal.c
 Author      : L. (lucho-a.github.io)
 Version     : 1.2.1
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

#define PROGRAM_NAME					"ChatGP-Terminal"
#define PROGRAM_VERSION					"1.2.1"
#define PROGRAM_URL						"https://github.com/lucho-a/chatgp-terminal"
#define PROGRAM_CONTACT					"<https://lucho-a.github.io/>"

#define PROMPT_DEFAULT					"-> "
#define BANNER 							printf("\n%s%s v%s by L. <%s>%s\n\n",Colors.h_white,PROGRAM_NAME, PROGRAM_VERSION,PROGRAM_URL,Colors.def);

#define	DEFAULT_RESPONSE_VELOCITY		25000

struct Colors{
	char *yellow;
	char *h_green;
	char *h_red;
	char *h_cyan;
	char *h_white;
	char *def;
}Colors;

bool canceled=FALSE;
bool exitProgram=FALSE;
int prevInput=0;

void init_colors(bool uncolored){
	if(uncolored){
		Colors.yellow=Colors.h_green=Colors.h_red=Colors.h_cyan=Colors.h_white=Colors.def="";
		return;
	}
	Colors.yellow="\e[0;33m";
	Colors.h_green="\e[0;92m";
	Colors.h_red="\e[0;91m";
	Colors.h_cyan="\e[0;96m";
	Colors.h_white="\e[0;97m";
	Colors.def="\e[0m";
}

void print_error(char *msg, char *libGPTError, bool exitProgram){
	printf("\n%s%s%s%s\n",Colors.h_red, msg,libGPTError,Colors.def);
	if(exitProgram){
		printf("\n");
		exit(EXIT_FAILURE);
	}
}

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
	fprintf(f,"\"%d/%02d/%02d\"\t\"%02d:%02d:%02d\"\t",tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour,tm.tm_min, tm.tm_sec);
	fprintf(f,"%s\t%d\t%d\t%d\t%s\n",cgptResponse.finishReason, cgptResponse.promptTokens,
			cgptResponse.completionTokens,cgptResponse.totalTokens,cgptResponse.model);
	fclose(f);
	return RETURN_OK;
}

void *speech_response(void *args){
	char* message=(char*)args;
	espeak_Synth(message, strlen(message)+1, 0, POS_CHARACTER, 0, espeakCHARS_AUTO, NULL, NULL);
	espeak_Synchronize();
	pthread_exit(NULL);
}

void print_response(ChatGPTResponse *cgptResponse, long int responseVelocity, bool showModel,
		bool showFinishReason, bool showPromptTokens,bool showCompletionTokens, bool showTotalTokens){
	printf("%s",Colors.h_white);
	if(responseVelocity==0){
		printf("%s",cgptResponse->contentFormatted);
	}else{
		printf("\n");
		for(int i=0;cgptResponse->contentFormatted[i]!=0 && !canceled;i++){
			usleep(responseVelocity);
			printf("%c",cgptResponse->contentFormatted[i]);
			fflush(stdout);
		}
	}
	printf("\n");
	if(showModel) printf("%s\nModel: %s%s\n",Colors.def,Colors.yellow, cgptResponse->model);
	if(showFinishReason && !canceled) printf("\n%sFinish status: %s%s\n",Colors.def,Colors.yellow,cgptResponse->finishReason);
	if(showFinishReason && canceled) printf("%s\nFinish status: %scanceled by user\n",Colors.def,Colors.yellow);
	if(showPromptTokens) printf("%s\nPrompt tokens: %s%d\n",Colors.def,Colors.yellow, cgptResponse->promptTokens);
	if(showCompletionTokens) printf("%s\nCompletion tokens: %s%d\n",Colors.def,Colors.yellow, cgptResponse->completionTokens);
	if(showTotalTokens) printf("%s\nTotal tokens: %s%d\n",Colors.def,Colors.yellow, cgptResponse->totalTokens);
}

void usage(char *programName){
	BANNER;
	printf("See: man chatgp-terminal or refer to the documentation <https://rb.gy/75nbf>\n");
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
		print_error("",libGPT_error(resp),FALSE);
		return;
	}
	printf("\nService Status: %s'%s'%s\n",Colors.h_white,serviceStatus,Colors.def);
	free(serviceStatus);
	return;
}

void send_chat(ChatGPT cgpt, char *message, long int responseVelocity, bool showModel, bool showFinishedStatus,
		bool showPromptTokens, bool showCompletionTokens, bool showTotalTokens,bool tts,
		char *logFile, bool checkStatus){
	ChatGPTResponse cgptResponse;
	int resp=0;
	if((resp=libGPT_send_chat(cgpt, &cgptResponse, message))!=RETURN_OK){
		switch(resp){
		case LIBGPT_RESPONSE_MESSAGE_ERROR:
			print_error(cgptResponse.contentFormatted,"",FALSE);
			break;
		default:
			if(canceled){
				printf("\n");
				break;
			}
			print_error("",libGPT_error(resp),FALSE);
			break;
		}
	}else{
		if(tts){
			pthread_t threadSpeechMessage;
			pthread_create(&threadSpeechMessage, NULL, speech_response, (void*)cgptResponse.contentFormatted);
		}
		print_response(&cgptResponse, responseVelocity, showModel, showFinishedStatus, showPromptTokens,
				showCompletionTokens,showTotalTokens);
		if(logFile!=NULL){
			if(log_response(cgptResponse, logFile)!=RETURN_OK) print_error("Error logging file. ",strerror(errno),FALSE);
		}
	}
	libGPT_clean_response(&cgptResponse);
}

int main(int argc, char *argv[]) {
	signal(SIGINT, signal_handler);
	char *apikey="", *role=NULL, *message=NULL, *saveMessagesTo="", *sessionFile=NULL, *roleFile=NULL,
			*logFile=NULL, *model=NULL;
	size_t len=0;
	int maxTokens=LIBGPT_DEFAULT_MAX_TOKENS, responseVelocity=DEFAULT_RESPONSE_VELOCITY,
			maxContextMessages=LIBGPT_DEFAULT_MAX_CONTEXT_MSGS, n=LIBGPT_DEFAULT_N;
	bool checkStatus=FALSE, showFinishedStatus=FALSE,showPromptTokens=FALSE,showCompletionTokens=FALSE,
			showTotalTokens=FALSE,textToSpeech=FALSE, csv=FALSE, showModel=FALSE;
	double freqPenalty=LIBGPT_DEFAULT_FREQ_PENALTY, presPenalty=LIBGPT_DEFAULT_PRES_PENALTY, temperature=LIBGPT_DEFAULT_TEMPERATURE;
	init_colors(FALSE);
	for(int i=1;i<argc;i+=2){
		char *tail=NULL;
		if(strcmp(argv[i],"--version")==0){
			BANNER;
			exit(EXIT_SUCCESS);
		}
		if(strcmp(argv[i],"--apikeyfile")==0){
			if(i+1>=argc) print_error("--apikeyfile: option argument missing. ","",TRUE);
			FILE *f=fopen(argv[i+1],"r");
			if(f==NULL) print_error("Error opening API key file.",strerror(errno),TRUE);
			while(getline(&apikey, &len, f)!=-1);
			apikey[strlen(apikey)-1]=0;
			fclose(f);
			continue;
		}
		if(strcmp(argv[i],"--apikey")==0){
			if(i+1>=argc) print_error("--apikey: option argument missing. ","",TRUE);
			apikey=argv[i+1];
			continue;
		}
		if(strcmp(argv[i],"--role")==0){
			if(i+1>=argc) print_error("--role: option argument missing. ","",TRUE);
			role=argv[i+1];
			continue;
		}
		if(strcmp(argv[i],"--max-tokens")==0){
			if(i+1>=argc) print_error("--max-tokens: option argument missing. ","",TRUE);
			maxTokens=strtoul(argv[i+1],&tail,10);
			if(*tail!='\0') print_error("'Max.Tokens' value not valid. ","",TRUE);
			continue;
		}
		if(strcmp(argv[i],"--freq-penalty")==0){
			if(i+1>=argc) print_error("--freq-penalty: option argument missing. ","",TRUE);
			freqPenalty=strtod(argv[i+1],&tail);
			if(*tail!='\0') print_error("'Frequency Penalty' value not valid. ","",TRUE);
			continue;
		}
		if(strcmp(argv[i],"--pres-penalty")==0){
			if(i+1>=argc) print_error("--pres-penalty: option argument missing. ","",TRUE);
			presPenalty=strtod(argv[i+1],&tail);
			if(*tail!='\0') print_error("'Presence Penalty' value not valid. ","",TRUE);
			continue;
		}
		if(strcmp(argv[i],"--temperature")==0){
			if(i+1>=argc) print_error("--temperature: option argument missing. ","",TRUE);
			temperature=strtod(argv[i+1],&tail);
			if(*tail!='\0') print_error("'Temperature' value not valid. ","",TRUE);
			continue;
		}
		if(strcmp(argv[i],"--n")==0){
			if(i+1>=argc) print_error("--n: option argument missing. ","",TRUE);
			n=strtoul(argv[i+1],&tail,10);
			if(*tail!='\0') print_error("'N' value not valid. ","",TRUE);
			continue;
		}
		if(strcmp(argv[i],"--response-velocity")==0){
			if(i+1>=argc) print_error("--response-velocity: option argument missing. ","",TRUE);
			responseVelocity=strtod(argv[i+1],&tail);
			if(responseVelocity<0 || *tail!='\0') print_error("Response velocity value not valid. ","",TRUE);
			continue;
		}
		if(strcmp(argv[i],"--max-context-messages")==0){
			if(i+1>=argc) print_error("--max-context-messages: option argument missing. ","",TRUE);
			maxContextMessages=strtod(argv[i+1],&tail);
			if(*tail!='\0') print_error("'Max. Context Messages' value not valid. ","",TRUE);
			continue;
		}
		if(strcmp(argv[i],"--session-file")==0){
			if(i+1>=argc) print_error("--session-file: option argument missing. ","",TRUE);
			FILE *f=fopen(argv[i+1],"a");
			if(f==NULL) print_error("Error opening/creating session file. ",strerror(errno),TRUE);
			fclose(f);
			sessionFile=argv[i+1];
			continue;
		}
		if(strcmp(argv[i],"--role-file")==0){
			if(i+1>=argc) print_error("--role-file: option argument missing. ","",TRUE);
			FILE *f=fopen(argv[i+1],"r");
			if(f==NULL) print_error("Error opening role file. ",strerror(errno),TRUE);
			fclose(f);
			roleFile=argv[i+1];
			continue;
		}
		if(strcmp(argv[i],"--log-file")==0){
			if(i+1>=argc) print_error("--log-file: option argument missing. ","",TRUE);
			FILE *f=fopen(argv[i+1],"a");
			if(f==NULL) print_error("Error opening/creating log file. ",strerror(errno),TRUE);
			fclose(f);
			logFile=argv[i+1];
			continue;
		}
		if(strcmp(argv[i],"--save-messages-to")==0){
			if(i+1>=argc) print_error("--save-messages-to: option argument missing. ","",TRUE);
			FILE *f=fopen(argv[i+1],"a");
			if(f==NULL) print_error("Error opening/creating save-messages file. ",strerror(errno),TRUE);
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
			if(i+1>=argc) print_error("--message: option argument missing. ","",TRUE);
			message=argv[i+1];
			continue;
		}
		if(strcmp(argv[i],"--uncolored")==0){
			init_colors(TRUE);
			i--;
			continue;
		}
		if(strcmp(argv[i],"--model")==0){
			if(i+1>=argc) print_error("--model: option argument missing. ","",TRUE);
			if(model!=NULL) print_error("Duplicated model parameter found. ","",TRUE);
			model=argv[i+1];
			continue;
		}
		if(strcmp(argv[i],"--check-status")==0){
			checkStatus=TRUE;
			i--;
			continue;
		}
		if(strcmp(argv[i],"--show-model")==0){
			showModel=TRUE;
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
		if(strcmp(argv[i],"--tts")==0){
			if(i+1>=argc) print_error("--tts: option argument missing. ","",TRUE);
			if(espeak_Initialize(AUDIO_OUTPUT_PLAYBACK, 0, NULL, 0)!=-1){
				textToSpeech=TRUE;
				char setVoice[64]="";
				snprintf(setVoice,64,"%s+f3",argv[i+1]);
				if(espeak_SetVoiceByName(setVoice)==EE_OK){
					textToSpeech=TRUE;
				}else{
					print_error("TTS language selection not valid. ","",TRUE);
				}
			}else{
				print_error("Espeak initialization failed. ","",TRUE);
			}
			continue;
		}
		if(strcmp(argv[i],"--help")==0){
			usage(argv[0]);
			printf("\n");
			exit(EXIT_FAILURE);
		}
		usage(argv[0]);
		print_error(argv[i],": argument not recognized",TRUE);
	}
	if(model==NULL) model=LIBGPT_DEFAULT_MODEL;
	ChatGPT cgpt;
	int resp=0;
	if((resp=libGPT_init(&cgpt, model, apikey, role, roleFile, maxTokens, freqPenalty, presPenalty,
			temperature, n, maxContextMessages))!=RETURN_OK){
		print_error("Initialization error. ",libGPT_error(resp),TRUE);
	}
	if(sessionFile!=NULL){
		if((resp=libGPT_import_session_file(sessionFile))!=RETURN_OK)
			print_error("Error importing session file. ",libGPT_error(resp),FALSE);
	}
	if(message!=NULL){
		init_colors(TRUE);
		send_chat(cgpt, message, 0, showModel, showFinishedStatus, showPromptTokens, showCompletionTokens,
				showTotalTokens, FALSE, logFile, checkStatus);
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
		printf("%s\n",Colors.h_cyan);
		messagePrompted=readline_get(PROMPT_DEFAULT, FALSE);
		printf("%s",Colors.def);
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
				print_error("No file defined (see: '--save-message-to' option). ","",FALSE);
				continue;
			}
			if((resp=libGPT_save_message(saveMessagesTo, csv))!=RETURN_OK)
				print_error("Error saving file. ",libGPT_error(resp),FALSE);
			continue;
		}
		if(strstr(messagePrompted,"model;")==messagePrompted){
			char arg[128]="";
			int cont=0;
			for(int i=strlen("model;");i<strlen(messagePrompted) && i<128;i++){
				if(messagePrompted[i]!=' '){
					arg[cont]=messagePrompted[i];
					cont++;
				}
			}
			if (arg[0]==0) strcpy(arg,model);
			printf("\n%s\n",arg);
			int resp=0;
			if((resp=libGPT_set_model(&cgpt, arg))!=RETURN_OK) print_error("",libGPT_error(resp),FALSE);
			continue;
		}
		if(strstr(messagePrompted,"mt;")==messagePrompted){
			char arg[16]="";
			for(int i=strlen("mt;");i<strlen(messagePrompted) && i<16;i++) arg[i-strlen("mt;")]=messagePrompted[i];
			char *tail=NULL;
			long int lmaxTokens=strtoul(arg,&tail,10);
			if(*tail!='\0'){
				print_error("'Max.Tokens' value not valid. ","",FALSE);
				continue;
			}
			if (arg[0]==0) lmaxTokens = maxTokens;
			int resp=0;
			if((resp=libGPT_set_max_tokens(&cgpt, lmaxTokens))!=RETURN_OK)
				print_error("",libGPT_error(resp),FALSE);
			continue;
		}
		if(strstr(messagePrompted,"n;")==messagePrompted){
			char arg[16]="";
			for(int i=strlen("n;");i<strlen(messagePrompted) && i<16;i++) arg[i-strlen("n;")]=messagePrompted[i];
			char *tail=NULL;
			int ln=strtoul(arg,&tail,10);
			if(*tail!='\0'){
				print_error("'N' value not valid. ","",FALSE);
				continue;
			}
			if (arg[0]==0) ln=n;
			int resp=0;
			if((resp=libGPT_set_n(&cgpt, ln))!=RETURN_OK) print_error("",libGPT_error(resp),FALSE);
			continue;
		}
		if(strstr(messagePrompted,"fp;")==messagePrompted){
			char arg[16]="";
			for(int i=strlen("fp;");i<strlen(messagePrompted) && i<16;i++) arg[i-strlen("fp;")]=messagePrompted[i];
			char *tail=NULL;
			double lfreqPenalty=strtod(arg,&tail);
			if(*tail!='\0'){
				print_error("'Frequency Penalty' value not valid.","",FALSE);
				continue;
			}
			if (arg[0]==0) lfreqPenalty=freqPenalty;
			int resp=0;
			if((resp=libGPT_set_frequency_penalty(&cgpt, lfreqPenalty))!=RETURN_OK)
				print_error("",libGPT_error(resp),FALSE);
			continue;
		}
		if(strstr(messagePrompted,"pp;")==messagePrompted){
			char arg[16]="";
			for(int i=strlen("pp;");i<strlen(messagePrompted) && i<16;i++)
				arg[i-strlen("pp;")]=messagePrompted[i];
			char *tail=NULL;
			double lpresPenalty=strtod(arg,&tail);
			if(*tail!='\0'){
				print_error("'Presence Penalty' value not valid.","",FALSE);
				continue;
			}
			if (arg[0]==0) lpresPenalty=presPenalty;
			int resp=0;
			if((resp=libGPT_set_presence_penalty(&cgpt, lpresPenalty))!=RETURN_OK)
				print_error("",libGPT_error(resp),FALSE);
			continue;
		}
		if(strstr(messagePrompted,"t;")==messagePrompted){
			char arg[16]="";
			for(int i=strlen("t;");i<strlen(messagePrompted) && i<16;i++) arg[i-strlen("t;")]=messagePrompted[i];
			char *tail=NULL;
			double ltemperature=strtod(arg,&tail);
			if(*tail!='\0'){
				print_error("'Temperature' value not valid.","",FALSE);
				continue;
			}
			if (arg[0]==0) ltemperature=temperature;
			int resp=0;
			if((resp=libGPT_set_temperature(&cgpt, ltemperature))!=RETURN_OK)
				print_error("",libGPT_error(resp),FALSE);
			continue;
		}
		send_chat(cgpt, messagePrompted, responseVelocity, showModel, showFinishedStatus, showPromptTokens,
				showCompletionTokens, showTotalTokens, textToSpeech,logFile, checkStatus);
		add_history(messagePrompted);
	}while(TRUE);
	if(sessionFile!=NULL){
		if(libGPT_export_session_file(sessionFile)!=RETURN_OK)
			print_error("Error dumping session. ",libGPT_error(resp),FALSE);
	}
	if(textToSpeech) espeak_Terminate();
	libGPT_clean(&cgpt);
	rl_clear_history();
	printf("\n");
	exit(EXIT_SUCCESS);
}
