#pragma once
#include "Gamecommon.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/IntVec3.h"
#include "Engine/Math/Vec3.hpp"

class Block;
// local~index
int LocalCoordsToIndex(const IntVec3& localCoords);
int LocalCoordsToIndex(int x, int y, int z);
int IndexToLocalX(int index);
int IndexToLocalY(int index);
int IndexToLocalZ(int index);
IntVec3 IndexToLocalCoords(int index);

// world
int GlobalCoordsToIndex(const IntVec3& globalCoords);
int GlobalCoordsToIndex(int x, int y, int z);
IntVec2 GetChunkCoords(const IntVec3& globalCoords);
IntVec2 GetChunkCenter(const IntVec2& chunkCoords);
IntVec3 GlobalCoordsToLocalCoords(const IntVec3& globalCoords);
IntVec3 GetGlobalCoords(const IntVec2& chunkCoords, int blockIndex);
IntVec3 GetGlobalCoords(const IntVec2& chunkCoords, const IntVec3& localCoords);
IntVec3 GetGlobalCoords(const Vec3& position);
uint8_t GetAttachmentFacingValue(Direction attachDir);
Rgba8 GetRedstoneWireColor(uint8_t power);
Rgba8 GetCropGrowthColor(uint8_t type);

// block
bool IsSolid(uint8_t type);
bool IsLiquid(uint8_t type);
bool IsFoliage(uint8_t type);
bool IsLog(uint8_t type);
bool IsSnow(uint8_t type);
bool IsOpaque(uint8_t type);
bool IsNonGroundCover(uint8_t type);
bool IsRedstoneComponent(uint8_t blockType);
bool IsRedstonePowerSource(uint8_t blockType);
bool IsRedstoneConductor(uint8_t blockType);
bool IsRedstoneWire(uint8_t blockType);
bool IsRedstonePowerable(uint8_t blockType);
bool IsPowerSource(uint8_t blockType);
bool IsCrop(uint8_t blockType);
bool IsUnderwaterProduct(uint8_t blockType);
bool IsPlantBillboard(uint8_t blockType);
bool IsMatureCrop(uint8_t blockType);
bool IsBlockTransparent(uint8_t blockType);
void GetPerpendicularDirectionsForLeftAndRight(Direction facing, 
    Direction& leftDir, Direction& rightDir);

