/*
 * Copyright (c) 2018 Inria
 * Copyright (c) 2012-2014,2017 ARM Limited
 * All rights reserved.
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2003-2005,2014 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * Definitions of a common framework for indexing policies.
 */

#include "mem/cache/tags/indexing_policies/base.hh"

#include <cstdlib>

#include "base/intmath.hh"
#include "base/logging.hh"
#include "mem/cache/replacement_policies/replaceable_entry.hh"

namespace gem5
{

BaseIndexingPolicy::BaseIndexingPolicy(const Params &p)
    : SimObject(p), assoc(p.assoc),
      numSets(p.size / (p.entry_size * assoc)),
      setShift(floorLog2(p.entry_size)), setMask(numSets - 1), sets(numSets),
      tagShift(setShift + floorLog2(numSets))
{
    fatal_if(!isPowerOf2(numSets), "# of sets must be non-zero and a power " \
             "of 2");
    fatal_if(assoc <= 0, "associativity must be greater than zero");

    // Make space for the entries
    for (uint32_t i = 0; i < numSets; ++i) {
        sets[i].resize(numSegments);
    }
}

//ReplaceableEntry*
//BaseIndexingPolicy::getEntry(const uint32_t set, const uint32_t way) const
//{
//    return sets[set][way];
//}

ReplaceableEntry*
BaseIndexingPolicy::getEntryData(const uint32_t set, const uint32_t segmentOffset, 
                                 const bool cStatus, const size_t cSize) const
{
    // Validate set and segmentOffset indices
    if (set >= sets.size() || segmentOffset >= sets[set].size()) {
        throw std::out_of_range("Index out of bounds");
    }

    ReplaceableEntry* entry = sets[set][segmentOffset];
    if (!entry) {
        throw std::runtime_error("Entry is null");
    }

    // Calculate the byte range based on compression status
    size_t dataStart = segmentOffset * 8; // Convert segmentOffset to byte offset
    size_t dataEnd;

    if (cStatus) {
        // Compressed: cSize is the number of segments, so multiply by 8 to get byte size
        dataEnd = dataStart + (cSize * 8);

    } else {
        // Uncompressed: Always extract one segment (8 bytes)
        dataEnd = dataStart + (8 * 8);

    }

    // Extract the data range and return
    return sets[set][dataStart, dataEnd];
}

void
BaseIndexingPolicy::setEntry(ReplaceableEntry* entry, const uint64_t index)
{
    // Calculate set and way from entry index
    const std::lldiv_t div_result = std::div((long long)index, assoc);
    const uint32_t set = div_result.quot;
    const uint32_t way = div_result.rem;

    // Sanity check
    assert(set < numSets);

    // Assign a free pointer
    sets[set][way] = entry;

    // Inform the entry its position
    entry->setPosition(set, way);
}

// Set an entry pointer to its corresponding set and way
void BaseIndexingPolicy::setEntry(ReplaceableEntry* entry, const uint64_t index)
{
    uint32_t set = index & setMask;        // Get the set index
    uint32_t way = (index >> setShift) & (assoc - 1);  // Get the way index

    sets[set].entries[way] = entry;        // Assign the entry to the set and way
}

Addr
BaseIndexingPolicy::extractTag(const Addr addr) const
{
    return (addr >> tagShift);
}

// Set the compressed size for an entry
void BaseIndexingPolicy::setCompressedSize(uint32_t set, uint32_t way, uint8_t compressedSize)
{
    sets[set].compressedSizes[way] = compressedSize;  // Set the compressed size
}

// Set the compression status for an entry
void BaseIndexingPolicy::setCompressionStatus(uint32_t set, uint32_t way, bool status)
{
    sets[set].compressionStatus[way] = status;  // Set the compression status
}

// Set the coherence state for an entry
void BaseIndexingPolicy::setCoherenceState(uint32_t set, uint32_t way, char state)
{
    sets[set].coherenceStates[way] = state;  // Set the coherence state
}

// Print the cache's set and entry details
std::string BaseIndexingPolicy::printCache() const
{
    std::string result = "";
    for (size_t setIdx = 0; setIdx < sets.size(); ++setIdx) {
        result += "Set " + std::to_string(setIdx) + ":\n";
        for (size_t wayIdx = 0; wayIdx < sets[setIdx].entries.size(); ++wayIdx) {
            result += "  Way " + std::to_string(wayIdx) + " - ";
            result += "CSize: " + std::to_string(sets[setIdx].compressedSizes[wayIdx]) + " ";
            result += "CStatus: " + std::to_string(sets[setIdx].compressionStatus[wayIdx]) + " ";
            result += "Coherence: " + std::string(1, sets[setIdx].coherenceStates[wayIdx]) + "\n";
        }
    }
    return result;
}

// Check if a number is a power of 2
bool BaseIndexingPolicy::isPowerOf2(uint32_t x) const
{
    return (x != 0) && ((x & (x - 1)) == 0);
}

// Calculate the log base 2 of a number
uint32_t BaseIndexingPolicy::floorLog2(uint32_t x) const
{
    return static_cast<uint32_t>(std::log2(x));
}


} // namespace gem5
