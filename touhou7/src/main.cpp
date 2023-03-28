#include "Game.h"

//#include <Windows.h>

#include <iostream>

#ifdef TH_RELEASE
extern "C" int __stdcall AllocConsole(void);

static void SpawnConsole() {
	AllocConsole();
	FILE* fDummy;
	freopen_s(&fDummy, "CONOUT$", "w", stdout);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);
	freopen_s(&fDummy, "CONIN$", "r", stdin);
	std::cout.clear();
	std::clog.clear();
	std::cerr.clear();
	std::cin.clear();
}
#endif

extern "C" int main(int argc, char* argv[]) {
#ifdef TH_RELEASE
	if (argc > 1) {
		if (strcmp(argv[1], "--console") == 0) {
			SpawnConsole();
		}
	}
#endif

	for (;;) {
		th::Game game;

		game.Init();

		game.Run();

		game.Shutdown();

		if (!game.restart) {
			break;
		}
	}

	return 0;
}