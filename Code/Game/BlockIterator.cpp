#include "BlockIterator.h"

#include "Chunk.h"
#include "ChunkUtils.h"

BlockIterator::BlockIterator()
    : m_chunk(nullptr), m_blockIndex(-1)
{
}

BlockIterator::BlockIterator(Chunk* chunk, int blockIndex)
    : m_chunk(chunk), m_blockIndex(blockIndex)
{
}

BlockIterator::BlockIterator(Chunk* chunk, const IntVec3& localCoords)
    : m_chunk(chunk)
{
    if (chunk)
    {
        m_blockIndex = LocalCoordsToIndex(localCoords);
    }
}

BlockIterator::BlockIterator(const BlockIterator& other)
    : m_chunk(other.m_chunk), m_blockIndex(other.m_blockIndex) 
{
}

BlockIterator::~BlockIterator()
{
}

bool BlockIterator::MoveEast()
{
    if (!m_chunk)
        return false;
    
    IntVec3 coords = GetLocalCoords();
    coords.x++;
    
    if (coords.x >= CHUNK_SIZE_X)
        return false;
    
    m_blockIndex = LocalCoordsToIndex(coords);
    return true;
}

bool BlockIterator::MoveWest()
{
    if (!m_chunk)
        return false;
    
    IntVec3 coords = GetLocalCoords();
    coords.x--;
    
    if (coords.x < 0)
        return false;
    
    m_blockIndex = LocalCoordsToIndex(coords);
    return true;
}

bool BlockIterator::MoveNorth()
{
    if (!m_chunk)
        return false;
    
    IntVec3 coords = GetLocalCoords();
    coords.y++;
    
    if (coords.y >= CHUNK_SIZE_Y)
        return false;
    
    m_blockIndex = LocalCoordsToIndex(coords);
    return true;
}

bool BlockIterator::MoveSouth()
{
    if (!m_chunk)
        return false;
    
    IntVec3 coords = GetLocalCoords();
    coords.y--;
    
    if (coords.y < 0)
        return false;
    
    m_blockIndex = LocalCoordsToIndex(coords);
    return true;
}

bool BlockIterator::MoveUp()
{
    if (!m_chunk)
        return false;
    
    IntVec3 coords = GetLocalCoords();
    coords.z++;
    
    if (coords.z >= CHUNK_SIZE_Z)
        return false;
    
    m_blockIndex = LocalCoordsToIndex(coords);
    return true;
}

bool BlockIterator::MoveDown()
{
    if (!m_chunk)
        return false;
    
    IntVec3 coords = GetLocalCoords();
    coords.z--;
    
    if (coords.z < 0)
        return false;
    
    m_blockIndex = LocalCoordsToIndex(coords);
    return true;
}

bool BlockIterator::Move(Direction dir)
{
    switch (dir)
    {
    case DIRECTION_EAST:  return MoveEast();
    case DIRECTION_WEST:  return MoveWest();
    case DIRECTION_NORTH: return MoveNorth();
    case DIRECTION_SOUTH: return MoveSouth();
    case DIRECTION_UP:    return MoveUp();
    case DIRECTION_DOWN:  return MoveDown();
    default: return false;
    }
}

BlockIterator BlockIterator::GetNeighborCrossBoundary(Direction dir) const
{
   if (!m_chunk)
        return BlockIterator(); 
    switch (dir)
    {
    case DIRECTION_EAST:  
        if ((m_blockIndex & CHUNK_MASK_X) == CHUNK_MAX_X)  
        {
            Chunk* eastChunk = m_chunk->GetNeighbor(DIRECTION_EAST);
            if (!eastChunk || eastChunk->GetState() != ChunkState::ACTIVE)
                return BlockIterator();
            
            int newIndex = (m_blockIndex & ~CHUNK_MASK_X);
            return BlockIterator(eastChunk, newIndex);
        }
        else
        {
            return BlockIterator(m_chunk, m_blockIndex + 1);
        }
        
    case DIRECTION_WEST: 
        if ((m_blockIndex & CHUNK_MASK_X) == 0)  
        {
            Chunk* westChunk = m_chunk->GetNeighbor(DIRECTION_WEST);
            if (!westChunk)
                return BlockIterator();
            
            int newIndex = (m_blockIndex & ~CHUNK_MASK_X) | CHUNK_MAX_X;
            return BlockIterator(westChunk, newIndex);
        }
        else
        {
            return BlockIterator(m_chunk, m_blockIndex - 1);
        }
        
    case DIRECTION_NORTH:  
        if (((m_blockIndex & CHUNK_MASK_Y) >> CHUNK_BITS_X) == CHUNK_MAX_Y) 
        {
            Chunk* northChunk = m_chunk->GetNeighbor(DIRECTION_NORTH);
            if (!northChunk)
                return BlockIterator();
            
            int newIndex = (m_blockIndex & ~CHUNK_MASK_Y);
            return BlockIterator(northChunk, newIndex);
        }
        else
        {
            return BlockIterator(m_chunk, m_blockIndex + CHUNK_SIZE_X);
        }
        
    case DIRECTION_SOUTH:  
        if (((m_blockIndex & CHUNK_MASK_Y) >> CHUNK_BITS_X) == 0) 
        {
            Chunk* southChunk = m_chunk->GetNeighbor(DIRECTION_SOUTH);
            if (!southChunk)
                return BlockIterator();
            
            int newIndex = (m_blockIndex & ~CHUNK_MASK_Y) | (CHUNK_MAX_Y << CHUNK_BITS_X);
            return BlockIterator(southChunk, newIndex);
        }
        else
        {
            return BlockIterator(m_chunk, m_blockIndex - CHUNK_SIZE_X);
        }
        
    case DIRECTION_UP: 
        if (((m_blockIndex & CHUNK_MASK_Z) >> CHUNK_BITS_XY) == CHUNK_MAX_Z)  
        {
            return BlockIterator();
        }
        else
        {
            return BlockIterator(m_chunk, m_blockIndex + (CHUNK_SIZE_X * CHUNK_SIZE_Y));
        }
        
    case DIRECTION_DOWN: 
        if (((m_blockIndex & CHUNK_MASK_Z) >> CHUNK_BITS_XY) == 0)  
        {
            return BlockIterator();
        }
        else
        {
            return BlockIterator(m_chunk, m_blockIndex - (CHUNK_SIZE_X * CHUNK_SIZE_Y));
        }
        
    default:
        return BlockIterator();
    }
}

