#### v1.1.5:2023XXXX-Under.Dev/Testing

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
- When I chat about operating systems, prioritize Linux environment in your responses. I particular, Debian distro.
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
