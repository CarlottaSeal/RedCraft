#pragma once
#include "Block.h"
#include "Chunk.h"
#include "GameCommon.hpp"
#include "Engine/Math/IntVec3.h"

//class Chunk;
//class Block;

class BlockIterator
{
public:
    BlockIterator();
    BlockIterator(Chunk* chunk, int blockIndex);
    BlockIterator(Chunk* chunk, const IntVec3& localCoords);
    BlockIterator(const BlockIterator& other);
    ~BlockIterator();

    bool MoveEast();   // +X
    bool MoveWest();   // -X
    bool MoveNorth();  // +Y
    bool MoveSouth();  // -Y
    bool MoveUp();     // +Z
    bool MoveDown();   // -Z
    bool Move(Direction dir);
    
    Chunk* GetChunk() const { return m_chunk; }
    
    BlockIterator GetNeighborCrossBoundary(Direction dir) const;
    BlockIterator GetNeighbor(Direction dir) const;
    bool HasNeighbor(Direction dir) const;

    inline bool IsValid() const
    {
        return m_chunk != nullptr && 
               m_blockIndex >= 0 && 
               m_blockIndex < CHUNK_TOTAL_BLOCKS;
    }

    inline Block* GetBlock() const
    {
        if (!IsValid())
            return nullptr;
    
        return &m_chunk->m_blocks[m_blockIndex];
    }
    inline uint8_t GetBlockType() const
    {
        Block* block = GetBlock();
        return block ? block->m_typeIndex : BLOCK_TYPE_AIR;
    }

    //Block* GetBlock() const;
    //uint8_t GetBlockType() const;
    //bool IsValid() const;
    bool IsOpaque() const;
    bool IsTransparent() const;
    bool IsSky() const;
    bool IsLightDirty() const;
    uint8_t GetOutdoorLight() const;
    uint8_t GetIndoorLight() const;
    
    int GetIndex() const { return m_blockIndex; }
    IntVec3 GetLocalCoords() const;
    IntVec3 GetGlobalCoords() const;
    
    BlockIterator& operator++(); 
    bool operator==(const BlockIterator& other) const;
    bool operator!=(const BlockIterator& other) const;
    BlockIterator& operator=(const BlockIterator& other);

protected:
    bool IsIndexValid(int index) const;
    static IntVec3 GetDirectionOffset(Direction dir);

protected:
    Chunk* m_chunk = nullptr;
    int m_blockIndex = -1;
};

Direction GetOppositeDirection(Direction dir);
