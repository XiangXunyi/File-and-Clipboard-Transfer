#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "comdlg32.lib")

// Must compile with -lws2_32 -lcomdlg32

// Version 1.1
const int versionId = 10001;

void reciever(SOCKET);

void handleClient(SOCKET);

void checkIfUserIsOnline(int);

char username[64][1024];
unsigned long ip[64];
int userStatu[64];
char groupname[64][1024] = { "everyone" };
unsigned long long usersInGroup[64];

int main(int argc, char** argv)
{
	fputs("Welcome to the File and Clipboard Transfer 1.1 !\n", stderr);
	fputs("Type \"help\" for a list of available commands.\n", stderr);

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		fputs("WSAStartup failed.\n", stderr);
		return 1;
	}

	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
	{
		fputs("socket failed.\n", stderr);
		WSACleanup();
		return 1;
	}

	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(11451);
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	if (bind(sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		fputs("bind failed.\n", stderr);
		closesocket(sock);
		WSACleanup();
		return 1;
	}

	if (listen(sock, SOMAXCONN) == SOCKET_ERROR)
	{
		fputs("listen failed.\n", stderr);
		closesocket(sock);
		WSACleanup();
		return 1;
	}

	CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)reciever, (LPVOID)sock, 0, nullptr);

	for (int i = 0; i < 64; ++i)
		CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)checkIfUserIsOnline, (LPVOID)i, 0, nullptr);

	while (true)
	{
		fputs("\n> ", stderr);
		char command[1024], s1[1024], s2[1024], s3[1024], s4[1024];
		fgets(command, 1024, stdin);
		int len = strlen(command);
		for (int i = 0; i < len; ++i)
			if ('A' <= command[i] && command[i] <= 'Z')
				command[i] += 32;
		sscanf(command, "%s%s%s%s", s1, s2, s3, s4);
		if (strcmp(s1, "exit") == 0)
			break;
		else if (strcmp(s1, "help") == 0)
		{
			fputs("Available commands:\n", stderr);
			fputs("\"exit\" - exit the program\n", stderr);
			fputs("\"help\" - display this message\n", stderr);
			fputs("\"send <filename> to <username>\" - send a file to a user\n", stderr);
			fputs("\"send cp to <username>\" - send your clipboard to a user\n", stderr);
			fputs("\"send <filename> to <groupname>\" - send a file to all users in a group\n", stderr);
			fputs("\"send cp to <groupname>\" - send your clipboard to all users in a group\n", stderr);
			fputs("\"newuser <username> <ip>\" - create a new user\n", stderr);
			fputs("\"deleteuser <username>\" - delete a user\n", stderr);
			fputs("\"newgroup <groupname>\" - create a new group\n", stderr);
			fputs("\"adduser <username> to <groupname>\" - add a user to a group\n", stderr);
			fputs("\"removeuser <username> from <groupname>\" - remove a user from a group\n", stderr);
			fputs("\"listusers in <groupname>\" - list all users in a group\n", stderr);
			fputs("\"deletegroup <groupname>\" - delete a group\n", stderr);
			fputs("\"listgroups\" - list all groups\n", stderr);
		}
		else if (strcmp(s1, "send") == 0 && strcmp(s2, "cp") != 0 && strcmp(s3, "to") == 0)
		{
			unsigned long long receivers = 0;
			for (int i = 0; i < 64; ++i)
				if (strcmp(s4, username[i]) == 0)
					receivers |= 1ULL << i;
			for (int i = 0; i < 64; ++i)
				if (strcmp(s4, groupname[i]) == 0)
					receivers |= usersInGroup[i];
			for (int i = 0; i < 64; ++i)
				if (receivers & (1ULL << i))
				{
					fprintf(stderr, "Sending file to %s: ", username[i]);
					SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
					if (clientSocket == INVALID_SOCKET)
					{
						fputs("socket failed.\n", stderr);
						continue;
					}
					SOCKADDR_IN clientAddr;
					clientAddr.sin_family = AF_INET;
					clientAddr.sin_port = htons(11451);
					clientAddr.sin_addr.s_addr = ip[i];
					if (connect(clientSocket, (SOCKADDR*)&clientAddr, sizeof(clientAddr)) == SOCKET_ERROR)
					{
						fputs("connect failed.\n", stderr);
						closesocket(clientSocket);
						continue;
					}
					FILE *file = fopen(s2, "rb");
					if (file == nullptr)
					{
						fputs("fopen failed.\n", stderr);
						closesocket(clientSocket);
						continue;
					}
					int s2Len = strlen(s2), sufnameLen = 0;
					for (int i = s2Len - 1; true; --i)
						if (i == -1)
							break;
						else if (s2[i] == '.')
						{
							sufnameLen = s2Len - i - 1;
							break;
						}
						else if (s2[i] == '\\' || s2[i] == '/')
							break;
					send(clientSocket, (const char*)(&sufnameLen), 4, 0);
					send(clientSocket, s2 + s2Len - sufnameLen, sufnameLen, 0);
					fseek(file, 0, SEEK_END);
					unsigned long long fileLen = ftell(file);
					fseek(file, 0, SEEK_SET);
					send(clientSocket, (const char*)(&fileLen), 8, 0);
					char buffer[1024];
					while (fileLen > 0)
					{
						int len = fread(buffer, 1, fileLen > 1024 ? 1024 : fileLen, file);
						send(clientSocket, buffer, len, 0);
						fileLen -= len;
					}
					fclose(file);
					closesocket(clientSocket);
					fputs("done.\n", stderr);
				}
		}
		else if (strcmp(s1, "send") == 0 && strcmp(s2, "cp") == 0 && strcmp(s3, "to") == 0)
		{
			unsigned long long receivers = 0;
			for (int i = 0; i < 64; ++i)
				if (strcmp(s4, username[i]) == 0)
					receivers |= 1ULL << i;
			for (int i = 0; i < 64; ++i)
				if (strcmp(s4, groupname[i]) == 0)
					receivers |= usersInGroup[i];
			for (int i = 0; i < 64; ++i)
				if (receivers & (1ULL << i))
				{
					fprintf(stderr, "Sending clipboard to %s: ", username[i]);
					SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
					if (clientSocket == INVALID_SOCKET)
					{
						fputs("socket failed.\n", stderr);
						continue;
					}
					SOCKADDR_IN clientAddr;
					clientAddr.sin_family = AF_INET;
					clientAddr.sin_port = htons(11451);
					clientAddr.sin_addr.s_addr = ip[i];
					if (connect(clientSocket, (SOCKADDR*)&clientAddr, sizeof(clientAddr)) == SOCKET_ERROR)
					{
						fputs("connect failed.\n", stderr);
						closesocket(clientSocket);
						continue;
					}
					if (OpenClipboard(nullptr))
					{
						HANDLE hData = GetClipboardData(CF_UNICODETEXT);
						if (hData != nullptr)
						{
							const wchar_t *data = (const wchar_t*)GlobalLock(hData);
							if (data != nullptr)
							{
								unsigned long long dataLen = 0;
								while (data[dataLen] != L'\0')
									++dataLen;
								dataLen <<= 1;
								const unsigned int value = 0xFFFFFFFF;
								send(clientSocket, (const char*)(&value), 4, 0);
								send(clientSocket, (const char*)(&dataLen), 8, 0);
								send(clientSocket, (const char*)(data), dataLen, 0);
							}
							GlobalUnlock(hData);
						}
						CloseClipboard();
					}
					closesocket(clientSocket);
					fputs("done.\n", stderr);
				}
		}
		else if (strcmp(s1, "newuser") == 0)
		{
			bool flag = false;
			for (int i = 0; i < 64; ++i)
				if (strcmp(s2, username[i]) == 0)
				{
					fputs("Username already exists.\n", stderr);
					flag = true;
					break;
				}
			if (flag)
				continue;
			for (int i = 0; i < 64; ++i)
				if (strcmp(s2, groupname[i]) == 0)
				{
					fputs("Username already exists as a group name.\n", stderr);
					flag = true;
					break;
				}
			if (flag)
				continue;

			int emptySlot = -1;
			for (int i = 0; i < 64; ++i)
				if (username[i][0] == '\0')
				{
					emptySlot = i;
					break;
				}
			if (emptySlot == -1)
			{
				fputs("No empty slot available. The maximum number of users is 64.\n", stderr);
				continue;
			}
			strcpy(username[emptySlot], s2);
			ip[emptySlot] = inet_addr(s3);
			usersInGroup[0] |= 1ULL << emptySlot;
			fputs("User created.\n", stderr);
		}
		else if (strcmp(s1, "deleteuser") == 0)
		{
			int slot = -1;
			for (int i = 0; i < 64; ++i)
				if (strcmp(s2, username[i]) == 0)
				{
					slot = i;
					break;
				}
			if (slot == -1)
			{
				fputs("User not found.\n", stderr);
				continue;
			}
			for (int i = 0; i < 64; ++i)
				usersInGroup[i] &= ~(1ULL << slot);
			username[slot][0] = '\0';
			ip[slot] = 0;
			fputs("User deleted.\n", stderr);
		}
		else if (strcmp(s1, "newgroup") == 0)
		{
			bool flag = false;
			for (int i = 0; i < 64; ++i)
				if (strcmp(s2, groupname[i]) == 0)
				{
					fputs("Group already exists.\n", stderr);
					flag = true;
					break;
				}
			if (flag)
				continue;
			for (int i = 0; i < 64; ++i)
				if (strcmp(s2, username[i]) == 0)
				{
					fputs("Group name already exists as a username.\n", stderr);
					flag = true;
					break;
				}
			if (flag)
				continue;

			int emptySlot = -1;
			for (int i = 0; i < 64; ++i)
				if (groupname[i][0] == '\0')
				{
					emptySlot = i;
					break;
				}
			if (emptySlot == -1)
			{
				fputs("No empty slot available. The maximum number of groups is 64.\n", stderr);
				continue;
			}
			strcpy(groupname[emptySlot], s2);
			usersInGroup[emptySlot] = 0;
			fputs("Group created.\n", stderr);
		}
		else if (strcmp(s1, "adduser") == 0 && strcmp(s3, "to") == 0)
		{
			int userSlot = -1, groupSlot = -1;
			for (int i = 0; i < 64; ++i)
				if (strcmp(s2, username[i]) == 0)
					userSlot = i;
			for (int i = 0; i < 64; ++i)
				if (strcmp(s4, groupname[i]) == 0)
					groupSlot = i;
			if (userSlot == -1)
			{
				fputs("User not found.\n", stderr);
				continue;
			}
			if (groupSlot == -1)
			{
				fputs("Group not found.\n", stderr);
				continue;
			}
			if (groupSlot == 0)
			{
				fputs("Group \"everyone\" cannot be modified.\n", stderr);
				continue;
			}
			usersInGroup[groupSlot] |= 1ULL << userSlot;
			fputs("User added to group.\n", stderr);
		}
		else if (strcmp(s1, "removeuser") == 0 && strcmp(s3, "from") == 0)
		{
			int userSlot = -1, groupSlot = -1;
			for (int i = 0; i < 64; ++i)
				if (strcmp(s2, username[i]) == 0)
					userSlot = i;
			for (int i = 0; i < 64; ++i)
				if (strcmp(s4, groupname[i]) == 0)
					groupSlot = i;
			if (userSlot == -1)
			{
				fputs("User not found.\n", stderr);
				continue;
			}
			if (groupSlot == -1)
			{
				fputs("Group not found.\n", stderr);
				continue;
			}
			if (groupSlot == 0)
			{
				fputs("Group \"everyone\" cannot be modified.\n", stderr);
				continue;
			}
			usersInGroup[groupSlot] &= ~(1ULL << userSlot);
			fputs("User removed from group.\n", stderr);
		}
		else if (strcmp(s1, "listusers") == 0 && strcmp(s2, "in") == 0)
		{
			int slot = -1;
			for (int i = 0; i < 64; ++i)
				if (strcmp(s3, groupname[i]) == 0)
				{
					slot = i;
					break;
				}
			if (slot == -1)
			{
				fputs("Group not found.\n", stderr);
				continue;
			}
			for (int i = 0; i < 64; ++i)
				if (usersInGroup[slot] & (1ULL << i))
				{
					fprintf(stderr, "%s: %s\n", username[i], userStatu[i] == 1 ? "online" : userStatu[i] == 0 ? "offline" : "different version");
				}
			fputs("done.\n", stderr);
		}
		else if (strcmp(s1, "deletegroup") == 0)
		{
			int slot = -1;
			for (int i = 0; i < 64; ++i)
				if (strcmp(s2, groupname[i]) == 0)
				{
					slot = i;
					break;
				}
			if (slot == -1)
			{
				fputs("Group not found.\n", stderr);
				continue;
			}
			if (slot == 0)
			{
				fputs("Group \"everyone\" cannot be deleted.\n", stderr);
				continue;
			}
			groupname[slot][0] = '\0';
			usersInGroup[slot] = 0;
			fputs("Group deleted.\n", stderr);
		}
		else if (strcmp(s1, "listgroups") == 0)
		{
			for (int i = 0; i < 64; ++i)
				if (groupname[i][0] != '\0')
				{
					fprintf(stderr, "group %s: \n", groupname[i]);
					for (int j = 0; j < 64; ++j)
						if (usersInGroup[i] & (1ULL << j))
						{
							fprintf(stderr, "\t%s: %s\n", username[j], userStatu[j] == 1 ? "online" : userStatu[j] == 0 ? "offline" : "different version");
						}
				}
			fputs("done.\n", stderr);
		}
		else
		{
			fputs("Invalid command. Please type \"help\" for a list of available commands.\n", stderr);
		}
	}

	return 0;
}

