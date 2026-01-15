#pragma once
#include "Engine/Job/JobSystem.h"
#include "Engine/Math/Vec3.hpp"

class Chunk;

class ChunkJob : public Job
{
public:
    ChunkJob(Chunk* chunk, JobType jobType) ;
    virtual void Execute() override = 0;
    virtual void OnComplete() override = 0;
public:
    Chunk* m_chunk = nullptr;
};

class GenerateChunkJob : public ChunkJob
{
public:
    GenerateChunkJob(Chunk* chunk);
    virtual void Execute() override;
    virtual void OnComplete() override;
public:
    //Chunk* m_chunk = nullptr;
};

class LoadChunkJob : public ChunkJob
{
public:
    LoadChunkJob(Chunk* chunk) ;
    virtual void Execute() override;
    virtual void OnComplete() override;
public:
    //Chunk* m_chunk = nullptr;
};

class SaveChunkJob : public ChunkJob
{
public:
    SaveChunkJob(Chunk* chunk);
    virtual void Execute() override;
    virtual void OnComplete() override;
public:
    //Chunk* m_chunk;
};

class ChunkSortTransparencyJob : public ChunkJob
{
public:
    ChunkSortTransparencyJob(Chunk* chunk, const Vec3& cameraPos);
    virtual void Execute() override;
    virtual void OnComplete() override;
private:
    Vec3 m_cameraPos;
};