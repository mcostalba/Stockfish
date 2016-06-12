#ifndef TBPROBE_H
#define TBPROBE_H

#include "../search.h"

namespace Tablebases {

extern int MaxCardinality;

void init(const std::string& path);
void load_dtz_table(char *str, uint64_t key1, uint64_t key2);
short ReadUshort(unsigned char* d);
int ReadUint32(unsigned char* d);
int probe_wdl(Position& pos, int *success);
int probe_dtz(Position& pos, int *success);
bool root_probe(Position& pos, Search::RootMoves& rootMoves, Value& score);
bool root_probe_wdl(Position& pos, Search::RootMoves& rootMoves, Value& score);
void filter_root_moves(Position& pos, Search::RootMoves& rootMoves);

}

#endif
