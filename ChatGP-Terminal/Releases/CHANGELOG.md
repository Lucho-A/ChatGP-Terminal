#### v1.2.3:20231225-Christmas Edition hhaha

- SHA2-256(chatgp-terminal_1.2_3.deb)=2d2d128c384a8903ccb37390490d86c1e0384b97fb705d8d06a7259f2329808d
- SHA2-256(chatgp-terminal_1.2_3.tar.gz)=9837405d2a6b037e7f2a8c5d48c77178f11c6cc0950a92abe242a37c73816c21

Features:
- Added option 'params;' when prompting, for showing the parameters set up.

Bug-Fixed:
- catch SIGPIPE signal. In some -rare- cases, I found that 'libcrypto.so.3' could raise the exception.

Others:
- data abstraction implemented.
- minor changes, code optimized & code cleaned-up.

#### v1.2.2:20231128

- SHA2-256(chatgp-terminal_1.2_2.deb)=07ae1396bc1f9c97dda26d10bc1a74b5d002f7cddc7b089bcce9e529596720ba
- SHA2-256(chatgp-terminal_1.2_2.tar.gz)=77a8768c83c2a55bf14ac07f0a6ae0bde442b3508439a4b5324763da6df9ebc1

Features:
- Added option '--top-p'. It allows to incorporate a 'Top P' value. Default 1.0.
- Added option 'tp; [value]' when prompting, for changing the 'top p' parameter.
- Added option '--timeout'. For setting the timeout of responses (in ms). Default 60000.
- Added option 'to; [value]' when prompting, for changing the timeout. Zero or empty value assign the default one.
- Added option '--alert-finished-status': shows the finished status only if finished status != "stop".

Improvements:
- shows all the choices received (n>1). Because each choice has its own 'finished response', if choice > 1, 'N/A' will be logged (if '--log-file') into this field. In the same line, '--show-finished-status' shows information for each choice.

Note: all the choices are going to be added as a single context message instead of different ones. This could be trivial but is not. Anyway, at the moment, I found this the best approach. Keep in mind the cost involved, and that, in order to avoid several responses (one for each choice) in following messages, you should choose one of the previous choices and handling 'n' parameter according.

Bug-Fixed:
- solved incorrect free().

Others:
- minor changes & code cleaned-up.

#### v1.2.1:20231119

- SHA2-256(chatgp-terminal_1.2_1.deb)=c729d9148ffcfbf22ebbb54e4f5e5078d5dad7bc1b1a66e22a092faeb454c0a7
- SHA2-256(chatgp-terminal_1.2_1.tar.gz)=4b1dc7faa1eba07c34d25582e81aecd3428fb95c414054d90486e4d52a1590f6

Features:
- Added option '--model [value]'. It allows to specify the model to use. (1)
- Added option '--show-model'. It shows the model used in the response (v.gr. 'gpt-3.5-turbo-0613', 'gpt-4-0613', etc.).
- Added option 'model; [value]' when prompting for changing the model.
- Added option '--pres-penalty'. It allows to incorporate a 'Presence Penalty' value. Default 0.0.
- Added option 'pp; [value]' when prompting, in order to change the 'presence penalty' parameter. An empty value assign the 'session value'.
- Added option '--uncolored'. Yes, you guessed hhahah stdout uncolored. When '--message' is specified, the output is always uncolored. Default: false.

Others:
- in order to simplify code, and better maintenance/scalability, the validation of the parameter's values (boundaries) are done in server side. For similar reasons, as long as now it's possible to use any chat model, '--show-cost' was removed. Just for the records, imo, this info. should be part of the responses. (2)
- now it's possible to assign the value 0 to '--response-velocity' parameter (no 'typing' effect). Default value: 25000
- the random term at the moment of printing out the response was removed.
- '--log-file' adds to log the model used in the response (and the cost is not longer logged).
- minor changes & code cleaned-up

(1) in the particular case of 'gpt-4-vision-preview' uploading images isn't implemented.

