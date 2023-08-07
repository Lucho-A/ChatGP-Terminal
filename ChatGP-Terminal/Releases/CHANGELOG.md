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

Feature:
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

Feature:
- Accepts as option '--no-create-context'. If set, a context (historical of user's prompt and responses) won't be generated (FALSE by default).

Others:
- code optimization & code cleaned-up

#### v1.0.5:20230722

Feature:
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
