/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2014 Marco Costalba, Joona Kiiski, Tord Romstad

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>

#include "bitboard.h"
#include "evaluate.h"
#include "position.h"
#include "search.h"
#include "thread.h"
#include "tt.h"
#include "ucioption.h"
#include <thread>

using namespace std;

void cpuID(unsigned i, unsigned regs[4]) {
#ifdef _WIN32
	__cpuid((int *)regs, (int)i);

#else
	asm volatile
		("cpuid" : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3])
		: "a" (i), "c" (0));
#endif
}

//set affinity
void setaff()
{
	unsigned regs[4];
	// Get CPU features
	cpuID(1, regs);
	unsigned cpuFeatures = regs[3]; // EDX

	// Detect hyperthreads
	bool hyperThreads = cpuFeatures & (1 << 28);

	//may return 0 when not able to detect
	int cores = std::thread::hardware_concurrency();

	if (hyperThreads && cores > 0)
	{
		HANDLE process = GetCurrentProcess();
		DWORD_PTR processAffinityMask = 0;
		for (int i = 0; i < cores; i += 2)
			processAffinityMask = processAffinityMask | 1 << i;
		BOOL success = SetProcessAffinityMask(process, processAffinityMask);
	}
}

int main(int argc, char* argv[]) {
  setaff();
  std::cout << engine_info() << std::endl;

  UCI::init(Options);
  Bitboards::init();
  Position::init();
  Bitbases::init_kpk();
  Search::init();
  Pawns::init();
  Eval::init();
  Threads.init();
  TT.resize(Options["Hash"]);

  UCI::loop(argc, argv);

  Threads.exit();
}
