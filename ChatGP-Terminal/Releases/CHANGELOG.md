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