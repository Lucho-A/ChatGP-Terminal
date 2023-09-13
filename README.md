# ChatGP-Terminal
<p align=justify>
The simplest and cutest hahah C-program for querying ChatGPT from Linux terminal. At the moment, only model 'gpt-3.5-turbo' is supported.
</p>
<ul>
  <li> Download: <a href="https://github.com/Lucho-A/ChatGP-Terminal/tree/master/ChatGP-Terminal/Releases">here</a></li>
  <li> Documentation only (man): <a href="https://github.com/Lucho-A/ChatGP-Terminal/blob/master/ChatGP-Terminal/Releases/chatgp-terminal.1.gz">here</a></li>
  <li> CHANGELOG/TODO: <a href="https://github.com/Lucho-A/ChatGP-Terminal/blob/master/ChatGP-Terminal/Releases/CHANGELOG.md">here</a></li>
</ul>
<p align=justify>
If you want to install it (recommended for resolving dependencies and man page):
</p>

```
sudo apt-get install ./chatgp-terminal_x.x_x.deb
```

<p>
Have Fun!
</p>

### Dependencies:
<ul>
  <li>libssl.so.3</li>
  <li>libreadline.so.8</li>
  <li>libc.so.6</li>
  <li>libespeak-ng.so.1</li>
</ul>

If you didn't install the package (deb), you can install the dependencies by executing:

```
sudo apt-get install libssl3 libreadline8 libc6 libespeak-ng1
```

### Usage:

```
chatgp-terminal --help
```

### Examples:

I'm using this way right now, and works pretty smoothly:

```
chatgp-terminal --apikeyfile ~/.cgpt/.cgpt.key --max-context-messages 5 --max-tokens 1024 --role-file ~/.cgpt/cgpt.role --session-file ~/.cgpt/.cgpt.session --save-messages-to ~/.cgpt/cgpt.msg --log-file ~/.cgpt/cgpt.log
```

Into the man page you can find others.

### Screenshots:

![imagen](https://github.com/Lucho-A/ChatGP-Terminal/assets/40904281/721de418-ebee-47f4-b64a-29df0d538de2)

![imagen](https://github.com/Lucho-A/ChatGP-Terminal/assets/40904281/d73bcd03-1108-4e3b-9e99-2517c41db5da)

![imagen](https://github.com/Lucho-A/ChatGP-Terminal/assets/40904281/06eafc60-b6dc-4b4c-95ac-8d353b0317cf)

![imagen](https://github.com/Lucho-A/ChatGP-Terminal/assets/40904281/b48aed4c-4dee-4574-aca7-dbc2e0246ba1)
