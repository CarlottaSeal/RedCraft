#pragma once
#include <map>
#include <mutex>
#include <set>
#include <unordered_map>
#include <vector>

#include "BlockIterator.h"
#include "Gamecommon.hpp"
#include "Generator/WorldGenPipeline.h"

class RedstoneSimulator;
class BlockIterator;
struct IntVec2;
struct Vec3;
class Block;
class Chunk;

struct GameRaycastResult3D : public RaycastResult3D
{
    BlockIterator m_hitBlock;      
    Direction m_hitFace;           
    IntVec3 m_hitLocalCoords;      
    IntVec3 m_hitGlobalCoords;     
    
    GameRaycastResult3D() : m_hitFace(NUM_DIRECTIONS) {}
};

struct BlockHighlight
{
    bool m_isValid = false;
    IntVec3 m_worldCoords;
    Direction m_hitFace;
};

class World
{
    friend class Game;
    
public:
    World(Game* owner);
    ~World();

    void Update(float deltaSeconds);
    void Render() const;

    

    Chunk* GetChunkFromPlayerCameraPosition(Vec3 cameraPos);
    Chunk* GetChunk(int chunkX, int chunkY);
    Block GetBlockAtWorldCoords(int worldX, int worldY, int worldZ);
    bool CanConnectToRedstone(const IntVec3& globalPos);

    void ActivateProcessedChunk(Chunk* chunk);
    
    static IntVec2 WorldToChunkXY(const Vec3& worldPos);

    void ProcessDirtyLighting();                         
    void ProcessNextDirtyLightBlock();                    
    void MarkLightingDirty(const BlockIterator& iter);   
    void MarkLightingDirtyIfNotOpaque(const BlockIterator& iter);
    void UndirtyAllBlocksInChunk(Chunk* chunk);

    void UpdateWorldConstants();
    float GetTimeOfDay() const;        
    void UpdateDayNightCycle(float deltaSeconds);
    Rgba8 CalculateSkyColor() const;
    Rgba8 CalculateOutdoorLightColor() const;

    float CalculateLightningStrength() const;
    float CalculateGlowstoneFlicker() const;

    GameRaycastResult3D RaycastVsBlocks(const Vec3& start, const Vec3& direction, float maxDistance);

    //redstone
    void HandleBlockInteraction(const BlockIterator& block);
    void OnBlockPlaced(const BlockIterator& block);
    void OnBlockRemoved(const BlockIterator& block, uint8_t oldType);

    void ToggleDebugMode();
    void ToggleDebugPrintingMode();
    bool IsDebugging() const;
    bool IsDebuggingPrinting() const;
    
private:
    void UpdateTypeToPlace();
    void UpdateDiggingAndPlacing(float deltaSeconds);
    void UpdateAccelerateTime();
    void UpdateVisibleChunks();
    
    bool RegenerateSingleNearestDirtyChunk();
    bool ActivateSingleNearestMissingChunkWithinRange();
    void DeactivateSingleFarthestOutsideRangeIfAny();
    void ActivateChunk(IntVec2 chunkCoords);
    void DeactivateChunk(IntVec2 chunkCoords);

    void ConnectChunkNeighbors(Chunk* chunk);
    void DisconnectChunkNeighbors(Chunk* chunk);
    void ForceDeactivateAllChunks();  
    void SaveAllModifiedChunks();
    
    void ProcessCompletedJobs();
    void SubmitNewActivateJobs();
    void RebuildDirtyMeshes(int maxPerFrame = 2);

    void ComputeCorrectLightInfluence(const BlockIterator& iter, 
                                      uint8_t& outOutdoorLight, 
                                      uint8_t& outIndoorLight);

    void BindWorldConstansBuffer() const;
    void RenderBlockHighlight() const;

private:
    bool m_isDebugging = false;
    bool m_isDebugPrinting = false;

public:
    Game* m_owner;
    WorldGenPipeline* m_worldGenPipeline;
    bool m_hasDirtyChunk = false;

    BlockHighlight m_highlightedBlock;

    //redstone
    RedstoneSimulator* m_redstoneSimulator = nullptr;

    Shader* m_worldShader = nullptr;
    ConstantBuffer* m_worldConstantBuffer = nullptr;

    BlockType m_typeToPlace = BLOCK_TYPE_GLOWSTONE;
    std::deque<BlockIterator> m_dirtyLightBlocks;

    Rgba8 m_skyColor;
    Rgba8 m_outdoorLightColor;
    Rgba8 m_indoorLightColor;
    float m_worldTimeInDays = 0.5f;        // 世界时间（天）
    float m_worldTimeScale = 1.0f;         // 时间缩放
    bool m_isTimeAccelerated = false;      // Y 键加速

    bool m_isRaycastLocked = false;
    GameRaycastResult3D m_currentRaycast;


protected:
    std::map<IntVec2, Chunk*> m_activeChunks;
    std::vector<Chunk*> m_visibleChunks;
    
    std::set<IntVec2> m_queuedChunks;
    std::mutex m_queuedChunksMutex;       
    
    std::map<IntVec2, Chunk*> m_processingChunks; 
    std::mutex m_processingChunksMutex;

    int m_generatingChunksCount = 0; //debugging
    
    std::vector<Chunk*> m_chunks;

    float m_debugTimer = 0.0f;
};

