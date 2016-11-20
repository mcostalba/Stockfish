/*
    Texel - A UCI chess engine.
    Copyright (C) 2014-2015  Peter Österlund, peterosterlund2@gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _WIN32

WinProcGroup::WinProcGroup() {}
static void WinProcGroup::bindThisThread(size_t) {}

#else

#if _WIN32_WINNT < 0x0601
#undef  _WIN32_WINNT
#define _WIN32_WINNT 0x0601 // Force to include newest API (Win 7 or later)
#endif

#include <windows.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <set>

#include "bitboard.h"
#include "win_groups.h"

WinProcGroup::WinProcGroup() {

  int threads = 0;
  int nodes = 0;
  int cores = 0;
  DWORD returnLength = 0;
  DWORD byteOffset = 0;

  SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* buffer = nullptr;
  while (true)
  {
      if (GetLogicalProcessorInformationEx(RelationAll, buffer, &returnLength))
          break;

      else if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
      {
          free(buffer);
          buffer = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)malloc(returnLength);
          if (!buffer)
              return;
      }
      else
      {
          free(buffer);
          return;
      }
  }

  SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* ptr = buffer;
  while ((ptr->Size > 0) && (byteOffset + ptr->Size <= returnLength))
  {
      switch (ptr->Relationship)
      {
      case RelationNumaNode:
          nodes++;
          break;
      case RelationProcessorCore:
          cores++;
          threads += (ptr->Processor.Flags == LTP_PC_SMT) ? 2 : 1;
          break;
      default:
          break;
      }
      byteOffset += ptr->Size;
      ptr = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)(((char*)ptr) + ptr->Size);
  }

  for (int n = 0; n < nodes; n++)
      for (int i = 0; i < cores / nodes; i++)
          threadToGroup.push_back(n);

  for (int t = 0; t < threads - cores; t++)
      threadToGroup.push_back(t % nodes);
}

void WinProcGroup::bindThisThread(size_t idx) {

  if (idx >= instance().threadToGroup.size())
      return;

  int group = instance().threadToGroup[idx];
  GROUP_AFFINITY mask;

  if (!GetNumaNodeProcessorMaskEx(group, &mask))
      return;

  if (SetThreadGroupAffinity(GetCurrentThread(), &mask, nullptr))
      std::cout << "Bind thread ";
  else
      std::cout << "Failed to bind thread ";

  std::cout << idx << " to group " << group << std::endl;
}

#endif
