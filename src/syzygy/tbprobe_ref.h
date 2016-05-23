#ifndef TBPROBE_REF_H
#define TBPROBE_REF_H

#include "../search.h"

namespace TablebasesRef {

extern int MaxCardinality;

void init(const std::string& path);
int probe_wdl(Position& pos, int *success);
int probe_dtz(Position& pos, int *success);
bool root_probe(Position& pos, Search::RootMoves& rootMoves, Value& score);
bool root_probe_wdl(Position& pos, Search::RootMoves& rootMoves, Value& score);

}

#endif
