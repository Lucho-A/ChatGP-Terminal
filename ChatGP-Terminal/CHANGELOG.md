#### v1.0.1:2023XXXX-Testing

Improvements:

- allowing cancel by user handling signal ('SIGINT').
	
Bugs-Fixed:

- a lot (string parsings, data validations, error messages, not exit if errors, etc.).

Bugs-Known:

- readline() has some troubles for handling "e[0;XXm". I replaced #defines by "\033[0;XXm" in order to evaluate if behavior changes.
- JSON content message errors because absence of validation and parsing of user's messages ('?','\', etc). 

#### v1.0.0:20230718

- First version.
