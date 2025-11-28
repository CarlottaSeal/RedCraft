#include "ChunkUtils.h"
#include <cmath>

#include "Block.h"

int LocalCoordsToIndex(const IntVec3& localCoords)
{
    return localCoords.x | (localCoords.y << CHUNK_BITS_X) | (localCoords.z << CHUNK_BITS_XY);
}

int LocalCoordsToIndex(int x, int y, int z)
{
    return x | (y << CHUNK_BITS_X) | (z << CHUNK_BITS_XY);
}

int IndexToLocalX(int index)
{
    return index & CHUNK_MASK_X;
}

int IndexToLocalY(int index)
{
    return (index & CHUNK_MASK_Y) >> CHUNK_BITS_X;
}

int IndexToLocalZ(int index)
{
    return (index & CHUNK_MASK_Z) >> CHUNK_BITS_XY;
}

IntVec3 IndexToLocalCoords(int index)
{
    return IntVec3(
        IndexToLocalX(index),
        IndexToLocalY(index),
        IndexToLocalZ(index)
    );
}

int GlobalCoordsToIndex(const IntVec3& globalCoords)
{
    IntVec3 localCoords = GlobalCoordsToLocalCoords(globalCoords);
    return LocalCoordsToIndex(localCoords);
}

int GlobalCoordsToIndex(int x, int y, int z)
{
    return GlobalCoordsToIndex(IntVec3(x, y, z));
}

IntVec2 GetChunkCoords(const IntVec3& globalCoords)
{
    int chunkX = FloorDivision(globalCoords.x, CHUNK_SIZE_X);
    int chunkY = FloorDivision(globalCoords.y, CHUNK_SIZE_Y);
    return IntVec2(chunkX, chunkY);
}

IntVec2 GetChunkCenter(const IntVec2& chunkCoords)
{
    int centerX = (chunkCoords.x * CHUNK_SIZE_X) + (CHUNK_SIZE_X / 2);
    int centerY = (chunkCoords.y * CHUNK_SIZE_Y) + (CHUNK_SIZE_Y / 2);
    return IntVec2(centerX, centerY);
}

IntVec3 GlobalCoordsToLocalCoords(const IntVec3& globalCoords)
{
    int localX = globalCoords.x & CHUNK_MAX_X;
    int localY = globalCoords.y & CHUNK_MAX_Y;
    int localZ = globalCoords.z;  
    
    if (globalCoords.x < 0 && localX != 0)
    {
        localX = CHUNK_SIZE_X - ((-globalCoords.x) & CHUNK_MAX_X);
    }
    if (globalCoords.y < 0 && localY != 0)
    {
        localY = CHUNK_SIZE_Y - ((-globalCoords.y) & CHUNK_MAX_Y);
    }
    
    return IntVec3(localX, localY, localZ);
}

IntVec3 GetGlobalCoords(const IntVec2& chunkCoords, int blockIndex)
{
    IntVec3 localCoords = IndexToLocalCoords(blockIndex);
    return GetGlobalCoords(chunkCoords, localCoords);
}

IntVec3 GetGlobalCoords(const IntVec2& chunkCoords, const IntVec3& localCoords)
{
    int globalX = (chunkCoords.x * CHUNK_SIZE_X) + localCoords.x;
    int globalY = (chunkCoords.y * CHUNK_SIZE_Y) + localCoords.y;
    int globalZ = localCoords.z;
    
    return IntVec3(globalX, globalY, globalZ);
}

IntVec3 GetGlobalCoords(const Vec3& position)
{
    int globalX = static_cast<int>(std::floor(position.x));
    int globalY = static_cast<int>(std::floor(position.y));
    int globalZ = static_cast<int>(std::floor(position.z));
    
    return IntVec3(globalX, globalY, globalZ);
}

uint8_t GetAttachmentFacingValue(Direction attachDir)
{
	switch (attachDir)
	{
	case DIRECTION_DOWN:  return 0;
	case DIRECTION_UP:    return 1;
	case DIRECTION_NORTH: return 2;
	case DIRECTION_SOUTH: return 3;
	case DIRECTION_EAST:  return 4;
	case DIRECTION_WEST:  return 5;
	default: return 0;
	}
}