(2) unless ur expecting chatgpt write a novel for you hahhah (pretty sad, btw, isn't it??), just use the model 'gpt-3.5-turbo' (default). It's, by far, the best relation between results and cost.

#### v1.2.0:20231111

- SHA2-256(chatgp-terminal_1.2_0.deb)=7c0f52474e00b4dfa109bd5c8b021516ebf153d842c7fd38afee4cfae42f0386
- SHA2-256(chatgp-terminal_1.2_0.tar.gz)=3e210be9c76024ae6e8b9b9a0c9a96b5058200e354832293795f597d36490bf8

Features:
- Added option '--freq-penalty'. Allow to incorporate a 'Frequency Penalty' (-2.0 to 2.0) value. Default 0.0.
- Added option '--n'. Allow to incorporate an 'N' (1 to 20) value. Default 1.
- Now, it's possible to prompt and change some parameters 'on-the-fly' (I find it useful when a response doesn't fulfill my expectations and I want to query again). Namely:
    - 'n; [value]'  // for N parameter
    - 'fp; [value]' // for Frequency Penalty parameter
    - 'mt; [value]' // for Max. Tokens parameter
    - 't; [value]'  // for Temperature parameter

    If value is empty, the parameter adopts the value assigned when the program started (session value).

Bug-Fixed:
- fixed segmentation fault when option argument is missing
- fixed input errors & validations handling

Others:
- Input cost updated: 0.0010
- code optimized, minor changes & code cleaned-up

#### v1.1.9:20231110

- SHA2-256(chatgp-terminal_1.1_9.deb)=6e3752a20eefcba4effcc8e5ef9f651700b0d98544c9a2c4fc798f245f7ae260
- SHA2-256(chatgp-terminal_1.1_9.tar.gz)=a34aeb603c9a375c4e2d6f4d15aff616b4ff7990a58a2dfc813e1df056d58f74

Features:
- Added option '--check-status'. Check the status service when start the program.
- prompting 'status;', show the status of the service.

Bug-Fixed:
- fixed segmentation fault when JSON with unexpected format responses is received.

Others:
- Changed default max. tokens -> 1024
- code optimized, minor changes & code cleaned-up

#### v1.1.8:20231022

- SHA2-256(chatgp-terminal_1.1_8.deb)=c6a33fdc45d13695789bab6249efd11212969eef28699b2bc3ae023797d7ded7
- SHA2-256(chatgp-terminal_1.1_8.tar.gz)=7039f96c8d7adb4c83dd6604d4e701daf6baeaa84484707fe7d96188eb8d007b

Bug-Fixed:
- fixed free arrays avoiding uncommon segmentation fault when communication error occurs.

#### v1.1.7:20230919

- SHA2-256(chatgp-terminal_1.1_7.deb)=ea1597b1b0e4b6af62ec244e242f2e22b598f40987877ec7c06a254e5f4a6214
- SHA2-256(chatgp-terminal_1.1_7.tar.gz)=e999c06ae3e5fcc6538dddb56f1a5b856c2fd9623328856b8ef1ffeb7a7b4e11

Improvements:
- multi-line editing improved.

Others:
- for practicality, and in order to achieve more and best compatibility among platforms, I changed the way for generating newlines and exiting the program. Now, in order to do it, just 'alt+enter' must be pressed (exit when prompt empty). Btw, from Windows, I encourage you to use "Terminal Windows". You can install it from the Ms Store and, among others nice features, the keyboard setting actions is very straight forward.
- prompt string changed.
- code optimized, minor changes & code cleaned-up

#### v1.1.6:20230917

- SHA2-256(chatgp-terminal_1.1_6.deb)=078c06c73b747ebe113295f1391e44f390043245ac4aa63e21a548fa17f97fd7
- SHA2-256(chatgp-terminal_1.1_6.tar.gz)=0031356b4272146c9235b7331e44b62110d0781865da2268d9e6282a5ea2b6df

Improvements:
- Into prompt 'Shift+Enter' create a newline without submitting (1)
- Prompt allows inserting tabs ('\t')

