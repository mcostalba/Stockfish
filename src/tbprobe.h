#ifndef TBPROBE_H
#define TBPROBE_H

namespace Tablebases {

void init(void);
int probe_wdl(Position& pos, int *success);
int probe_dtz(Position& pos, int *success);
bool root_probe(Position& pos);

}

#endif

