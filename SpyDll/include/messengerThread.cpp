#include "pch.h"
#include "messengerThread.h"
#include "Flags.h"
#include "messageQueue.h"

DWORD WINAPI MessageThread(LPVOID) {
	std::string buff = "";
	DWORD bytesWritten;
	HANDLE hPipe;
	int failureIteration = 1;
	do {
		hPipe = CreateFile(
			_TEXT("\\\\.\\pipe\\messagePipeline"),
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			0,
			NULL
		);
		Sleep(std::clamp(50 * failureIteration, 50, 5000));
		failureIteration++;
	} while (hPipe == INVALID_HANDLE_VALUE && !ThreadExpectedToStop);

	while (!ThreadExpectedToStop) {
		while (messenger::readOffSet <= messenger::writeOffSet) {
			buff = messenger::ReadMessage();
			messenger::readOffSet++;
			if (buff.empty()) continue;
			WriteFile(
				hPipe,
				buff.c_str(),
				static_cast<DWORD>(buff.size()),
				&bytesWritten,
				NULL
			);
		}
		Sleep(500);
	}

	CloseHandle(hPipe);
	return 0;
}
