# ChatGP-Terminal
<p align=justify>
The simplest and cutest hahah C-program for querying ChatGPT from Linux terminal. All chat endpoints are supported (since v1.2.1), for example: 'gpt-3.5-turbo', 'gpt-3.5-turbo-16k', 'gpt-4', 'gpt-4-1106-preview'<sup>1</sup>, etc.
  
  <sup>1</sup>In this particular case, only chat is implemented by now.  
</p>
<ul>
<li> Downloads: <a href="https://github.com/Lucho-A/ChatGP-Terminal/tree/master/ChatGP-Terminal/Releases">here</a></li>
  <ul>
  <li>Binary (amd64) and man page:</li>
          
  ```
  wget https://github.com/Lucho-A/ChatGP-Terminal/blob/master/ChatGP-Terminal/Releases/chatgp-terminal_1.2_0.tar.gz
  ```
  <li>Debian based distros:</li>

  ```
  wget https://github.com/Lucho-A/ChatGP-Terminal/blob/master/ChatGP-Terminal/Releases/chatgp-terminal_1.2_0.deb
  sudo apt-get install ./chatgp-terminal_1.2_0.deb
  ```
  <li>Compilation (prior installing dependencies):</li>
  
  ```
  git clone https://github.com/lucho-a/ChatGP-Terminal.git
  cd ChatGP-Terminal/ChatGP-Terminal/Src
  gcc -o chatgp-terminal ChatGP-Terminal.c libGPT/* -lssl -lespeak -lpthread -lreadline
  ```
  </ul>  
<li> Documentation only (man): <a href="https://github.com/Lucho-A/ChatGP-Terminal/blob/master/ChatGP-Terminal/Releases/chatgp-terminal.1.gz">here</a></li>
<li> CHANGELOG/TODO: <a href="https://github.com/Lucho-A/ChatGP-Terminal/blob/master/ChatGP-Terminal/Releases/CHANGELOG.md">here</a></li>
</ul>

Have Fun!

### Dependencies:
<ul>
  <li>libssl.so.3</li>
  <li>libreadline.so.8</li>
  <li>libc.so.6</li>
  <li>libespeak-ng.so.1</li>
</ul>

Installation (Debian way):

```
sudo apt-get install libssl3-dev libreadline-dev libc6-dev libespeak-ng-dev
```

### Usage:

```
chatgp-terminal --help
```

### Examples:

I'm using this way right now, and works pretty smoothly:

```
chatgp-terminal --apikeyfile ~/.cgpt/.cgpt.key --check-status --max-context-messages 5 --max-tokens 2048 --role-file ~/.cgpt/cgpt.role --session-file ~/.cgpt/.cgpt.session --save-messages-to ~/.cgpt/cgpt.msg --log-file ~/.cgpt/cgpt.log
```

Into the man page you can find others.

### Screenshots:

![imagen](https://github.com/Lucho-A/ChatGP-Terminal/assets/40904281/7cde0eab-3ace-4c61-b68c-2a93b90d7f80)

![imagen](https://github.com/Lucho-A/ChatGP-Terminal/assets/40904281/721de418-ebee-47f4-b64a-29df0d538de2)