(1) if you use WSL2 terminal, take into account that you could need to key-binding this key combination.

Others:
- code optimized, minor changes & code cleaned-up

#### v1.1.5:20230913

- SHA2-256(chatgp-terminal_1.1_5.deb)=c5dcbe59ace69d6db848c2dc42a2c196293998227bcd9d2afa0bb4290a701b2c
- SHA2-256(chatgp-terminal_1.1_5.tar.gz)=c4733281b5b14b0c1afdd53ded719711095d85bf2bcc32bd66d62b913362d330

Bug-Fixed:
- '--session-file': fixed creating session file if it doesn't exist when program starts.

Others:
- Error handling improved
- code optimized, minor changes & code cleaned-up

#### v1.1.4:20230912

Hashes:
- SHA2-256(chatgp-terminal_1.1_4.deb)=f0f6bdc65a3ef4345acf21b1e24ea3a338e4a6c52bd24428bba694a2f996fc5e
- SHA2-256(chatgp-terminal_1.1_4.tar.gz)=d1013841ef23b9b2a6e3591fe73b94ab8ea47a43ab6db186d3347c70a69f24fe

Features:
- '--role-file': according to some tests I'm performing, I'm finding very useful incorporating contextual information into the 'system role', as long as I can give particular instructions, for example:

```
Act as IT professional, specialist in security, and tourist guide; and take the following into consideration:
- When I chat about operating systems, prioritize Linux environment in your responses. In particular, Debian distro.
- When I request for places to go out, take into account that I'm planning going to Paris in holidays.
```

So, I added this option in order to incorporate static/read-only contextual environment during the entire session, independent from the history context. In this regard, let's say that this information complements the one that you can include using '--role' option (I'm evaluating the convenience, or not, to keep this option). However, I recommend not to use both and using this way.

Example usage: '--role-file ~/.cgpt/cgpt.role'.

Bug-Fixed:
- '--temperature': fixed upper boundary (2.0) validation.
- '--tts': fixed validation when the option is not placed at the end of the command. Now, selecting a language is mandatory.

Improvements:
- For scripting reasons, among others, in order to allow the using of the output of '--message' to other commands ('grep', for example), '--response-velocity' and 'tts' options are not taking into account when this option is specified (no delay in responses, always FALSE, respectively). Finally, the output is uncolored.

Others:
- code optimized, minor changes & code cleaned-up

#### v1.1.3:20230909

Hashes:
- SHA2-256(chatgp-terminal_1.1_3.deb)= c6e3d5f78ec86d91fa2f5077bfffcafa0adaf11de6f6bcae5670f2246f81273f
- SHA2-256(chatgp-terminal_1.1_3.tar.gz)= 240ca8e82df21b1b0bb1f5811be918f1d8642aceec5a0dee1d2f96e2331fab70

Features:
- option '--session-file' added. If this option is specified, all the interactions user/assistant will be dumped (when the program is closed) and imported (at the start) to/from the file defined. The number of interactions dumped/imported (newest ones) will be defined by '--max-context-messages' (default 3). If '--message' option is included, this one won't has effect. Take into account the openai restrictions, and the costs involved. In this regard, if you exceed the maximum tokens allowed in a message (4k/16k tokens) you'll get a message form openai reporting the situation (as any other error message). If this situation arise, you must flush (flush;) (and lose) the history context in order to continue querying (or closing the program, manually editing the session file, and opening it again).
- option '--show-cost' added. Show the cost approx. associated to the response. Values up today (20230909). It seems to be pretty accuracy, however, always check against openai dashboard.
- option '--log-file' added. In order to have information for testing/evaluations, statistics, etc., I created this option for logging all the responses' information (except the user's and assistant's messages). Fields: creation_date, creation_time, finish_reason, prompt_tokens, completion_tokens, total_tokens, cost. Delimiter: '\t'.

Bug-Fixed:
- fixed some json miss-parsing from user's message.

