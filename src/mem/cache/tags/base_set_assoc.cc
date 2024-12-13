/*
 * Copyright (c) 2012-2014 ARM Limited
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
 * Definitions of a conventional tag store.
 */

#include "mem/cache/tags/base_set_assoc.hh"

#include <string>

#include "base/intmath.hh"

namespace gem5
{

BaseSetAssoc::BaseSetAssoc(const Params &p)
    :BaseTags(p), allocAssoc(p.assoc), blks(p.size / p.block_size),
     sequentialAccess(p.sequential_access),
     replacementPolicy(p.replacement_policy)
{
    // There must be a indexing policy
    fatal_if(!p.indexing_policy, "An indexing policy is required");

    // Check parameters
    if (blkSize < 4 || !isPowerOf2(blkSize)) {
        fatal("Block size must be at least 4 and a power of 2");
    }
}

void
BaseSetAssoc::tagsInit()
{
    // Initialize all blocks
    for (unsigned blk_index = 0; blk_index < numBlocks; blk_index++) {
        // Locate next cache block
        CacheBlk* blk = &blks[blk_index];

        // Link block to indexing policy
        indexingPolicy->setEntry(blk, blk_index);

        // Associate a data chunk to the block
        blk->data = &dataBlks[blkSize*blk_index];

        // Associate a replacement data entry to the block
        blk->replacementData = replacementPolicy->instantiateEntry();
    }
}

void
BaseSetAssoc::invalidate(CacheBlk *blk)
{
    BaseTags::invalidate(blk);

    // Decrease the number of tags in use
    stats.tagsInUse--;

    // Invalidate replacement data
    replacementPolicy->invalidate(blk->replacementData);

    /* Start EL: Adaptive Cache Compression */
    blk->lastTouchTick = Tick(0);
    /* End EL */
}

void
BaseSetAssoc::moveBlock(CacheBlk *src_blk, CacheBlk *dest_blk)
{
    BaseTags::moveBlock(src_blk, dest_blk);

    // Since the blocks were using different replacement data pointers,
    // we must touch the replacement data of the new entry, and invalidate
    // the one that is being moved.
    replacementPolicy->invalidate(src_blk->replacementData);
    src_blk->lastTouchTick = Tick(0);
    replacementPolicy->reset(dest_blk->replacementData);
    dest_blk->lastTouchTick = curTick();
}

CacheBlk* BaseSetAssoc::findCompressedDataReplacement(Addr addr,
                        const bool is_secure,
                        const std::size_t req_size,
                        std::vector<CacheBlk*>& evicts,
                        bool update_expansion)
{
	CacheBlk* replacement = NULL;
    CacheBlk* update_blk = NULL;
	std::vector<ReplaceableEntry*> valid_blocks;
	
	
	
	// Get all blocks in the set
    const std::vector<ReplaceableEntry*> blocks = indexingPolicy->getPossibleEntries(addr);
	Addr tag = extractTag(addr);
	unsigned segments = 0;
	// Max set size in segments
    unsigned max_set_segments = 32;
    

    // Process all blocks in the cache set
    for (const auto& block : blocks) {
        CacheBlk* cache_blk = static_cast<CacheBlk*>(block);

        if (cache_blk->isValid()) {
            // If the block needs updating, set it as replacement block
            if (cache_blk->matchTag(tag, is_secure) && update_expansion) {
                update_blk = cache_blk;
                replacement = update_blk;
            }
            // Otherwise, add valid blocks to the list and update the set size
            else {
                valid_blocks.push_back(block);
                segments += ((cache_blk->cSize + 63) / 64);  // Convert size to segments
            }
        } else {
            // Invalid block, can immediately be evicted
            replacement = cache_blk;
            evicts.push_back(replacement);
        }
    }

    
    // Calculate remaining space needed in segments
    int extra_segments = (req_size + 63) / 64 - (max_set_segments - segments);

    // If not enough space or no replacement candidate found, use LRU for selection
    if (extra_segments > 0 || !replacement) {
        replacement = static_cast<CacheBlk*>(replacementPolicy->getVictim(valid_blocks));
        evicts.push_back(replacement);
    }

    extra_segments -= ((replacement->cSize + 63) / 64);

    // If still not enough space, find larger valid blocks to evict
    if (extra_segments > 0) {
        std::vector<ReplaceableEntry*> large_blocks;

        // Find valid blocks large enough to meet the space requirement
        for (const auto& block : valid_blocks) {
            CacheBlk* cache_blk = static_cast<CacheBlk*>(block);
            if (((cache_blk->cSize + 63) / 64) >= extra_segments && cache_blk != replacement) {
                large_blocks.push_back(block);
            }
        }

        // Evict from the filtered list if necessary
        if (!large_blocks.empty()) {
            replacement = static_cast<CacheBlk*>(replacementPolicy->getVictim(large_blocks));
            evicts.push_back(replacement);
        }
    }

    extra_segments -= ((replacement->cSize + 63) / 64);

    // Return the block being updated, or the selected block to be replaced
    return (update_blk) ? update_blk : replacement;
}


} // namespace gem5
