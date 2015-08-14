/* mbed Microcontroller Library
 * Copyright (c) 2006-2015 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __BLOCKCOLLECTION_H__
#define __BLOCKCOLLECTION_H__

#include "mbed-block/Block.h"
#include "mbed-util/Array.h"

/*
    BlockCollection

    Provide unified view over multiple Block objects.
*/
class BlockCollection : public Block
{
public:
    /* Constructor with initial Block. */
    BlockCollection(Block* first)
        :   blockArray(),
            cachedBlock(first),
            cachedStart(0),
            cachedEnd(first->getLength()),
            cachedIndex(0)
    {
        Block::length = first->getLength();
        Block::offset = 0;
        Block::maxLength = first->getLength();

        UAllocTraits_t traits = {0};

        blockArray.init(2, 1, traits);
        blockArray.push_back(first);
    }

    /*  Copy data out from BlockCollection and into destination buffer.
        uint8_t* destination    : Destination buffer
        size_t index            : Index into BlockCollection
        size_t num              : Number of bytes to be copied
    */
    virtual void memcpy(void* destination, std::size_t index, std::size_t num)
    {
        // iterate through the array to find Block matching location.
        for (std::size_t blockIndex = 0;
             (blockIndex < blockArray.get_num_elements()) && (num > 0);
             blockIndex++)
        {
            Block* currentBlock = blockArray.at(blockIndex);

            // check if current fragment holds the location
            if (index < currentBlock->getLength())
            {
                // calculate length to be copied
                std::size_t availableLength = currentBlock->getLength() - index;
                std::size_t actualLength = 0;

                // if the length is larger than what the current fragment holds
                // set actualLength to make an aligned copy.
                if (num > availableLength)
                {
                    actualLength = availableLength;
                    num -= availableLength;
                }
                else
                {
                    actualLength = num;
                    num = 0;
                }

                currentBlock->memcpy(destination, index, actualLength);

                // prepare pointers for next fragment
                destination = ((uint8_t*)destination) + availableLength;
                index = 0;
            }
            else
            {
                // decrement the location with the current fragment's length
                index -= currentBlock->getLength();
            }
        }
    }

    /*  Copy data into BlockCollection from source buffer.
        uint8_t* source     : Source buffer
        size_t index        : Index into BlockCollection
        size_t num          : Number of bytes to be copied
    */
    virtual void memcpy(std::size_t index, const void* source, std::size_t num)
    {
        // iterate through the array to find Block matching location.
        for (std::size_t blockIndex = 0;
             (blockIndex < blockArray.get_num_elements()) && (num > 0);
             blockIndex++)
        {
            Block* currentBlock = blockArray.at(blockIndex);

            // check if current fragment holds the location
            if (index < currentBlock->getLength())
            {
                // calculate length to be copied
                std::size_t availableLength = currentBlock->getLength() - index;
                std::size_t actualLength = 0;

                // if the length is larger than what the current fragment holds
                // set actualLength to make an aligned copy.
                if (num > availableLength)
                {
                    actualLength = availableLength;
                    num -= availableLength;
                }
                else
                {
                    actualLength = num;
                    num = 0;
                }

                currentBlock->memcpy(index, source, actualLength);

                // prepare pointers for next fragment
                source = ((uint8_t*)source) + availableLength;
                index = 0;
            }
            else
            {
                // decrement the location with the current fragment's length
                index -= currentBlock->getLength();
            }
        }
    }

    /*  Subscript operator.
        Provides unified view over Block collections.

        NOTE: taking the address of the returned reference
        and using it with memcpy will only work for the
        current fragment, not across fragments.
    */
    virtual uint8_t& at(const std::size_t index)
    {
        if (index < cachedStart)
        {
            // go backwards in array starting from cachedIndex
            // to find Block matching location.
            while ((cachedIndex != 0) && (index < cachedStart))
            {
                cachedIndex--;
                cachedBlock = blockArray.at(cachedIndex);
                cachedEnd = cachedStart;
                cachedStart -= cachedBlock->getLength();
            }
        }
        else if (index >= cachedEnd)
        {
            // go forwards in array starting from cachedIndex
            // to find Block matching location.
            while ((cachedIndex < blockArray.get_num_elements()) && (index >= cachedEnd))
            {
                cachedIndex++;
                cachedBlock = blockArray.at(cachedIndex);
                cachedStart = cachedEnd;
                cachedEnd += cachedBlock->getLength();
            }
        }

        if ((index >= cachedStart) && (index < cachedEnd))
        {
            return cachedBlock->at(index - cachedStart);
        }
        else
        {
            return dummyByte;
        }
    }

    virtual uint8_t& operator[](const std::size_t index)
    {
        return at(index);
    }

    void push_back(Block* block)
    {
        Block::length += block->getLength();
        Block::maxLength += block->getLength();

        blockArray.push_back(block);
    }

    Block* pop_back()
    {
        Block* retval = NULL;

        std::size_t elements = blockArray.get_num_elements();

        if (elements > 0)
        {
            retval = blockArray.at(elements - 1);
        }

        return retval;
    }

protected:

    /* Default constructor. */
    BlockCollection()
        :   blockArray(),
            cachedBlock(NULL),
            cachedStart(0),
            cachedEnd(0),
            cachedIndex(0)
    {
        Block::length = 0;
        Block::offset = 0;
        Block::maxLength = 0;

        UAllocTraits_t traits = {0};

        blockArray.init(1, 1, traits);
    }

    /* Constructor with estimated number of fragments as input. */
    BlockCollection(const std::size_t size)
        :   blockArray(),
            cachedBlock(NULL),
            cachedStart(0),
            cachedEnd(0),
            cachedIndex(0)
    {
        Block::length = 0;
        Block::offset = 0;
        Block::maxLength = 0;

        UAllocTraits_t traits = {0};

        blockArray.init(size, 1, traits);
    }

private:
    mbed::util::Array<Block*> blockArray;
    Block* cachedBlock;
    std::size_t cachedStart;
    std::size_t cachedEnd;
    std::size_t cachedIndex;
    uint8_t dummyByte;
};

#endif // __BLOCKCOLLECTION_H__
