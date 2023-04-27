#include "Game.h"

#ifdef TH_RELEASE
#include <iostream>

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

//void* operator new(size_t size) {
//	printf("alloc ");
//	return malloc(size);
//}

extern "C" int main(int argc, char* argv[]) {
#ifdef TH_RELEASE
	if (argc == 2) {
		if (strcmp(argv[1], "--console") == 0) {
			SpawnConsole();
		}
	}
#endif

	int result = 0;

	for (;;) {
		th::Game game;

		if (game.Init()) {
			if (game.Run()) {
				result = 0;
			} else {
				result = 1;
			}
		} else {
			result = 1;
		}

		game.Shutdown();

		if (!game.restart) {
			break;
		}
	}

	return result;
}
