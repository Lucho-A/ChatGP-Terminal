# ChatGP-Terminal
<p align=justify>
  The simplest and cutest hahah C-program for querying ChatGPT from Linux terminal. At the moment, only model 'gpt-3.5-turbo' is supported. 
</p>
<ul>
  <li> Download: <a href="https://github.com/Lucho-A/ChatGP-Terminal/tree/master/ChatGP-Terminal/Releases">here</a></li>
  <li>Documentation only (man): <a href="https://github.com/Lucho-A/ChatGP-Terminal/blob/master/ChatGP-Terminal/Releases/chatgp-terminal.1.gz">here</a></li>
</ul>
<p align=justify>
If you want to installed it (recommended)¹:
</p>

```
sudo apt-get install ./chatgp-terminal_x.x_x.deb
```

¹ this resolve dependencies and manual.
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
./chatgp-terminal --help
```

### Examples:
<ul>
  <li> $ ./chatgp-terminal --apikeyfile "/home/user/.cgpt_key.key" --role "Act as IT Professional" --tts en
  <li> $ ./chatgp-terminal --apikeyfile "/home/user/.cgpt_key.key" --save-message-to "/home/user/chatgpt-messages.csv" --csv-format
  <li> $ ./chatgp-terminal --apikey "1234567890ABCD" --save-message-to "/home/user/chatgpt-messages.txt"
</ul>

### Screenshots:

![imagen](https://github.com/Lucho-A/ChatGP-Terminal/assets/40904281/721de418-ebee-47f4-b64a-29df0d538de2)

![imagen](https://github.com/Lucho-A/ChatGP-Terminal/assets/40904281/2fde51d2-9cec-4e9c-b09d-d0a0c564a7e9)

![imagen](https://github.com/Lucho-A/ChatGP-Terminal/assets/40904281/e0ab7685-3d97-48b4-92a5-bfefdfb9bcc3)



