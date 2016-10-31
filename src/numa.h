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

#ifndef NUMA_H
#define NUMA_H

#include <vector>


/// Bind a threads to the best suitable NUMA node
class Numa {
public:
    static Numa& instance() {
      static Numa numa;
      return numa;
    }

    /// Disable NUMA awareness. Useful when running several single-threaded
    /// test games simultaneously on NUMA hardware.
    void disable() { threadToNode.clear(); }

    /// Bind current thread to NUMA node determined by threadToNode
    void bindThisThread(size_t idx) const;

private:
    Numa();

    std::vector<int> threadToNode;
};

#endif
