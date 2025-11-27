#pragma once
#include <atomic>
#include <vector>

#include "Block.h"
//#include "BlockIterator.h"
#include "ChunkSerializer.h"
#include "Gamecommon.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/IntVec3.h"
#include "Generator/WorldGenPipeline.h"

class World;
class BlockIterator;

enum class ChunkState : int
{
    UNINITIALIZED,           // 初始状态
    QUEUED_FOR_GENERATION,   // 排队等待生成
    GENERATING,              // 正在生成中
    QUEUED_FOR_LOADING,      // 排队等待加载
    LOADING,                 // 正在加载中
    QUEUED_FOR_SAVING,       // 排队等待保存
    SAVING,                  // 正在保存中
    GENERATION_COMPLETE,     // 生成/加载完成，等待激活
    ACTIVE,                  // 已激活，在m_activeChunks中
    MARKED_FOR_DEACTIVATION  // 标记待停用
};

class Chunk
{
    friend class World;
    friend class BlockIterator;
    friend class ChunkSerializer;
    friend class GenerateChunkJob;
    friend class LoadChunkJob;
    friend class SaveChunkJob;
    
    friend class FeaturePlacer;
    friend class WorldGenPipeline;
    
public:
    Chunk(World* owner, IntVec2 chunkCoords);
    ~Chunk();

    void InitializeLighting();

    int GetBlockLocalIndexFromLocalCoords(int x, int y, int z) const;
    void GetLocalCoordsFromIndex(int index, int& x, int& y, int& z) const; //index -> local coords
    IntVec3 GetLocalCoordsFromIndex(int index) const;
    Block GetBlock(int localX, int localY, int localZ) const;
    Vec3 GetBlockWorldPosition(int blockIndex) const;

    void Update(float deltaSeconds);
    void Render() const;

    IntVec2 GetThisChunkCoords() const { return m_chunkCoords; }
    
    void SetNeighbor(Direction dir, Chunk* neighbor);
    Chunk* GetNeighbor(Direction dir) const;

    void MarkSelfDirty();
    void MarkNeighborChunkDirty(Direction dir);
    bool IsOnBoundary(const IntVec3& localCoords, Direction dir) const;

    void UpdateInputForDigAndPlace();
    void DigBlock(const IntVec3& localCoords);
    void PlaceBlock(const IntVec3& localCoords, uint8_t blockType);
    IntVec3 FindDigTarget(const Vec3& worldPos);
    IntVec3 FindPlaceTarget(const Vec3& worldPos);
    bool CanDigBlock(uint8_t blockType);
    bool HasSupport(const IntVec3& localCoords, uint8_t blockType);
    
    //relate to world
    void ReportDirty();

    // save
    void Save();
    bool Load();
    static std::string MakeChunkFilename(const IntVec2& chunkCoords);

    //state
    ChunkState GetState() const { return m_state.load(); }
    void SetState(ChunkState newState) { m_state.store(newState); }

public:
    ChunkGenData m_chunkGenData;
    
protected:
    void GenerateBlocks();
    bool GenerateMesh();
    void GenerateDebug();
    bool ShouldRenderFace(const BlockIterator& iter, Direction direction);
    void AddFaceToMesh(const IntVec3& localCoords, const BlockDefinition& blockDef, Direction direction);
    void AddRedstoneWireToMesh(const BlockIterator& block,std::vector<Vertex_PCU>& verts);
    void AddWireClimbingFace(std::vector<Vertex_PCU>& verts,const Vec3& basePos,Direction climbDir,const Rgba8& color,
        const Vec2& uvMin,const Vec2& uvMax);
    const int* GetFaceIndices(Direction direction);
    IntVec3 GetNeighborBlockCoords(const IntVec3& localCoords, Direction dir);
    void UpdateVBOIBO();
    bool AreAllNeighborsActive() const;

protected:
    ChunkSerializer* m_serializer;
    Block m_blocks[CHUNK_TOTAL_BLOCKS];
    World* m_world = nullptr;
    IntVec2 m_chunkCoords;
    bool m_needsSaving = false;
    bool m_needsImmediateRebuild = false;

    std::atomic<ChunkState> m_state{ChunkState::UNINITIALIZED};

    Chunk* m_northNeighbor = nullptr; 
    Chunk* m_southNeighbor = nullptr; 
    Chunk* m_eastNeighbor = nullptr;  
    Chunk* m_westNeighbor = nullptr;  

    AABB3 m_bounds;

    VertexBuffer* m_vertexBuffer = nullptr;
    IndexBuffer* m_indexBuffer = nullptr;
    std::vector<Vertex_PCUTBN> m_vertices;
    std::vector<unsigned int> m_indices;
    bool m_isDirty = true;

    VertexBuffer* m_vertexBufferDebug = nullptr;
    IndexBuffer* m_indexBufferDebug = nullptr;
    std::vector<Vertex_PCU> m_verticesDebug;
    std::vector<unsigned int> m_indicesDebug;
};

