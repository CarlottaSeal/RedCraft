#pragma once
#include <map>
#include <mutex>
#include <set>
#include <vector>

#include "BlockIterator.h"
#include "Gamecommon.hpp"
#include "Generator/WorldGenPipeline.h"
#include "Weather/WeatherSystem.h"

class WeatherParticleRenderer;
class CloudSystem;
class MediumSystem;
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
    friend class CropSystem;
    
public:
    World(Game* owner);
    ~World();

    void Update(float deltaSeconds);
    void Render() const;

    
    Chunk* GetChunkFromPlayerCameraPosition(Vec3 cameraPos);
    Chunk* GetChunk(int chunkX, int chunkY);
    Block GetBlockAtWorldCoords(int worldX, int worldY, int worldZ);
    BlockIterator GetBlockIterator(const IntVec3& globalCoords);
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
    void OnBlockPlaced(const BlockIterator& block);
    void OnBlockRemoved(const BlockIterator& block, uint8_t oldType);
    void OnBlockStateChanged(const BlockIterator& block, uint8_t oldType, uint8_t newType);

    const std::map<IntVec2, Chunk*>& GetActiveChunks();

    std::string GetWorldTypeName() const;
    std::string GetSaveSubdirectory() const;
    
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
    void SaveAllChunks();
    
    void ProcessCompletedJobs();
    void SubmitNewActivateJobs();
    void RebuildDirtyMeshes(int maxPerFrame = 2);

    void ComputeCorrectLightInfluence(const BlockIterator& iter, 
                                      uint8_t& outOutdoorLight, 
                                      uint8_t& outIndoorLight);

    void ScanAndRegisterCropsInChunk(Chunk* chunk);
    void UnregisterCropsInChunk(const IntVec2& chunkCoords);

    void BindWorldConstansBuffer() const;
    void RenderTransparent() const;
    void RenderBlockHighlight() const;

private:
    bool m_isDebugging = false;
    bool m_isDebugPrinting = false;

public:
    Game* m_owner;
    WorldGenPipeline* m_worldGenPipeline;
    bool m_hasDirtyChunk = false;
    std::string m_worldName = "";

    BlockHighlight m_highlightedBlock;

    RedstoneSimulator* m_redstoneSimulator = nullptr;
    CropSystem* m_cropSystem = nullptr;
    WeatherSystem*  m_weatherSystem = nullptr;
    MediumSystem*   m_mediumSystem = nullptr;
    CloudSystem*    m_cloudSystem = nullptr;
    WeatherParticleRenderer* m_weatherParticleRenderer = nullptr; 

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

    static const int SORT_CHUNK_FREQUENCY = 3;
    const int MAX_SORTED_CHUNKS = 25;
    const float MAX_SORT_DISTANCE = 300.0f * 300.0f;
protected:
    std::map<IntVec2, Chunk*> m_activeChunks;
    std::vector<Chunk*> m_visibleChunks;
    
    std::set<IntVec2> m_queuedChunks;
    std::mutex m_queuedChunksMutex;       
    
    std::map<IntVec2, Chunk*> m_processingChunks; 
    std::mutex m_processingChunksMutex;
    mutable int m_sortFrameCounter = 0;

    int m_generatingChunksCount = 0; //debugging

    float m_debugTimer = 0.0f;
};