Rgba8 GetRedstoneWireColor(uint8_t power)
{
	// 无信号：深红色 (76, 0, 0)
	// 满信号：亮红色 (255, 25, 25)
	float t = (float)power / 15.0f;
	uint8_t r = (uint8_t)(76 + t * 179);     // 76 -> 255
	uint8_t g = (uint8_t)(0 + t * 25);       // 0 -> 25
	uint8_t b = (uint8_t)(0 + t * 25);       // 0 -> 25
	return Rgba8(r, g, b, 255);
}

bool IsSolid(uint8_t type)
{
	switch (type)
	{
	case BLOCK_TYPE_STONE:
	case BLOCK_TYPE_DIRT:
	case BLOCK_TYPE_GRASS:
	case BLOCK_TYPE_SAND:
	case BLOCK_TYPE_COBBLESTONE:
	case BLOCK_TYPE_CHISELED_BRICK:
	case BLOCK_TYPE_OBSIDIAN:
	case BLOCK_TYPE_COAL:
	case BLOCK_TYPE_IRON:
	case BLOCK_TYPE_GOLD:
	case BLOCK_TYPE_DIAMOND:
	case BLOCK_TYPE_GLOWSTONE:
	case BLOCK_TYPE_ICE:
	case BLOCK_TYPE_OAK_LOG:
	case BLOCK_TYPE_BIRCH_LOG:
	case BLOCK_TYPE_CACTUS_LOG:
	case BLOCK_TYPE_SPRUCE_LOG:
	case BLOCK_TYPE_JUNGLE_LOG:
	case BLOCK_TYPE_ACACIA_LOG:
	case BLOCK_TYPE_BIRCH_PLANKS:
	case BLOCK_TYPE_OAK_PLANKS:
	case BLOCK_TYPE_JUNGLE_PLANKS:
	case BLOCK_TYPE_SPRUCE_PLANKS:
	case BLOCK_TYPE_ACACIA_PLANKS:
	case BLOCK_TYPE_SNOW:
		return true;
	default:
		return false;
	}
}

bool IsLiquid(uint8_t type)
{
	return (type == BLOCK_TYPE_WATER || type == BLOCK_TYPE_LAVA);
}

bool IsFoliage(uint8_t type)
{
	return (type == BLOCK_TYPE_BIRCH_LEAVES
        || type == BLOCK_TYPE_OAK_LEAVES
        || type == BLOCK_TYPE_ACACIA_LEAVES
        || type == BLOCK_TYPE_JUNGLE_LEAVES
        || type == BLOCK_TYPE_SPRUCE_LEAVES
        || type == BLOCK_TYPE_SPRUCE_LEAVES_SNOW);
}

bool IsLog(uint8_t type)
{
	switch (type) 
    {
	case BLOCK_TYPE_OAK_LOG:
	case BLOCK_TYPE_BIRCH_LOG:
	case BLOCK_TYPE_SPRUCE_LOG:
	case BLOCK_TYPE_JUNGLE_LOG:
	case BLOCK_TYPE_ACACIA_LOG:
	case BLOCK_TYPE_CACTUS_LOG:
		return true;
	default:
		return false;
	}
}

bool IsSnow(uint8_t type)
{
    return type == BLOCK_TYPE_SNOW;
}

bool IsOpaque(uint8_t blockType)
{
	if (blockType == BLOCK_TYPE_AIR) return false;
	if (IsLiquid(blockType)) return false;
	if (IsFoliage(blockType)) return false;
	if (blockType == BLOCK_TYPE_ICE) return false;
	if (IsRedstoneComponent(blockType)) return false;  // 红石组件不算不透明
    
	return true;
}

bool IsNonGroundCover(uint8_t type)
{
	return (type == BLOCK_TYPE_AIR) || IsFoliage(type) || IsLiquid(type);
}

bool IsRedstoneComponent(uint8_t blockType)
{
	return blockType >= 46 && blockType <= 71;
}

bool IsRedstonePowerSource(uint8_t blockType)
{
	switch (blockType)
	{
	case BLOCK_TYPE_REDSTONE_TORCH:
	case BLOCK_TYPE_REDSTONE_BLOCK:
	case BLOCK_TYPE_LEVER:
	case BLOCK_TYPE_BUTTON_STONE:
	case BLOCK_TYPE_BUTTON_WOOD:
	case BLOCK_TYPE_PRESSURE_PLATE_STONE:
	case BLOCK_TYPE_PRESSURE_PLATE_WOOD:
	case BLOCK_TYPE_REPEATER_ON:
	case BLOCK_TYPE_OBSERVER:
		return true;
	default:
		return false;
	}
}

