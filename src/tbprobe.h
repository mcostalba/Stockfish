#ifndef TBPROBE_H
#define TBPROBE_H

namespace Tablebases {

extern int TBLargest;

void init(const std::string& path);
int probe_wdl(Position& pos, int *success);
int probe_dtz(Position& pos, int *success);
bool root_probe(Position& pos);
bool root_probe_wdl(Position& pos);

}

#endif
