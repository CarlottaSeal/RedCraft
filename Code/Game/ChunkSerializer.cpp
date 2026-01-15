#include "ChunkSerializer.h"

#include "Chunk.h"
#include "Gamecommon.hpp"
#include "Engine/Save/RLECompression.h"

ChunkSerializer::ChunkSerializer(Chunk* myChunk)
    : m_chunk(myChunk)
{
    // 分配足够空间存储所有 Block 数据 (3 bytes each)
    m_blockData = new uint8_t[CHUNK_TOTAL_BLOCKS * sizeof(Block)];
    
    // 复制 Block 数据
    memcpy(m_blockData, myChunk->m_blocks, CHUNK_TOTAL_BLOCKS * sizeof(Block));
}

ChunkSerializer::~ChunkSerializer()
{
    // 释放内存
    if (m_blockData)
    {
        delete[] m_blockData;
        m_blockData = nullptr;
    }
}

void ChunkSerializer::SaveToBinary(std::vector<uint8_t>& buffer) const
{
    if (!m_chunk || !m_chunk->m_blocks)
        return;

    size_t totalBytes = CHUNK_TOTAL_BLOCKS * sizeof(Block);
    const uint8_t* blockBytes = reinterpret_cast<const uint8_t*>(m_chunk->m_blocks);

    std::vector<uint8_t> compressed = RLECompression::CompressBytes(blockBytes, totalBytes);
    uint32_t compressedSize = static_cast<uint32_t>(compressed.size());

    ChunkFileHeader header(CHUNK_BITS_X, CHUNK_BITS_Y, CHUNK_BITS_Z);
    size_t headerPos = buffer.size();
    buffer.resize(headerPos + sizeof(ChunkFileHeader));
    memcpy(buffer.data() + headerPos, &header, sizeof(ChunkFileHeader));

    size_t sizePos = buffer.size();
    buffer.resize(sizePos + sizeof(uint32_t));
    memcpy(buffer.data() + sizePos, &compressedSize, sizeof(uint32_t));

    size_t compressedPos = buffer.size();
    buffer.resize(compressedPos + compressed.size());
    memcpy(buffer.data() + compressedPos, compressed.data(), compressed.size());
}

bool ChunkSerializer::LoadFromBinary(const std::vector<uint8_t>& buffer, size_t& offset)
{
    if (offset + sizeof(ChunkFileHeader) > buffer.size())
    {
        ERROR_RECOVERABLE("Chunk file too small for header");
        return false;
    }

    ChunkFileHeader header;
    memcpy(&header, buffer.data() + offset, sizeof(ChunkFileHeader));

    if (!header.Validate(CHUNK_BITS_X, CHUNK_BITS_Y, CHUNK_BITS_Z))
    {
        ERROR_RECOVERABLE("Invalid chunk header");
        return false;
    }

    offset += sizeof(ChunkFileHeader);

    // 2. 读取紧跟在 header 后面的 compressedSize（uint32_t）
    if (offset + sizeof(uint32_t) > buffer.size())
    {
        ERROR_RECOVERABLE("Chunk file too small for compressed size");
        return false;
    }

    uint32_t compressedSize = 0;
    memcpy(&compressedSize, buffer.data() + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    // 3. 验证压缩数据长度
    //    - 不能为 0
    //    - 必须是偶数（RLE: [value][count] 成对存储）
    //    - 不能超过 buffer 剩余空间
    if (compressedSize == 0 || (compressedSize % 2) != 0)
    {
        ERROR_RECOVERABLE("Invalid RLE data size");
        return false;
    }

    if (offset + compressedSize > buffer.size())
    {
        ERROR_RECOVERABLE("Compressed data exceeds buffer size");
        return false;
    }

    // 4. 确保 m_blockData 已分配
    if (!m_blockData)
    {
        m_blockData = new uint8_t[CHUNK_TOTAL_BLOCKS * sizeof(Block)];
    }

    // 5. 解压缩数据
    size_t totalBytes = CHUNK_TOTAL_BLOCKS * sizeof(Block);
    bool success = RLECompression::DecompressBytes(
        buffer.data() + offset,           // 压缩数据起始地址
        static_cast<size_t>(compressedSize), // 压缩数据字节数
        m_blockData,                      // 输出缓冲区
        totalBytes                        // 期望解压后的字节数
    );

    if (!success)
    {
        ERROR_RECOVERABLE("RLE decompression failed");
        return false;
    }

    // 6. 复制解压后的数据到 chunk->m_blocks
    memcpy(m_chunk->m_blocks, m_blockData, totalBytes);

    // 7. 前进 offset，跳过刚刚消费掉的压缩数据
    offset += compressedSize;
    return true;
}

std::string ChunkSerializer::GetSaveIdentifier() const
{
    return "GCHK";
}