void reciever(SOCKET socket)
{
	while (true)
	{
		SOCKET clientSocket = accept(socket, nullptr, nullptr);
		if (clientSocket == INVALID_SOCKET)
			break;
		CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)handleClient, (LPVOID)clientSocket, 0, nullptr);
	}
	closesocket(socket);
	WSACleanup();
}

void handleClient(SOCKET socket)
{
	SOCKADDR_STORAGE address;
	int addressSize = sizeof(address);
	getpeername(socket, (SOCKADDR *)&address, &addressSize);

	unsigned int sufnameLen = 0xFFFFFFFEU;
	unsigned long long fileLen = 0xFFFFFFFFFFFFFFFFULL;
	recv(socket, (char*)(&sufnameLen), 4, 0);
	bool saveToClipboard = false, ifSave = true;
	char* sufname = nullptr;
	if (sufnameLen == 0xFFFFFFFFU)
	{
		recv(socket, (char*)(&fileLen), 8, 0);
		char *str = new char[1024];
		sprintf(str, "Do you want to save it to the clipboard?\n"
			"Choose \"Yes\" to save it to the clipboard, \"No\" to save it to a file, or \"Cancel\" to discard it.\n"
			"From %s (%s), %llu bytes.", username[0], inet_ntoa(((SOCKADDR_IN *)(&address))->sin_addr), fileLen);
		int result = MessageBox(nullptr, str, "File Transfer", MB_YESNOCANCEL | MB_ICONQUESTION);
		if (result == IDYES)
			saveToClipboard = true;
		else if (result == IDCANCEL)
			ifSave = false;
	}
	else if (sufnameLen == 0xFFFFFFFEU)
	{
		// check if this user is online
		send(socket, (const char*)(&versionId), 4, 0);
		closesocket(socket);
		return;
	}
	else
	{
		sufname = new char[sufnameLen + 1];
		recv(socket, sufname, sufnameLen, 0);
		sufname[sufnameLen] = '\0';
		int slot = -1;
		for (int i = 0; i < 64; ++i)
			if (ip[i] == ((SOCKADDR_IN *)(&address))->sin_addr.s_addr && username[i][0] != '\0')
			{
				slot = i;
				break;
			}
		recv(socket, (char*)(&fileLen), 8, 0);
		char *str = new char[1536 + sufnameLen];
		sprintf(str, "Do you want to save it?\n"
			"Choose \"Yes\" to save it, or \"No\" to discard it.\n"
			"From %s (%s), %llu bytes.", slot == -1 ? "unknown" : username[slot], inet_ntoa(((SOCKADDR_IN *)(&address))->sin_addr), fileLen);
		int result = MessageBox(nullptr, str, "File Transfer", MB_YESNO | MB_ICONQUESTION);
		if (result == IDNO)
			ifSave = false;
		delete []str;
	}
	if (ifSave && fileLen != 0xFFFFFFFFFFFFFFFFULL)
		if (saveToClipboard)
		{
			if (OpenClipboard(nullptr))
			{
				EmptyClipboard();
				HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, fileLen + 2);
				if (hMem != nullptr)
				{
					char* buffer = (char*)GlobalLock(hMem);
					recv(socket, buffer, fileLen, 0);
					buffer[fileLen] = buffer[fileLen + 1] = '\0';
					GlobalUnlock(hMem);
					SetClipboardData(CF_UNICODETEXT, hMem);
				}
				CloseClipboard();
			}
		}
		else
		{
			OPENFILENAME ofn;
			char filename[1024], *lpstrFilter = nullptr;

			ZeroMemory(&ofn, sizeof(ofn));
			ZeroMemory(filename, sizeof(filename));

			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = nullptr;
			ofn.lpstrFile = filename;
			ofn.nMaxFile = sizeof(filename);
			if (sufname == nullptr)
			{
				lpstrFilter = new char[21];
				sprintf(lpstrFilter, "All Files (*.*)\n*.*\n");
				for (int i = 20; i >= 0; --i)
					if (lpstrFilter[i] == '\n')
						lpstrFilter[i] = '\0';
			}
			else
			{
				lpstrFilter = new char[42 + (sufnameLen << 1)];
				sprintf(lpstrFilter, "Recieve Kind (*.%s)\n*.%s\nAll Files (*.*)\n*.*\n", sufname, sufname);
				for (int i = 41 + (sufnameLen << 1); i >= 0; --i)
					if (lpstrFilter[i] == '\n')
						lpstrFilter[i] = '\0';
			}
			ofn.lpstrFilter = lpstrFilter;
			ofn.nFilterIndex = 1;
			ofn.lpstrTitle = "Recieve File";
			ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
			if (GetSaveFileName(&ofn) == TRUE)
			{
				FILE* file = fopen(filename, "wb");
				if (file != nullptr)
				{
					char buffer[1024];
					while (fileLen > 0)
					{
						int len = recv(socket, buffer, fileLen > 1024 ? 1024 : fileLen, 0);
						if (len == 0)
							break;
						fwrite(buffer, 1, len, file);
						fileLen -= len;
					}
					fclose(file);
				}
			}
			delete[] lpstrFilter;
		}
	if (sufname != nullptr)
		delete[] sufname;
	send(socket, "done", 4, 0);
	closesocket(socket);
}

void checkIfUserIsOnline(int slot)
{
	while (true)
	{
		Sleep(1000);
		if (ip[slot] != 0)
		{
			SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (clientSocket == INVALID_SOCKET)
			{
				userStatu[slot] = 0;
				continue;
			}
			SOCKADDR_IN clientAddr;
			clientAddr.sin_family = AF_INET;
			clientAddr.sin_port = htons(11451);
			clientAddr.sin_addr.s_addr = ip[slot];
			if (connect(clientSocket, (SOCKADDR*)&clientAddr, sizeof(clientAddr)) == SOCKET_ERROR)
			{
				userStatu[slot] = 0;
				closesocket(clientSocket);
				continue;
			}
			unsigned int value = 0xFFFFFFFEU;
			send(clientSocket, (const char*)(&value), 4, 0);
			recv(clientSocket, (char*)(&value), 4, 0);
			if (value != versionId)
			{
				userStatu[slot] = -1;
				closesocket(clientSocket);
				continue;
			}
			userStatu[slot] = 1;
			closesocket(clientSocket);
		}
		else
			userStatu[slot] = 0;
	}
}
