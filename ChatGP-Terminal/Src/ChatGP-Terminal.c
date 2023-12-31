/*
 ============================================================================
 Name        : ChatGP-Terminal.c
 Author      : L. (lucho-a.github.io)
 Version     : 1.2.3
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
#define PROGRAM_VERSION					"1.2.3"
#define PROGRAM_URL						"https://github.com/lucho-a/chatgp-terminal"
#define PROGRAM_CONTACT					"<https://lucho-a.github.io/>"

#define PROMPT_DEFAULT					"-> "
#define BANNER 							printf("\n%s%s v%s by L. <%s>%s\n\n",Colors.h_white,PROGRAM_NAME, PROGRAM_VERSION,PROGRAM_URL,Colors.def);

#define	DEFAULT_RESPONSE_VELOCITY		25000

struct SessionParams{
	char *model;
	double freqPenalty;
	int n;
	long int maxTokens;
	double presPenalty;
	double topP;
	double temperature;
}SessionParams;

struct Colors{
	char *yellow;
	char *h_green;
	char *h_red;
	char *h_cyan;
	char *h_white;
	char *def;
}Colors;

bool canceled=FALSE, exitProgram=FALSE;
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

int log_response(ChatGPTResponse *cgptResponse, char *logFile){
	time_t timestamp=libGPT_get_response_creation(cgptResponse);
	struct tm tm = *localtime(&timestamp);
	FILE *f=fopen(logFile,"a");
	if(f==NULL) return RETURN_ERROR;
	fprintf(f,"\"%d/%02d/%02d\"\t\"%02d:%02d:%02d\"\t",tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour,tm.tm_min, tm.tm_sec);
	if(libGPT_get_response_responses(cgptResponse)==1){
		fprintf(f,"%s\t%d\t%d\t%d\t%s\n",libGPT_get_response_finishReason(cgptResponse, 0),
				libGPT_get_response_promptTokens(cgptResponse),libGPT_get_response_completionTokens(cgptResponse),
				libGPT_get_response_totalTokens(cgptResponse),libGPT_get_response_model(cgptResponse));
	}else{
		fprintf(f,"%s\t%d\t%d\t%d\t%s\n","N/A",libGPT_get_response_promptTokens(cgptResponse),
				libGPT_get_response_completionTokens(cgptResponse),libGPT_get_response_totalTokens(cgptResponse),
				libGPT_get_response_model(cgptResponse));
	}
	fclose(f);
	return RETURN_OK;
}

void *speech_response(void *args){
	size_t len=strlen((char*)args);
	espeak_Synth((char*)args, len+1, 0, POS_CHARACTER, 0, espeakCHARS_AUTO, NULL, NULL);
	espeak_Synchronize();
	free(args);
	pthread_exit(NULL);
}

void print_response(ChatGPTResponse *cgptResponse, long int responseVelocity, bool showModel,
		bool alertFinishStatus, bool showFinishReason, bool showPromptTokens,bool showCompletionTokens, bool showTotalTokens){
	printf("%s",Colors.h_white);
	if(responseVelocity==0){
		for(int i=0;i<libGPT_get_response_responses(cgptResponse);i++){
			printf("%s",libGPT_get_response_contentFormatted(cgptResponse, i));
			if(showFinishReason && !canceled)
				printf("\n\n%sFinish status: %s%s\n",Colors.def,Colors.yellow,libGPT_get_response_finishReason(cgptResponse, i));
		}
	}else{
		for(int choice=0;choice<libGPT_get_response_responses(cgptResponse);choice++){
			char *choiceContent=libGPT_get_response_contentFormatted(cgptResponse, choice);
			printf("\n");
			(libGPT_get_response_responses(cgptResponse)>1)?printf("%s%d) ",Colors.h_white, choice+1):printf("%s",Colors.h_white);
			for(int i=0;choiceContent[i]!=0 && !canceled;i++){
				usleep(responseVelocity);
				printf("%c",choiceContent[i]);
				fflush(stdout);
			}
			printf("\n");
			if(alertFinishStatus){
				if(strcmp(libGPT_get_response_finishReason(cgptResponse, choice),"stop")!=0)
					printf("\n%sFinish status: %s%s\n",Colors.def,Colors.yellow,libGPT_get_response_finishReason(cgptResponse, choice));
			}
			if(showFinishReason && !canceled)
				printf("\n%sFinish status: %s%s\n",Colors.def,Colors.yellow,libGPT_get_response_finishReason(cgptResponse, choice));
		}
	}
	if(showModel) printf("%s\nModel: %s%s\n",Colors.def,Colors.yellow, libGPT_get_response_model(cgptResponse));
	if(showFinishReason && canceled) printf("%s\nFinish status: %scanceled by user\n",Colors.def,Colors.yellow);
	if(showPromptTokens) printf("%s\nPrompt tokens: %s%d\n",Colors.def,Colors.yellow, libGPT_get_response_promptTokens(cgptResponse));
	if(showCompletionTokens) printf("%s\nCompletion tokens: %s%d\n",Colors.def,Colors.yellow, libGPT_get_response_completionTokens(cgptResponse));
	if(showTotalTokens) printf("%s\nTotal tokens: %s%d\n",Colors.def,Colors.yellow, libGPT_get_response_totalTokens(cgptResponse));
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
	case SIGPIPE:
		print_error("'SIGPIPE' signal received: the write end of the pipe or socket is closed.","", FALSE);
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

void send_chat(ChatGPT *cgpt, char *message, long int responseVelocity, bool showModel, bool alertFinishedStatus
		,bool showFinishedStatus, bool showPromptTokens, bool showCompletionTokens, bool showTotalTokens,bool tts,
		char *logFile, bool checkStatus){
	ChatGPTResponse *cgptResponse=libGPT_getChatGPTResponse_instance();
	if(cgptResponse==NULL) print_error("Getting ChatGPTResponse instance error. ","", TRUE);
	int resp=0;
	if((resp=libGPT_send_chat(cgpt, cgptResponse, message))!=RETURN_OK){
		switch(resp){
		case LIBGPT_RESPONSE_MESSAGE_ERROR:
			print_error(libGPT_get_response_contentFormatted(cgptResponse, 0),"",FALSE);
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
			char *msg=NULL;
			msg=malloc(1);
			msg[0]=0;
			for(int i=0;i<libGPT_get_response_responses(cgptResponse);i++){
				msg=realloc(msg,strlen(msg)+strlen(libGPT_get_response_contentFormatted(cgptResponse, i))+2);
				if(msg==NULL) print_error("", libGPT_error(LIBGPT_REALLOC_ERROR),FALSE);
				strcat(msg,libGPT_get_response_contentFormatted(cgptResponse, i));
				strcat(msg,"\n");
			}
			pthread_t threadSpeechMessage;
			pthread_create(&threadSpeechMessage, NULL, speech_response, (void*)msg);
		}
		print_response(cgptResponse, responseVelocity, showModel, alertFinishedStatus, showFinishedStatus, showPromptTokens,
				showCompletionTokens,showTotalTokens);
		if(logFile!=NULL){
			if(log_response(cgptResponse, logFile)!=RETURN_OK) print_error("Error logging file. ",strerror(errno),FALSE);
		}
	}
	libGPT_free_response(cgptResponse);
}

int main(int argc, char *argv[]) {
	signal(SIGINT, signal_handler);
	signal(SIGPIPE, signal_handler);
	init_colors(FALSE);
	libGPT_init();
	ChatGPT *cgpt=libGPT_getChatGPT_instance();
	if(cgpt==NULL) print_error("Getting ChatGPT instance error. ","", TRUE);
	char *message=NULL,*saveMessagesTo="",*sessionFile=NULL,*logFile=NULL;
	size_t len=0;
	int resp=0, responseVelocity=DEFAULT_RESPONSE_VELOCITY;
	bool checkStatus=FALSE,alertFinishedStatus=FALSE,showFinishedStatus=FALSE,showPromptTokens=FALSE,
			showCompletionTokens=FALSE,showTotalTokens=FALSE,textToSpeech=FALSE,csv=FALSE,showModel=FALSE;
	for(int i=1;i<argc;i+=2){
		char *tail=NULL;
		if(strcmp(argv[i],"--version")==0){
			BANNER;
			exit(EXIT_SUCCESS);
		}
		if(strcmp(argv[i],"--apikeyfile")==0){
			char *apikey=NULL;
			if(i+1>=argc) print_error("--apikeyfile: option argument missing. ","",TRUE);
			FILE *f=fopen(argv[i+1],"r");
			if(f==NULL) print_error("Error opening API key file.",strerror(errno),TRUE);
			while(getline(&apikey, &len, f)!=-1);
			fclose(f);
			apikey[strlen(apikey)-1]=0;
			if((resp=libGPT_set_api(cgpt, apikey))!=RETURN_OK){
				free(apikey);
				print_error("",libGPT_error(resp),TRUE);
			}
			free(apikey);
			continue;
		}
		if(strcmp(argv[i],"--apikey")==0){
			if(i+1>=argc) print_error("--apikey: option argument missing. ","",TRUE);
			if((resp=libGPT_set_api(cgpt, argv[i+1]))!=RETURN_OK) print_error("",libGPT_error(resp),TRUE);
			continue;
		}
		if(strcmp(argv[i],"--role")==0){
			if(i+1>=argc) print_error("--role: option argument missing. ","",TRUE);
			if((resp=libGPT_set_role(cgpt, argv[i+1],NULL))!=RETURN_OK) print_error("",libGPT_error(resp),TRUE);
			continue;
		}
		if(strcmp(argv[i],"--max-tokens")==0){
			if(i+1>=argc) print_error("--max-tokens: option argument missing. ","",TRUE);
			int maxTokens=strtoul(argv[i+1],&tail,10);
			if(*tail!='\0') print_error("'Max.Tokens' value not valid. ","",TRUE);
			if((resp=libGPT_set_max_tokens(cgpt, maxTokens))!=RETURN_OK) print_error("",libGPT_error(resp),TRUE);
			continue;
		}
		if(strcmp(argv[i],"--freq-penalty")==0){
			if(i+1>=argc) print_error("--freq-penalty: option argument missing. ","",TRUE);
			double freqPenalty=strtod(argv[i+1],&tail);
			if(*tail!='\0') print_error("'Frequency Penalty' value not valid. ","",TRUE);
			if((resp=libGPT_set_frequency_penalty(cgpt, freqPenalty))!=RETURN_OK) print_error("",libGPT_error(resp),TRUE);
			continue;
		}
		if(strcmp(argv[i],"--pres-penalty")==0){
			if(i+1>=argc) print_error("--pres-penalty: option argument missing. ","",TRUE);
			double presPenalty=strtod(argv[i+1],&tail);
			if(*tail!='\0') print_error("'Presence Penalty' value not valid. ","",TRUE);
			if((resp=libGPT_set_presence_penalty(cgpt, presPenalty))!=RETURN_OK) print_error("",libGPT_error(resp),TRUE);
			continue;
		}
		if(strcmp(argv[i],"--top-p")==0){
			if(i+1>=argc) print_error("--top-p: option argument missing. ","",TRUE);
			double topP=strtod(argv[i+1],&tail);
			if(*tail!='\0') print_error("'Top P' value not valid. ","",TRUE);
			if((resp=libGPT_set_top_p(cgpt, topP))!=RETURN_OK) print_error("",libGPT_error(resp),TRUE);
			continue;
		}
		if(strcmp(argv[i],"--temperature")==0){
			if(i+1>=argc) print_error("--temperature: option argument missing. ","",TRUE);
			double temperature=strtod(argv[i+1],&tail);
			if(*tail!='\0') print_error("'Temperature' value not valid. ","",TRUE);
			if((resp=libGPT_set_temperature(cgpt, temperature))!=RETURN_OK) print_error("",libGPT_error(resp),TRUE);
			continue;
		}
		if(strcmp(argv[i],"--n")==0){
			if(i+1>=argc) print_error("--n: option argument missing. ","",TRUE);
			int n=strtoul(argv[i+1],&tail,10);
			if(*tail!='\0') print_error("'N' value not valid. ","",TRUE);
			if((resp=libGPT_set_n(cgpt, n))!=RETURN_OK) print_error("",libGPT_error(resp),TRUE);
			continue;
		}
		if(strcmp(argv[i],"--timeout")==0){
			if(i+1>=argc) print_error("--timeout: option argument missing. ","",TRUE);
			long int timeout=strtol(argv[i+1],&tail,10);
			if(*tail!='\0' || libGPT_set_timeout(timeout)!=RETURN_OK) print_error("'Timeout' value not valid. ","",TRUE);
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
			int maxContextMessages=strtoul(argv[i+1],&tail,10);
			if(*tail!='\0') print_error("'Max. Context Messages' value not valid. ","",TRUE);
			if((resp=libGPT_set_max_context_msgs(maxContextMessages))!=RETURN_OK) print_error("",libGPT_error(resp),TRUE);
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
			if((resp=libGPT_set_role(cgpt,NULL, argv[i+1]))!=RETURN_OK) print_error("",libGPT_error(resp),TRUE);
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
			if((resp=libGPT_set_model(cgpt, argv[i+1]))!=RETURN_OK) print_error("",libGPT_error(resp),TRUE);
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
		if(strcmp(argv[i],"--alert-finished-status")==0){
			alertFinishedStatus=TRUE;
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
	if(sessionFile!=NULL){
		if((resp=libGPT_import_session_file(sessionFile))!=RETURN_OK)
			print_error("Error importing session file. ",libGPT_error(resp),FALSE);
	}
	if(message!=NULL){
		init_colors(TRUE);
		send_chat(cgpt, message, 0, showModel, alertFinishedStatus, showFinishedStatus, showPromptTokens, showCompletionTokens,
				showTotalTokens, FALSE, logFile, checkStatus);
		libGPT_free(cgpt);
		exit(EXIT_SUCCESS);
	}
	if(checkStatus) check_service_status();
	rl_getc_function=readline_input;
	struct SessionParams sp;
	sp.model=malloc(strlen(libGPT_get_model(cgpt))+1);
	snprintf(sp.model,strlen(libGPT_get_model(cgpt))+1,"%s",libGPT_get_model(cgpt));
	sp.freqPenalty=libGPT_get_frequency_penalty(cgpt);
	sp.maxTokens=libGPT_get_max_tokens(cgpt);
	sp.n=libGPT_get_n(cgpt);
	sp.presPenalty=libGPT_get_presence_penalty(cgpt);
	sp.temperature=libGPT_get_temperature(cgpt);
	sp.topP=libGPT_get_top_p(cgpt);
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
			if (arg[0]==0) strcpy(arg,sp.model);
			int resp=0;
			if((resp=libGPT_set_model(cgpt, arg))!=RETURN_OK) print_error("",libGPT_error(resp),FALSE);
			continue;
		}
		if(strstr(messagePrompted,"to;")==messagePrompted){
			char arg[16]="";
			for(int i=strlen("to;");i<strlen(messagePrompted) && i<16;i++) arg[i-strlen("to;")]=messagePrompted[i];
			char *tail=NULL;
			long int to=strtoul(arg,&tail,10);
			if(*tail!='\0'){
				print_error("'Timeout' value not valid. ","",FALSE);
				continue;
			}
			if (arg[0]==0) to=0;
			int resp=0;
			if((resp=libGPT_set_timeout(to))!=RETURN_OK) print_error("",libGPT_error(resp),FALSE);
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
			if (arg[0]==0) lmaxTokens = sp.maxTokens;
			int resp=0;
			if((resp=libGPT_set_max_tokens(cgpt, lmaxTokens))!=RETURN_OK) print_error("",libGPT_error(resp),FALSE);
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
			if (arg[0]==0) ln=sp.n;
			int resp=0;
			if((resp=libGPT_set_n(cgpt, ln))!=RETURN_OK) print_error("",libGPT_error(resp),FALSE);
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
			if (arg[0]==0) lfreqPenalty=sp.freqPenalty;
			int resp=0;
			if((resp=libGPT_set_frequency_penalty(cgpt, lfreqPenalty))!=RETURN_OK) print_error("",libGPT_error(resp),FALSE);
			continue;
		}
		if(strstr(messagePrompted,"pp;")==messagePrompted){
			char arg[16]="";
			for(int i=strlen("pp;");i<strlen(messagePrompted) && i<16;i++) arg[i-strlen("pp;")]=messagePrompted[i];
			char *tail=NULL;
			double lpresPenalty=strtod(arg,&tail);
			if(*tail!='\0'){
				print_error("'Presence Penalty' value not valid.","",FALSE);
				continue;
			}
			if (arg[0]==0) lpresPenalty=sp.presPenalty;
			int resp=0;
			if((resp=libGPT_set_presence_penalty(cgpt, lpresPenalty))!=RETURN_OK) print_error("",libGPT_error(resp),FALSE);
			continue;
		}
		if(strstr(messagePrompted,"tp;")==messagePrompted){
			char arg[16]="";
			for(int i=strlen("tp;");i<strlen(messagePrompted) && i<16;i++) arg[i-strlen("tp;")]=messagePrompted[i];
			char *tail=NULL;
			double ltopP=strtod(arg,&tail);
			if(*tail!='\0'){
				print_error("'Top P' value not valid.","",FALSE);
				continue;
			}
			if (arg[0]==0) ltopP=sp.topP;
			int resp=0;
			if((resp=libGPT_set_top_p(cgpt, ltopP))!=RETURN_OK) print_error("",libGPT_error(resp),FALSE);
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
			if (arg[0]==0) ltemperature=sp.temperature;
			int resp=0;
			if((resp=libGPT_set_temperature(cgpt, ltemperature))!=RETURN_OK) print_error("",libGPT_error(resp),FALSE);
			continue;
		}
		if(strstr(messagePrompted,"params;")==messagePrompted){
			printf("\n%sModel: %s%s",Colors.h_white, Colors.def, libGPT_get_model(cgpt));
			printf("\n%sMax. Tokens: %s%ld",Colors.h_white, Colors.def,libGPT_get_max_tokens(cgpt));
			printf("\n%sN: %s%d",Colors.h_white, Colors.def,libGPT_get_n(cgpt));
			printf("\n%sFreq. Penalty: %s%.4f",Colors.h_white, Colors.def,libGPT_get_frequency_penalty(cgpt));
			printf("\n%sPres. Penalty: %s%.4f",Colors.h_white, Colors.def,libGPT_get_presence_penalty(cgpt));
			printf("\n%sTemp.: %s%.4f",Colors.h_white, Colors.def,libGPT_get_temperature(cgpt));
			printf("\n%sTop P: %s%.4f\n",Colors.h_white, Colors.def,libGPT_get_top_p(cgpt));
			continue;
		}
		send_chat(cgpt, messagePrompted, responseVelocity, showModel, alertFinishedStatus, showFinishedStatus, showPromptTokens,
				showCompletionTokens, showTotalTokens, textToSpeech,logFile, checkStatus);
		add_history(messagePrompted);
	}while(TRUE);
	if(sessionFile!=NULL){
		if(libGPT_export_session_file(sessionFile)!=RETURN_OK) print_error("Error dumping session. ",libGPT_error(resp),FALSE);
	}
	if(textToSpeech) espeak_Terminate();
	libGPT_free(cgpt);
	free(sp.model);
	rl_clear_history();
	printf("\n");
	exit(EXIT_SUCCESS);
}
