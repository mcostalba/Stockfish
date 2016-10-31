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

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <set>

#include "bitboard.h"
#include "numa.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <numa.h>
#endif

Numa::Numa() {
#ifdef _WIN32
    int threads = 0;
    int nodes = 0;
    int cores = 0;
    DWORD returnLength = 0;
    DWORD byteOffset = 0;

#if _WIN32_WINNT >= 0x0601
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* buffer = nullptr;
    while (true) {
        if (GetLogicalProcessorInformationEx(RelationAll, buffer, &returnLength))
            break;
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            free(buffer);
            buffer = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)malloc(returnLength);
            if (!buffer)
                return;
        } else {
            free(buffer);
            return;
        }
    }
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* ptr = buffer;
    while ((ptr->Size > 0) && (byteOffset + ptr->Size <= returnLength)) {
        switch (ptr->Relationship) {
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
#else
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION* buffer = nullptr;
    while (true)
    {
        if (GetLogicalProcessorInformation(buffer, &returnLength))
            break;
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            free(buffer);
            buffer = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION*)malloc(returnLength);
            if (!buffer)
                return;
        }
        else
        {
            free(buffer);
            return;
        }
    }
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION* ptr = buffer;
    while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength)
    {
        switch (ptr->Relationship) {
        case RelationNumaNode:
            nodes++;
            break;
        case RelationProcessorCore:
            cores++;
            threads += popcount(ptr->ProcessorMask);
            break;
        default:
            break;
        }
        byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
        ptr++;
    }
#endif
    free(buffer);

    for (int n = 0; n < nodes; n++)
        for (int i = 0; i < cores / nodes; i++)
            threadToNode.push_back(n);
    for (int t = 0; t < threads - cores; t++)
        threadToNode.push_back(t % nodes);
#else
    if (numa_available() == -1)
        return;

    const int maxNode = numa_max_node();
    if (maxNode == 0)
        return;

    std::set<int> nodesToUse;
    bitmask* runNodes = numa_get_run_node_mask();
    int nBits = numa_bitmask_nbytes(runNodes) * 8;
    for (int i = 0; i < nBits; i++)
        if (numa_bitmask_isbitset(runNodes, i))
            nodesToUse.insert(i);

    std::map<int, NodeInfo> nodeInfo;
    std::string baseDir("/sys/devices/system/cpu");
    for (int i = 0; ; i++) {
        std::string cpuDir(baseDir + "/cpu" + num2Str(i));
        if (i > 0) {
            std::ifstream is(cpuDir + "/online");
            if (!is)
                break;
            std::string line;
            std::getline(is, line);
            if (!is || is.eof() || (line != "1"))
                continue;
        }

        int node = -1;
        for (int n = 0; n <= maxNode; n++) {
            std::ifstream is(cpuDir + "/node" + num2Str(n));
            if (is) {
                node = n;
                break;
            }
        }
        if (node < 0)
            continue;

        nodeInfo[node].node = node;
        nodeInfo[node].numThreads++;

        std::ifstream is(cpuDir + "/topology/thread_siblings_list");
        if (is) {
            std::string line;
            std::getline(is, line);
            if (is && !is.eof()) {
                auto pos = line.find_first_of(",-");
                if (pos != std::string::npos)
                    line = line.substr(0, pos);
                int num;
                if (str2Num(line, num)) {
                    if (i == num)
                        nodeInfo[node].numCores++;
                }
            }
        }
    }

    std::vector<NodeInfo> nodes;
    for (int node : nodesToUse) {
        auto it = nodeInfo.find(node);
        if (it != nodeInfo.end())
            nodes.push_back(it->second);
    }
    std::sort(nodes.begin(), nodes.end(), [](const NodeInfo& a, const NodeInfo& b) {
        if (a.numCores != b.numCores)
            return a.numCores > b.numCores;
        return a.numThreads > b.numThreads;
    });

    for (const NodeInfo& ni : nodes)
        for (int i = 0; i < ni.numCores; i++)
            threadToNode.push_back(ni.node);

    bool done = false;
    while (!done) {
        done = true;
        for (NodeInfo& ni : nodes) {
            if (ni.numThreads > ni.numCores) {
                threadToNode.push_back(ni.node);
                ni.numThreads--;
                done = false;
            }
        }
    }
#endif
}

void Numa::bindThisThread(size_t idx) const {

  if (idx >= threadToNode.size())
      return;

  int node = threadToNode[idx];

#ifdef _WIN32
#if _WIN32_WINNT >= 0x0601
    GROUP_AFFINITY mask;
    if (GetNumaNodeProcessorMaskEx(node, &mask))
    {
        if (SetThreadGroupAffinity(GetCurrentThread(), &mask, NULL))
            std::cout << "Bind thread " << idx << " to group " << node << std::endl;
        else
            std::cout << "Failed to bind thread " << idx << " to group " << node << std::endl;
    }
#else
    ULONGLONG mask;
    if (GetNumaNodeProcessorMask(node, &mask))
    {
        if (SetThreadAffinityMask(GetCurrentThread(), mask))
            std::cout << "Bind thread " << idx << " to node " << node << std::endl;
        else
            std::cout << "Failed to bind thread " << idx << " to node " << node << std::endl;
    }
#endif
#else
    numa_run_on_node(node);
    numa_set_preferred(node);
    std::cout << "Bind thread " << idx << " to node " << node << std::endl;
#endif
}
