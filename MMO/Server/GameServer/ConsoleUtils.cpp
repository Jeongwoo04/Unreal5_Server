#include "pch.h"
#include "ConsoleUtils.h"
#include <Windows.h>
#include <iostream>
#include <sstream>
#include <string>
#include "BenchMarkManager.h"

// ------------------------------------------------------------
// Console Utility
// ------------------------------------------------------------

HANDLE CreateRenderBuffer()
{
	HANDLE hBuffer = CreateConsoleScreenBuffer(
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr,
		CONSOLE_TEXTMODE_BUFFER,
		nullptr
	);

	return hBuffer;
}

void ClearConsoleBuffer(HANDLE hBuffer)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi{};

	if (!GetConsoleScreenBufferInfo(hBuffer, &csbi))
		return;

	const DWORD cellCount =
		static_cast<DWORD>(csbi.dwSize.X) *
		static_cast<DWORD>(csbi.dwSize.Y);

	COORD home = { 0, 0 };

	DWORD written = 0;

	// 기존 문자 제거
	FillConsoleOutputCharacterA(hBuffer, ' ', cellCount, home, &written);

	// 기존 속성 유지
	FillConsoleOutputAttribute(hBuffer, csbi.wAttributes, cellCount, home, &written);

	SetConsoleCursorPosition(hBuffer, home);
}

void WriteConsoleText(HANDLE hBuffer, const std::string& text)
{
	DWORD written = 0;

	WriteConsoleA(hBuffer, text.data(), static_cast<DWORD>(text.size()), &written, nullptr);
}

// ------------------------------------------------------------
// Rendering Worker
// ------------------------------------------------------------

