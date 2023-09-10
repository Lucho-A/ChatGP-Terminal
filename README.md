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
chatgp-terminal --apikeyfile ~/.cgpt/.cgpt.key --max-context-messages 5 --max-tokens 1024 --role "Act as IT professional" --save-messages-to ~/.cgpt/cgpt.msg --session-file ~/.cgpt/cgpt.session --log-file ~/.cgpt/cgpt.log
```

Into the man page you can find others.

### Screenshots:

![imagen](https://github.com/Lucho-A/ChatGP-Terminal/assets/40904281/721de418-ebee-47f4-b64a-29df0d538de2)

![imagen](https://github.com/Lucho-A/ChatGP-Terminal/assets/40904281/d053e91c-4e4e-4824-b2bd-f3649e3c1e5a)

![imagen](https://github.com/Lucho-A/ChatGP-Terminal/assets/40904281/95840753-d059-4ba2-b051-b64bb33f69e7)

![imagen](https://github.com/Lucho-A/ChatGP-Terminal/assets/40904281/0bf3b601-b7d1-44d7-879c-652b654a08d4)