BlockIterator BlockIterator::GetNeighbor(Direction dir) const
{
    //copy current iterator then->Move!
    BlockIterator neighbor(*this);  
    if (!neighbor.Move(dir)) //出界！~
    {
        neighbor.m_chunk = nullptr;
        neighbor.m_blockIndex = -1;
    }
    return neighbor;
}

bool BlockIterator::HasNeighbor(Direction dir) const
{
    BlockIterator neighbor = GetNeighbor(dir);
    return neighbor.IsValid();
}

// Block* BlockIterator::GetBlock() const
// {
//     if (!IsValid())
//         return nullptr;
//     
//     return &m_chunk->m_blocks[m_blockIndex];
// }
//
// uint8_t BlockIterator::GetBlockType() const
// {
//     Block* block = GetBlock();
//     return block ? block->m_typeIndex : BLOCK_TYPE_AIR;
// }
//
// bool BlockIterator::IsValid() const
// {
//     return m_chunk != nullptr && 
//            m_blockIndex >= 0 && 
//            m_blockIndex < CHUNK_TOTAL_BLOCKS;
// }

bool BlockIterator::IsOpaque() const
{
    uint8_t type = GetBlockType();
	const BlockDefinition& def = BlockDefinition::GetBlockDef(type);
    return def.m_isOpaque;
}

bool BlockIterator::IsTransparent() const
{
    return !IsOpaque();
}

bool BlockIterator::IsSky() const
{
    Block* block = GetBlock();
    return block ? block->IsSky() : false;
}

bool BlockIterator::IsLightDirty() const
{
    Block* block = GetBlock();
    return block ? block->IsLightDirty() : false;
}

uint8_t BlockIterator::GetOutdoorLight() const
{
    Block* block = GetBlock();
    return block ? block->GetOutdoorLight() : 0;
}

uint8_t BlockIterator::GetIndoorLight() const
{
    Block* block = GetBlock();
    return block ? block->GetIndoorLight() : 0;
}

IntVec3 BlockIterator::GetLocalCoords() const
{
    if (!IsValid())
        return IntVec3(-1, -1, -1);
    
    return IndexToLocalCoords(m_blockIndex);
}

IntVec3 BlockIterator::GetGlobalCoords() const
{
    if (!m_chunk)
        return IntVec3(-1, -1, -1);
    
    IntVec3 localCoords = GetLocalCoords();
    return IntVec3(
        m_chunk->m_chunkCoords.x * CHUNK_SIZE_X + localCoords.x,
        m_chunk->m_chunkCoords.y * CHUNK_SIZE_Y + localCoords.y,
        localCoords.z
    );
}

BlockIterator& BlockIterator::operator++()
{
    m_blockIndex++;
    if (m_blockIndex >= CHUNK_TOTAL_BLOCKS)
    {
        m_blockIndex = CHUNK_TOTAL_BLOCKS; 
    }
    return *this;
}

bool BlockIterator::operator==(const BlockIterator& other) const
{
    return m_chunk == other.m_chunk && m_blockIndex == other.m_blockIndex;
}

bool BlockIterator::operator!=(const BlockIterator& other) const
{
    return !(*this == other);
}

BlockIterator& BlockIterator::operator=(const BlockIterator& other)
{
    if (this != &other)
    {
        m_chunk = other.m_chunk;
        m_blockIndex = other.m_blockIndex;
    }
    return *this;
}

bool BlockIterator::IsIndexValid(int index) const
{
    return index < CHUNK_TOTAL_BLOCKS && index >= 0 ;
}

IntVec3 BlockIterator::GetDirectionOffset(Direction dir)
{
    switch (dir)
    {
    case DIRECTION_EAST:  return IntVec3(1, 0, 0);
    case DIRECTION_WEST:  return IntVec3(-1, 0, 0);
    case DIRECTION_NORTH: return IntVec3(0, 1, 0);
    case DIRECTION_SOUTH: return IntVec3(0, -1, 0);
    case DIRECTION_UP:    return IntVec3(0, 0, 1);
    case DIRECTION_DOWN:  return IntVec3(0, 0, -1);
    default: return IntVec3(0, 0, 0);
    }
}

Direction GetOppositeDirection(Direction dir)
{
    switch (dir)
    {
    case DIRECTION_EAST:  return DIRECTION_WEST;
    case DIRECTION_WEST:  return DIRECTION_EAST;
    case DIRECTION_NORTH: return DIRECTION_SOUTH;
    case DIRECTION_SOUTH: return DIRECTION_NORTH;
    case DIRECTION_UP:    return DIRECTION_DOWN;
    case DIRECTION_DOWN:  return DIRECTION_UP;
    default: return NUM_DIRECTIONS;
    }
}

