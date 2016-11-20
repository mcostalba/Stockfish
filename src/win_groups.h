/*
    Texel - A UCI chess engine.
    Copyright (C) 2014  Peter Österlund, peterosterlund2@gmail.com

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

#ifndef WIN_GROUPS_H
#define WIN_GROUPS_H

#include <cstddef>  // For size_t
#include <vector>


/// Bind a thread to the best suitable processor group
class WinProcGroup {
public:
    /// Disable NUMA awareness. Useful when running several single-threaded
    /// test games simultaneously on NUMA hardware.
    static void disable() { instance().threadToGroup.clear(); }

    /// Bind current thread to the processor group determined by threadToGroup
    static void bindThisThread(size_t idx);

private:
    WinProcGroup();

    static WinProcGroup& instance() {
      static WinProcGroup obj;
      return obj;
    }

    std::vector<int> threadToGroup;
};

#endif
