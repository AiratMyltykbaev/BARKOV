#include <Windows.h>
#include <iostream>
#include <fstream>
#include <WtsApi32.h>
#include <time.h>
#include <stdlib.h>

#define SERVICE_NAME TEXT("Service")

std::fstream Log;
SERVICE_STATUS ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE hServiceStausHandler = NULL;
HANDLE hServiceEvent = NULL;
void WINAPI ServiceMain(DWORD dwArgc, LPSTR* lpArgv);
DWORD WINAPI ServiceControlHandlerEx(DWORD dwControl, DWORD dwEventType, LPVOID dwEventData, LPVOID dwContent);
void ServiceReportStatus(
	DWORD dwCurrentState,
	DWORD dwWin32ExitCode,
	DWORD dwWaitHint
);
void ServiceInit(DWORD dwArgc, LPSTR* lpArgv);
void ServiceInstal();
void ServiceDelete();
BOOL CustomCreateProcess(DWORD wtsSession, DWORD& dwBytes);
char* GetUsernameFromSId(DWORD sId, DWORD& dwBytes);

void WINAPI ServiceMain(DWORD dwArgc, LPSTR* lpArgv) {
	if (Log.is_open())
		Log.sync();
	Log.close();
	Log.open("C:\\Users\\airat\\source\\repos\\BARKOV\\log.txt", std::ios_base::in | std::ios_base::app);
	Log << "Service Starting at " << std::endl;
	hServiceStausHandler = RegisterServiceCtrlHandlerEx(SERVICE_NAME, (LPHANDLER_FUNCTION_EX)ServiceControlHandlerEx, NULL);
	ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SESSIONCHANGE;
	ServiceStatus.dwServiceSpecificExitCode = 0;
	ServiceReportStatus(SERVICE_START_PENDING, NO_ERROR, 3000);
	PWTS_SESSION_INFO wtsSessions;
	PROCESS_INFORMATION processInfo;
	DWORD sessionCount, dwBytes = NULL;
	for (auto i = 1u; i < dwArgc; ++i)
		Log << lpArgv[i] << std::endl;
	Log << "Service have been Started at " << std::endl;
	if (!WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &wtsSessions, &sessionCount)) {
		Log << GetLastError() << std::endl;
		Log.close();
		return;
	}
	for (auto i = 1; i < sessionCount; ++i)
		CustomCreateProcess(wtsSessions[i].SessionId, dwBytes);
	ServiceInit(dwArgc, lpArgv);
	if (!Log.is_open())
		Log.open("C:\\Users\\airat\\source\\repos\\BARKOV\\log.txt", std::ios_base::in | std::ios_base::app);
	Log << "Service Initialization complete at " << std::endl
		<< "Service begins to performing" << std::endl;
	while (ServiceStatus.dwCurrentState != SERVICE_STOPPED) {
		Log << "Service is running. There are no problems on your PC. " << std::endl;
		Log.sync();
		if (WaitForSingleObject(hServiceEvent, 60000) != WAIT_TIMEOUT)
			ServiceReportStatus(SERVICE_STOPPED, NO_ERROR, 0);
	}
	Log.close();
}

DWORD WINAPI ServiceControlHandlerEx(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContent) {
	DWORD errorCode = NO_ERROR, dwBytes = NULL;
	WTSSESSION_NOTIFICATION* sessionNotification = NULL;
	char* pcUserName = nullptr;
	if (Log.is_open())
		Log.sync();
	Log.close();
	Log.open("C:\\Users\\airat\\source\\repos\\BARKOV\\log.txt", std::ios_base::in | std::ios_base::app);
	switch (dwControl)
	{
	case SERVICE_CONTROL_INTERROGATE:
		break;
	case SERVICE_CONTROL_SESSIONCHANGE: 
		sessionNotification = static_cast<WTSSESSION_NOTIFICATION*>(lpEventData);
		pcUserName = GetUsernameFromSId(sessionNotification->dwSessionId, dwBytes);
		if (dwEventType == WTS_SESSION_LOGOFF) {
			Log << pcUserName << " is logging off" << std::endl;
			break;
		}
		else if (dwEventType == WTS_SESSION_LOGON) {
			Log << pcUserName << " is logging in" << std::endl;
			CustomCreateProcess(sessionNotification->dwSessionId, dwBytes);
		}
		delete[] pcUserName;
		break;

	case SERVICE_CONTROL_STOP:
		Log << "Service have been Stopped at " << std::endl;
		ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		break;
	case SERVICE_CONTROL_SHUTDOWN:
		Log << "PC is going to SHUTDOWN stopping the Service" << std::endl;
		ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		break;
	default:
		errorCode = ERROR_CALL_NOT_IMPLEMENTED;
		break;
	}

	Log.close();
	ServiceReportStatus(ServiceStatus.dwCurrentState, errorCode, 0);
	return errorCode;
}

void ServiceReportStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint) {
	static DWORD dwCheckPoint = 1;
	ServiceStatus.dwCurrentState = dwCurrentState;
	ServiceStatus.dwWin32ExitCode = dwWin32ExitCode;
	ServiceStatus.dwWaitHint = dwWaitHint;
	if (dwCurrentState == SERVICE_RUNNING || dwCurrentState == SERVICE_STOPPED)
		ServiceStatus.dwCheckPoint = 0;
	else
		ServiceStatus.dwCheckPoint = dwCheckPoint++;
	SetServiceStatus(hServiceStausHandler, &ServiceStatus);
}

void ServiceInit(DWORD dwArgc, LPSTR* lpArgv) {
	if (!Log.is_open())
		Log.open("C:\\Users\\airat\\source\\repos\\BARKOV\\log.txt", std::ios_base::in | std::ios_base::app);
	hServiceEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (hServiceEvent == NULL) {
		ServiceReportStatus(SERVICE_STOPPED, NO_ERROR, 0);
		return;
	}
	else
		ServiceReportStatus(SERVICE_RUNNING, NO_ERROR, 0);
}

void ServiceInstal() {
	Log.open("C:\\Users\\airat\\source\\repos\\BARKOV\\log.txt", std::ios_base::in | std::ios_base::app);
	SC_HANDLE hSCManager = NULL;
	SC_HANDLE hService = NULL;
	DWORD dwModuleFileName = 0;
	TCHAR szPath[MAX_PATH];
	dwModuleFileName = GetModuleFileName(NULL, szPath, MAX_PATH);
	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	hService = CreateService(
		hSCManager,
		SERVICE_NAME,
		SERVICE_NAME,
		SERVICE_ALL_ACCESS,
		SERVICE_WIN32_OWN_PROCESS,
		SERVICE_DEMAND_START,
		SERVICE_ERROR_NORMAL,
		szPath,
		NULL, NULL, NULL, NULL, NULL
	);

	Log << "Service successfully installed at " << std::endl;
	Log.close();
	CloseServiceHandle(hService);
	CloseServiceHandle(hSCManager);
}

void ServiceDelete() {
	Log.open("C:\\Users\\airat\\source\\repos\\BARKOV\\log.txt", std::ios_base::in | std::ios_base::app);
	SC_HANDLE hSCManager = NULL;
	SC_HANDLE hService = NULL;
	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	hService = OpenService(hSCManager, SERVICE_NAME, SERVICE_ALL_ACCESS);
	if (!DeleteService(hService))
		Log << GetLastError() << std::endl;
	Log << "Service have been successfully deleted at " << std::endl;
	CloseServiceHandle(hService);
	CloseServiceHandle(hSCManager);
}

int main(int argc, CHAR* argv[]) {
	Log.open("C:\\Users\\airat\\source\\repos\\BARKOV\\log.txt", std::ios_base::in | std::ios_base::app);
	if (!Log)
		Log.open("C:\\Users\\airat\\source\\repos\\BARKOV\\log.txt", std::ios_base::in);
	Log.close();
	if (lstrcmpiA(argv[1], "install") == 0)
		ServiceInstal();
	else if (lstrcmpiA(argv[1], "delete") == 0)
		ServiceDelete();
	SERVICE_TABLE_ENTRY DispatchTable[] = {
		{(LPWSTR)SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
		{NULL, NULL}
	};
	StartServiceCtrlDispatcher(DispatchTable);
}

BOOL CustomCreateProcess(DWORD wtsSession, DWORD& dwBytes) {
	if (!Log.is_open())
		Log.open("C:\\Users\\airat\\source\\repos\\BARKOV\\log.txt", std::ios_base::in | std::ios_base::app);
	HANDLE userToken;
	PROCESS_INFORMATION pi{};
	STARTUPINFO si{};
	WCHAR path[] = L"sgvuoi.exe";
	WTSQueryUserToken(wtsSession, &userToken);
	CreateProcessAsUser(userToken, NULL, path, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
	char* pcUserName = GetUsernameFromSId(wtsSession, dwBytes);
	Log << "Application pId " << pi.dwProcessId << " have been started by user " << pcUserName
		<< " in session " << wtsSession << " at " << std::endl;
	delete[] pcUserName;
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return true;
}



char* GetUsernameFromSId(DWORD sId, DWORD& dwBytes) {
	char* pcUserName = new char[105];
	LPWSTR buff;
	WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, sId, WTSUserName, &buff, &dwBytes);
	WideCharToMultiByte(CP_UTF8, 0, buff, -1, pcUserName, 105, 0, 0);
	WTSFreeMemory(buff);
	return pcUserName;
}