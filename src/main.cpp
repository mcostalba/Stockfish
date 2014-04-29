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
#include "book.h"
#include "evaluate.h"
#include "position.h"
#include "search.h"
#include "thread.h"
#include "tt.h"
#include "ucioption.h"

extern "C" void init() {

  UCI::init(Options);
  Bitboards::init();
  Position::init();
  Bitbases::init_kpk();
  Search::init();
  Pawns::init();
  Eval::init();
  Threads.init();
  TT.resize(Options["Hash"]);

  UCI::commandInit();

}

int main(int argc, char* argv[]) {
  init();

  std::string args;

  for (int i = 1; i < argc; ++i)
      args += std::string(argv[i]) + " ";

  if(!args.empty())
	  UCI::command(args);

#ifndef EMSCRIPTEN
  std::string cmd;
  while(std::getline(std::cin, cmd))
	  UCI::command(cmd);
#endif
}

extern "C" void uci_command(const char* cmd) {
	UCI::command(cmd);
}

extern "C" void set_book(unsigned char* pBookData, unsigned int size) {
	PolyglotBook::setBookData(pBookData, size);
	UCI::command("setoption name OwnBook value true");
}