Others:
- in order to avoid/minimize the risk of delimiter/content-mistakes, I changed the delimiter character for '--csv-format' option, from ';' to '	' (\t);
- code optimized, minor changes & code cleaned-up

#### v1.1.2:20230907

Features:
- option '--tts' added. This option allows to speech the response in a selected language (can be cancelled with Crl+C). Dependency: libespeak-ng1. Usage: --tts [es|en|fr...]
- option '--csv-format' added. In order to allow generating a database of useful or particular responses (1), this option allows to save ('save;') records in delimited (';') format, in a file specified with '--save-messages-to'.

Others:
- default velocity response default value change to 10000
- code optimized, minor changes & code cleaned-up

(1) Actually, I'm working on incorporate, in a future release, an 'import' option in order to allow populating the history at the start of the program. 

#### v1.1.1:20230905

Features:
- option '--save-message-to' added. This option allows to specify a file (v.gr. '/home/user/chatgpt-messages.txt') in order to save the latest user & assistant message entering 'save;' into prompt. 

Others:
- code optimization, minor changes & code cleaned-up

#### v1.1.0:20230807

Bug-Fixed:
- fixed 'segmentation fault' when '--max-context-messages 1'

Others:
- removed option '--no-create-context'. You can obtain the same result setting '--max-context-messages 0'. 

#### v1.0.9:20230727

Others:
- changed '--response-velocity' default value to 20000
- changed '--max-context-messages' default value to 3 
- changed '--max-tokens' default value to 512 
- code optimization, minor changes & code cleaned-up

#### v1.0.8:20230725

Features:
- option '--max-context-messages'. Maximum number of chat history (5 as default). The program keep track of the latest N messages sent/responded in order to obtain a better chat experience. Take into account that the tokens in 'context' messages are part of the cost (see show-total-tokens).
- option '--show-prompt-tokens'. No argument required. If the option is specified, shows the tokens include in the prompt (user messages). This value included the previous prompts in  history (users, and assistant ones).
- option '--show-completion-tokens'. No argument required. If the option is specified, shows the tokens include in the response.
- 'flush;' command. Typing 'flush;' into the prompt, flush the history of messages.

Others:
- code optimization & code cleaned-up

#### v1.0.7:20230723

Bug-Fixed:
- parsed message fixed.
- crashes fixed.

Others:
- code optimization & code cleaned-up

#### v1.0.6:20230723

Features:
- Accepts as option '--no-create-context'. If set, a context (historical of user's prompt and responses) won't be generated (FALSE by default).

Others:
- code optimization & code cleaned-up

#### v1.0.5:20230722

Features:
- Accepts as option '--show-finished-status'. If set, shows the response 'Total tokens' at the end of the response

Others:
- increased default timeouting to 60s
- code optimization, minor changes & code cleaned-up

#### v1.0.4:20230720

Bugs-Fixed:
- command argument input validations 
- user prompt string parsed fixed when '\n'

Others:
- improved string memory assignment.
- minor changes & code cleaned-up

#### v1.0.3:20230719

Bugs-Fixed:
- (I think) prompt (user's input) & readline() prompt solved
- Others like '%' on output

Others:
- default receiving message timeouting increased (35s)
- minor changes & code cleaned-up

#### v1.0.2:20230719

Improvements:
- Accepts as option '--show-finished-status'. If set, shows the status at the end of the answers.
	
Bugs-Fixed:
- Some I don't remember hhaha

Others:
- Show 'errno' description if getting info errors occurs.
- minor changes & code cleaned-up

#### v1.0.1:20230719

Improvements:
- allowing cancel by user handling signal ('SIGINT').
	
Bugs-Fixed:
- a lot (string parsings, data validations, error messages, not exit if errors, etc.).

Bugs-Known:
- readline() has some troubles for handling "e[0;XXm". I replaced #defines by "\033[0;XXm" in order to evaluate if behavior changes.
- JSON content message errors because absence of validation and parsing of user's messages ('?','\', etc). 

#### v1.0.0:20230718
- First version.