void DoRenderingWorker()
{
	// --------------------------------------------------------
	// 1. Rendering 전용 Console Buffer 2개 생성
	// --------------------------------------------------------

	HANDLE hScreens[2];

	hScreens[0] = CreateRenderBuffer();
	hScreens[1] = CreateRenderBuffer();

	if (hScreens[0] == INVALID_HANDLE_VALUE ||
		hScreens[1] == INVALID_HANDLE_VALUE)
	{
		std::cerr << "콘솔 버퍼 생성 실패!" << std::endl;

		if (hScreens[0] != INVALID_HANDLE_VALUE)
			CloseHandle(hScreens[0]);

		if (hScreens[1] != INVALID_HANDLE_VALUE)
			CloseHandle(hScreens[1]);

		return;
	}

	// --------------------------------------------------------
	// 2. 커서 숨기기
	// --------------------------------------------------------

	CONSOLE_CURSOR_INFO cursorInfo{};
	cursorInfo.dwSize = 1;
	cursorInfo.bVisible = FALSE;

	SetConsoleCursorInfo(hScreens[0], &cursorInfo);
	SetConsoleCursorInfo(hScreens[1], &cursorInfo);

	// --------------------------------------------------------
	// 3. 초기 Buffer 설정
	// --------------------------------------------------------

	ClearConsoleBuffer(hScreens[0]);
	ClearConsoleBuffer(hScreens[1]);

	int currentBufferIndex = 0;

	// Rendering Worker 시작 시
	// 기존 서버 stdout과 Rendering 화면을 분리한다.
	SetConsoleActiveScreenBuffer(hScreens[currentBufferIndex]);

	// --------------------------------------------------------
	// 4. Rendering Loop
	// --------------------------------------------------------

	int32 lastRenderingRound = 0;
	RoundLog renderRoundLog;

	while (true)
	{
		// ----------------------------------------------------
		// Snapshot
		// ----------------------------------------------------

		RoomLog roomLog = GRoomLog;
		
		if (GRoundLog.round != lastRenderingRound)
		{
			renderRoundLog = GRoundLog;
			lastRenderingRound = GRoundLog.round;
		}


		// ----------------------------------------------------
		// Back Buffer 선택
		// ----------------------------------------------------

		const int backBufferIndex = 1 - currentBufferIndex;
		HANDLE hTargetBuffer = hScreens[backBufferIndex];


		// ----------------------------------------------------
		// Back Buffer 초기화
		// ----------------------------------------------------

		ClearConsoleBuffer(hTargetBuffer);


		// ----------------------------------------------------
		// 화면 전체를 문자열로 먼저 구성
		// ----------------------------------------------------

		std::ostringstream oss;

		// ----------------------------------------------------
		// ROOM
		// ----------------------------------------------------

		oss << "/*******************************\n"
			<< "*                              *\n"
			<< "         ROOM BENCH : No." << roomLog.roomID << "        \n"
			<< "*                              *\n"
			<< "*******************************/\n\n";

		oss << "[ROOM]\n";

		oss << "Tick Cost: "
			<< roomLog.tickCost
			<< " ms\n";

		oss << "Tick Interval: "
			<< roomLog.tickInterval
			<< " ms\n\n";


		// ----------------------------------------------------
		// OBJECT
		// ----------------------------------------------------

		oss << "[OBJECT]\n";

		oss << "Player     ] Chunk Count : "
			<< roomLog.players.first
			<< ", Player Count : "
			<< roomLog.players.second
			<< "\n";

		oss << "Monster    ] Chunk Count : "
			<< roomLog.monsters.first
			<< ", Monster Count : "
			<< roomLog.monsters.second
			<< "\n";

		oss << "Projectile ] Chunk Count : "
			<< roomLog.projectiles.first
			<< ", Projectile Count : "
			<< roomLog.projectiles.second
			<< "\n";

		oss << "Field      ] Chunk Count : "
			<< roomLog.fields.first
			<< ", Field Count : "
			<< roomLog.fields.second
			<< "\n\n";


		// ----------------------------------------------------
		// NETWORK
		// ----------------------------------------------------

		oss << "[NETWORK]\n";

		oss << "ImmediateFlushPktBytes: "
			<< roomLog.ImmediatePktBiteSize
			<< "\n";

		oss << "DeferFlushPktBytes: "
			<< roomLog.DeferPktBiteSize
			<< "\n\n";


		// ----------------------------------------------------
		// ROUND BENCH
		// ----------------------------------------------------

		if (renderRoundLog.round > 0)
		{
			oss << "/********************************\n"
				<< "*                               *\n"
				<< "         ROUND BENCH : " << renderRoundLog.round << "        \n"
				<< "*                               *\n"
				<< "********************************/\n\n";


			// ------------------------------------------------
			// TOTAL ROOM
			// ------------------------------------------------

			oss << "[TOTAL ROOM]\n";

			oss << "Tick Cost: "
				<< renderRoundLog.totalRoom.at("Room")
				<< "\n";

			oss << "Tick Interval: "
				<< renderRoundLog.totalRoom.at("TickInterval")
				<< "\n";

			oss << "Update Cost: "
				<< renderRoundLog.totalRoom.at("Update")
				<< "\n\n";


			// ------------------------------------------------
			// ROOM SEND JOB
			// ------------------------------------------------

			oss << "[ROOM SEND JOB]\n";

			oss << "Total Delay: "
				<< renderRoundLog.totalRoom.at("totalDelay")
				<< "\n";

			oss << "Queueing Delay: "
				<< renderRoundLog.totalRoom.at("QueueingDelay")
				<< "\n";

			oss << "Kernel Delivery: "
				<< renderRoundLog.totalRoom.at("kernelDelivery")
				<< "\n\n";


			// ------------------------------------------------
			// GLOBAL IO PENDING
			// ------------------------------------------------

			oss << "[GLOBAL IO PENDING]\n";

			oss << "Pending Counts : "
				<< renderRoundLog.IOPending
				<< "\n\n";


			// ------------------------------------------------
			// LOGIC WORKER
			// ------------------------------------------------

			oss << "[LOGIC WORKER]\n";

			int workerNum = 0;

			for (const auto& it : renderRoundLog.workers)
			{
				if (it.type != WorkerType::LOGIC)
					continue;

				oss << "Worker #"
					<< workerNum
					<< " | Job Counts "
					<< it.JobCounts
					<< " | ActiveTime "
					<< it.ActiveTimeMs
					<< " | Utilization "
					<< it.ActiveRatio
					<< " %\n";

				workerNum++;
			}

			oss << "\n";


			// ------------------------------------------------
			// SEND WORKER
			// ------------------------------------------------

			oss << "[SEND WORKER]\n";

			workerNum = 0;

			for (const auto& it : renderRoundLog.workers)
			{
				if (it.type != WorkerType::SEND)
					continue;

				oss << "Worker #"
					<< workerNum
					<< " | Job Counts "
					<< it.JobCounts
					<< " | ActiveTime "
					<< it.ActiveTimeMs
					<< " | Utilization "
					<< it.ActiveRatio
					<< " %\n";

				workerNum++;
			}
		}


		// ----------------------------------------------------
		// 완성된 문자열을 Back Buffer에 직접 출력
		// ----------------------------------------------------

		const std::string output = oss.str();

		WriteConsoleText(
			hTargetBuffer,
			output
		);


		// ----------------------------------------------------
		// 완성된 Back Buffer를 화면으로 교체
		// ----------------------------------------------------

		SetConsoleActiveScreenBuffer(hTargetBuffer);

		currentBufferIndex = backBufferIndex;


		// ----------------------------------------------------
		// Rendering 주기
		// ----------------------------------------------------

		Sleep(50);
	}


	// --------------------------------------------------------
	// 종료 처리
	// --------------------------------------------------------

	CloseHandle(hScreens[0]);
	CloseHandle(hScreens[1]);
}