
# Emmet
Advanced malware program written in C++, with it you can take over control of the potential victim's computer. For now Emmet is working only on Windows. Available features:
- VNC
- Catching Keystrokes
- Remote CMD command execution
- Self-Updater
- Auto-Destruct

## Libraries
I recommend to install all necessary libraries via [vcpkg](https://github.com/microsoft/vcpkg).
- [socket.io-client-cpp](https://katherineoelsner.com/)
- GdiPlus
- UrlMon

## KL-DASHBOARD
[KL-DASHBOARD](https://github.com/koloksk/KL-Dashboard) is needed WEB GUI Tool if you want to control clients. So I suggest you to firstly install the Web Panel then start creating clients.

## Before compilation
Remember to set correct IP Address of server where Web Panel is hosted ```const std::string server``` in ```Emmet.h``` file.

## Usage
After you compiled program you will have *.exe* file. There are two ways you can run it.
First way is to run it without any parameters and since you run it Emmet should appear in KL-DASHBOARD panel in ready to use status.
```bash
Emmet.exe
```
or you can run it with parameter *-i* which will run installation process where Emmet is installing itself on the computer and adding program to TaskScheduler to run it every time on logon.
```bash
Emmet.exe -i
```

**Now from version 2.4 you can define your own destination for file.**
```bash
Emmet.exe -i "C:\'Program Files'\MyFolder\NewEmmet.exe"
```
Yes if you choose destination that includes folder with white spaces you need to put it in quotes like here: ```'Program Files'```.

## License

[GPL-3.0](https://choosealicense.com/licenses/gpl-3.0/)