bool IsRedstoneConductor(uint8_t blockType)
{
	// 实心不透明方块可以传导红石信号
	return blockType != BLOCK_TYPE_AIR && 
		   blockType < 50 &&  // 非红石组件
		   IsSolid(blockType) && 
		   IsOpaque(blockType);
}

bool IsRedstoneWire(uint8_t blockType)
{
	return blockType >= BLOCK_TYPE_REDSTONE_WIRE_DOT && 
		   blockType <= BLOCK_TYPE_REDSTONE_WIRE_CROSS;
}

bool IsRedstonePowerable(uint8_t blockType)
{
	if (IsRedstoneWire(blockType)) return true;
	switch (blockType)
	{
	case BLOCK_TYPE_REDSTONE_TORCH:
	case BLOCK_TYPE_REDSTONE_TORCH_OFF:
	case BLOCK_TYPE_REDSTONE_BLOCK:
	case BLOCK_TYPE_REDSTONE_LAMP:
	case BLOCK_TYPE_REDSTONE_LAMP_ON:
	case BLOCK_TYPE_REPEATER_OFF:
	case BLOCK_TYPE_REPEATER_ON:
	case BLOCK_TYPE_LEVER:
	case BLOCK_TYPE_BUTTON_STONE:
	case BLOCK_TYPE_BUTTON_WOOD:
	case BLOCK_TYPE_PRESSURE_PLATE_STONE:
	case BLOCK_TYPE_PRESSURE_PLATE_WOOD:
	case BLOCK_TYPE_PISTON:
	case BLOCK_TYPE_STICKY_PISTON:
	case BLOCK_TYPE_PISTON_HEAD:
	case BLOCK_TYPE_OBSERVER:
		return true;
	default:
		return false;
	}
}

bool IsPowerSource(uint8_t blockType)
{
	switch (blockType)
	{
	case BLOCK_TYPE_REDSTONE_BLOCK:
	case BLOCK_TYPE_REDSTONE_TORCH:
	case BLOCK_TYPE_LEVER:
	case BLOCK_TYPE_BUTTON_STONE:
	case BLOCK_TYPE_BUTTON_WOOD:
	case BLOCK_TYPE_PRESSURE_PLATE_STONE:
	case BLOCK_TYPE_PRESSURE_PLATE_WOOD:
	case BLOCK_TYPE_REPEATER_ON:
		return true;
	default:
		return false;
	}
}

bool IsCrop(uint8_t blockType)
{
	return false;
}

bool IsMatureCrop(uint8_t blockType)
{
	return false;
}

void GetPerpendicularDirectionsForLeftAndRight(Direction facing, Direction& leftDir, Direction& rightDir)
{
	switch (facing)
	{
	case DIRECTION_NORTH:
		leftDir = DIRECTION_WEST;
		rightDir = DIRECTION_EAST;
		break;
	case DIRECTION_SOUTH:
		leftDir = DIRECTION_EAST;
		rightDir = DIRECTION_WEST;
		break;
	case DIRECTION_EAST:
		leftDir = DIRECTION_NORTH;
		rightDir = DIRECTION_SOUTH;
		break;
	case DIRECTION_WEST:
		leftDir = DIRECTION_SOUTH;
		rightDir = DIRECTION_NORTH;
		break;
	default:
		leftDir = DIRECTION_WEST;
		rightDir = DIRECTION_EAST;
		break;
	}
}

IntVec3 GetDirectionVector(Direction dir)
{
	switch (dir)
	{
	case DIRECTION_NORTH: return IntVec3(0, 1, 0);
	case DIRECTION_SOUTH: return IntVec3(0, -1, 0);
	case DIRECTION_EAST:  return IntVec3(1, 0, 0);
	case DIRECTION_WEST:  return IntVec3(-1, 0, 0);
	case DIRECTION_UP:    return IntVec3(0, 0, 1);
	case DIRECTION_DOWN:  return IntVec3(0, 0, -1);
	default: return IntVec3(0, 0, 0);
	}
}
