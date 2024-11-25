/**
 * Copyright (c) 2018-2020 Inria
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

#ifndef __MEM_CACHE_REPLACEMENT_POLICIES_REPLACEABLE_ENTRY_HH__
#define __MEM_CACHE_REPLACEMENT_POLICIES_REPLACEABLE_ENTRY_HH__

#include <cstdint>
#include <memory>

#include "base/compiler.hh"
#include "base/cprintf.hh"
#include <string>

namespace gem5
{

GEM5_DEPRECATED_NAMESPACE(ReplacementPolicy, replacement_policy);
namespace replacement_policy
{

/**
 * The replacement data needed by replacement policies. Each replacement policy
 * should have its own implementation of replacement data.
 */
struct ReplacementData {};

} // namespace replacement_policy

/**
 * A replaceable entry is a basic entry in a 2d table-like structure that needs
 * to have replacement functionality. This entry is located in a specific row
 * and column of the table (set and way in cache nomenclature), which are
 * stored within the entry itself.
 *
 * It contains the replacement data pointer, which must be instantiated by the
 * replacement policy before being used.
 * @sa Replacement Policies
 */
class ReplaceableEntry
{
  protected:
    /**
     * Set to which this entry belongs.
     */
    uint32_t _set;

    /**
     * Way (relative position within the set) to which this entry belongs.
     */
    uint32_t _way;

    /**
     * Compressed size of the cache line (in segments).
     */
    uint8_t cSize;

    /**
     * Compression status:
     * 0: Uncompressed
     * 1: Compressed
     */
    bool cStatus;

    /**
     * Coherence state:
     * M: Modified
     * S: Shared
     * I: Invalid
     * NP: Not Present
     */
     char coherenceState;

  public:
    ReplaceableEntry() : cSize(0), cStatus(false), coherenceState('I') {}
    virtual ~ReplaceableEntry() = default;

    /**
     * Replacement data associated to this entry.
     * It must be instantiated by the replacement policy before being used.
     */
    std::shared_ptr<replacement_policy::ReplacementData> replacementData;

    /**
     * Set both the set and way. Should be called only once.
     *
     * @param set The set of this entry.
     * @param way The way of this entry.
     */
    virtual void
    setPosition(const uint32_t set, const uint32_t way)
    {
        _set = set;
        _way = way;
    }

    /**
     * Get set number.
     *
     * @return The set to which this entry belongs.
     */
    uint32_t getSet() const { return _set; }

    /**
     * Get way number.
     *
     * @return The way to which this entry belongs.
     */
    uint32_t getWay() const { return _way; }

    /**
     * Get the compressed size.
     *
     * @return The compressed size of the cache line.
     */
    uint8_t getCompressedSize() const { return cSize; }

    /**
     * Set the compressed size.
     *
     * @param size The compressed size of the cache line.
     */
    void setCompressedSize(uint8_t size) { cSize = size; }

    /**
     * Get the compression status.
     *
     * @return True if compressed, false otherwise.
     */
    bool getCompressionStatus() const { return cStatus; }

    /**
     * Set the compression status.
     *
     * @param status The compression status (true = compressed).
     */
    void setCompressionStatus(bool status) { cStatus = status; }

    /**
     * Get the coherence state.
     *
     * @return The coherence state of the cache line.
     */
    char getCoherenceState() const { return coherenceState; }

    /**
     * Set the coherence state.
     *
     * @param state The coherence state ('M', 'S', 'I', 'NP').
     */
    void setCoherenceState(char state) { coherenceState = state; }

    /**
     * Prints relevant information about this entry.
     *
     * @return A string containing the contents of this entry.
     */
    virtual std::string
    print() const
    {
        return csprintf("set: %#x way: %#x cSize: %u cStatus: %d coherence: %c",
                        getSet(), getWay(), getCompressedSize(),
                        getCompressionStatus(), getCoherenceState());
    }
};

} // namespace gem5

#endif // __MEM_CACHE_REPLACEMENT_POLICIES_REPLACEABLE_ENTRY_HH_
