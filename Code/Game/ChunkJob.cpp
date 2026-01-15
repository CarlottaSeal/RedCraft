#include "ChunkJob.h"

#include "Chunk.h"
#include "World.h"

ChunkJob::ChunkJob(Chunk* chunk, JobType jobType)
    : Job(jobType)
    , m_chunk(chunk)
{
}

GenerateChunkJob::GenerateChunkJob(Chunk* chunk)
    : ChunkJob(chunk, JOB_TYPE_WORKER)
{
}

void GenerateChunkJob::Execute()
{
    m_chunk->SetState(ChunkState::GENERATING);
    m_chunk->GenerateBlocks();
    m_chunk->SetState(ChunkState::GENERATION_COMPLETE);
}

void GenerateChunkJob::OnComplete()
{
    if (m_chunk->GetState() == ChunkState::GENERATION_COMPLETE)
    {
        m_chunk->m_world->ActivateProcessedChunk(m_chunk);
    }
}

LoadChunkJob::LoadChunkJob(Chunk* chunk)
    : ChunkJob(chunk, JOB_TYPE_IO)
{
}

void LoadChunkJob::Execute()
{
    m_chunk->SetState(ChunkState::LOADING);
    bool success = m_chunk->Load();
    if (!success)
    {
        m_chunk->GenerateBlocks();
    }
    m_chunk->SetState(ChunkState::GENERATION_COMPLETE);
}

void LoadChunkJob::OnComplete()
{
    if (m_chunk->GetState() == ChunkState::GENERATION_COMPLETE)
    {
        m_chunk->m_world->ActivateProcessedChunk(m_chunk);
    }
}

SaveChunkJob::SaveChunkJob(Chunk* chunk)
    : ChunkJob(chunk, JOB_TYPE_IO)
{
}

void SaveChunkJob::Execute()
{
    m_chunk->SetState(ChunkState::SAVING);
    m_chunk->Save();
}

void SaveChunkJob::OnComplete()
{
    if (m_chunk->GetState() == ChunkState::GENERATION_COMPLETE)
    {
        delete m_chunk; 
    }
}

ChunkSortTransparencyJob::ChunkSortTransparencyJob(Chunk* chunk, const Vec3& cameraPos)
    : ChunkJob(chunk, JOB_TYPE_WORKER)
    , m_cameraPos(cameraPos)
{
}

void ChunkSortTransparencyJob::Execute()
{
    if (m_chunk)
    {
        m_chunk->PrepareSortedIndices(m_cameraPos);
    }
}

void ChunkSortTransparencyJob::OnComplete()
{
}