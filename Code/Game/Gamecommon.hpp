#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/EngineCommon.hpp"

enum Direction : uint8_t
{
	DIRECTION_EAST,   // +X
	DIRECTION_WEST,   // -X
	DIRECTION_NORTH,  // +Y
	DIRECTION_SOUTH,  // -Y
	DIRECTION_UP,     // +Z
	DIRECTION_DOWN,   // -Z
	NUM_DIRECTIONS
};

void DebugDrawRing( Vec2 const& center, float radius, float thickness, Rgba8 const& color );
void DebugDrawLine( Vec2 const& start, Vec2 const& end, Rgba8 color, float thickness );

constexpr float PI = 3.1415926535897932384626433832795f;

constexpr int NUM_LINE_TRIS = 2;
constexpr int NUM_LINE_VERTS = 3 * NUM_LINE_TRIS;

constexpr int NUM_SIDES = 32;  
constexpr int NUM_TRIS = NUM_SIDES;  
constexpr int NUM_VERTS = 3 * NUM_TRIS; 

class App;
extern App* g_theApp;

class Game;
extern Game* m_game;

class InputSystem;

extern InputSystem* g_theInput;

class Renderer;

extern Renderer* g_theRenderer;

static constexpr int CHUNK_BITS_X = 4;  
static constexpr int CHUNK_BITS_Y = 4;  
static constexpr int CHUNK_BITS_Z = 7; 
    
static constexpr int CHUNK_SIZE_X = 1 << CHUNK_BITS_X;  // 2 4
static constexpr int CHUNK_SIZE_Y = 1 << CHUNK_BITS_Y;  // 2 4
static constexpr int CHUNK_SIZE_Z = 1 << CHUNK_BITS_Z;  // 2 7

constexpr int CHUNK_MAX_X = CHUNK_SIZE_X - 1;
constexpr int CHUNK_MAX_Y = CHUNK_SIZE_Y - 1;
constexpr int CHUNK_MAX_Z = CHUNK_SIZE_Z - 1;
    
static constexpr int CHUNK_BITS_XY = CHUNK_BITS_X + CHUNK_BITS_Y;  // 8
constexpr int CHUNK_MASK_X = CHUNK_MAX_X;
constexpr int CHUNK_MASK_Y = CHUNK_MAX_Y << CHUNK_BITS_X;
constexpr int CHUNK_MASK_Z = CHUNK_MAX_Z << (CHUNK_BITS_X + CHUNK_BITS_Y);
constexpr int BLOCKS_PER_CHUNK = CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z;
    
static constexpr int CHUNK_TOTAL_BLOCKS = 1 << (CHUNK_BITS_X + CHUNK_BITS_Y + CHUNK_BITS_Z);  // 32768

constexpr int CHUNK_ACTIVATION_RANGE = 320;
constexpr int CHUNK_DEACTIVATION_RANGE = CHUNK_ACTIVATION_RANGE + CHUNK_SIZE_X + CHUNK_SIZE_Y;

constexpr int CHUNK_ACTIVATION_RADIUS_X = 1 + (CHUNK_ACTIVATION_RANGE / CHUNK_SIZE_X);
constexpr int CHUNK_ACTIVATION_RADIUS_Y = 1 + (CHUNK_ACTIVATION_RANGE / CHUNK_SIZE_Y);
constexpr int MAX_ACTIVE_CHUNKS = (2 * CHUNK_ACTIVATION_RADIUS_X) * (2 * CHUNK_ACTIVATION_RADIUS_Y);

constexpr int MAX_CONCURRENT_JOBS = 8;

constexpr uint8_t LIGHT_MASK_OUTDOOR = 0xF0;  
constexpr uint8_t LIGHT_MASK_INDOOR  = 0x0F;  

constexpr uint8_t BLOCK_BIT_IS_SKY         = 0x01;  // 第0位
constexpr uint8_t BLOCK_BIT_IS_LIGHT_DIRTY = 0x02;  // 第1位
constexpr uint8_t BLOCK_BIT_IS_OPAQUE      = 0x04;  // 第2位
constexpr uint8_t BLOCK_BIT_IS_SOLID       = 0x08;  // 第3位
constexpr uint8_t BLOCK_BIT_IS_VISIBLE     = 0x10;  // 第4位
constexpr uint8_t BLOCK_BIT_IS_REDSTONE_DIRTY = 0x20; 

enum BlockType : uint8_t
{
	BLOCK_TYPE_AIR = 0,
	BLOCK_TYPE_WATER,
	BLOCK_TYPE_SAND,
	BLOCK_TYPE_SNOW,
	BLOCK_TYPE_ICE,
	BLOCK_TYPE_DIRT,
	BLOCK_TYPE_STONE,
	BLOCK_TYPE_COAL,
	BLOCK_TYPE_IRON,
	BLOCK_TYPE_GOLD,
	BLOCK_TYPE_DIAMOND,
	BLOCK_TYPE_OBSIDIAN,
	BLOCK_TYPE_LAVA,
	BLOCK_TYPE_GLOWSTONE,
	BLOCK_TYPE_COBBLESTONE,
	BLOCK_TYPE_CHISELED_BRICK,
	BLOCK_TYPE_GRASS,
	BLOCK_TYPE_ACACIA_LOG,
	BLOCK_TYPE_ACACIA_PLANKS,
	BLOCK_TYPE_ACACIA_LEAVES,
	BLOCK_TYPE_CACTUS_LOG,
	BLOCK_TYPE_OAK_LOG,
	BLOCK_TYPE_OAK_PLANKS,
	BLOCK_TYPE_OAK_LEAVES,
	BLOCK_TYPE_BIRCH_LOG,
	BLOCK_TYPE_BIRCH_PLANKS,
	BLOCK_TYPE_BIRCH_LEAVES,
	BLOCK_TYPE_JUNGLE_LOG,
	BLOCK_TYPE_JUNGLE_PLANKS,
	BLOCK_TYPE_JUNGLE_LEAVES,
	BLOCK_TYPE_SPRUCE_LOG,
	BLOCK_TYPE_SPRUCE_PLANKS,
	BLOCK_TYPE_SPRUCE_LEAVES,
	BLOCK_TYPE_SPRUCE_LEAVES_SNOW,

	BLOCK_TYPE_REDSTONE_WIRE_DOT = 46,
	BLOCK_TYPE_REDSTONE_WIRE_NS = 47,
	BLOCK_TYPE_REDSTONE_WIRE_EW = 48,
	BLOCK_TYPE_REDSTONE_WIRE_CORNER = 49,
	BLOCK_TYPE_REDSTONE_WIRE_CROSS = 50,
	BLOCK_TYPE_REDSTONE_TORCH = 51,
	BLOCK_TYPE_REDSTONE_TORCH_OFF = 52,
	BLOCK_TYPE_REDSTONE_BLOCK = 53,
	BLOCK_TYPE_REDSTONE_LAMP = 54,
	BLOCK_TYPE_REDSTONE_LAMP_ON = 55,
    
	BLOCK_TYPE_REPEATER_OFF = 56,
	BLOCK_TYPE_REPEATER_ON = 57,
    
	BLOCK_TYPE_LEVER = 58,
	BLOCK_TYPE_BUTTON_STONE = 59,
	BLOCK_TYPE_BUTTON_WOOD = 60,
	BLOCK_TYPE_PRESSURE_PLATE_STONE = 61,
	BLOCK_TYPE_PRESSURE_PLATE_WOOD = 62,
    
	BLOCK_TYPE_PISTON = 63,
	BLOCK_TYPE_STICKY_PISTON = 64,
	BLOCK_TYPE_PISTON_HEAD = 65,
    
	BLOCK_TYPE_OBSERVER = 66,
	BLOCK_TYPE_DROPPER = 67,
	BLOCK_TYPE_DISPENSER = 68,
	BLOCK_TYPE_HOPPER = 69,
    
	BLOCK_TYPE_TNT = 70,
	BLOCK_TYPE_NOTE_BLOCK = 71,
    NUM_BLOCK_TYPES
};

constexpr unsigned int GAME_SEED = 0u;
constexpr float DEFAULT_OCTAVE_PERSISTANCE = 0.5f;
constexpr float DEFAULT_NOISE_OCTAVE_SCALE = 2.0f;
constexpr float DEFAULT_TERRAIN_HEIGHT = 80.0f; // g_terrainReferenceHeight
constexpr float RIVER_DEPTH = 8.0f;
constexpr float TERRAIN_NOISE_SCALE = 200.0f;
constexpr unsigned int TERRAIN_NOISE_OCTAVES = 5u;
constexpr float HUMIDITY_NOISE_SCALE = 800.0f;
constexpr unsigned int HUMIDITY_NOISE_OCTAVES = 4u;
constexpr float TEMPERATURE_RAW_NOISE_SCALE = 0.0075f;
constexpr float TEMPERATURE_NOISE_SCALE = 400.0f;
constexpr unsigned int TEMPERATURE_NOISE_OCTAVES = 4u;
constexpr float HILLINESS_NOISE_SCALE = 250.0f;
constexpr unsigned int HILLINESS_NOISE_OCTAVES = 4u;
constexpr float OCEAN_START_THRESHOLD = 0.0f;
constexpr float OCEAN_END_THRESHOLD = 0.5f;
constexpr float OCEAN_DEPTH = 30.0f;
constexpr float OCEANESS_NOISE_SCALE = 600.0f;
constexpr unsigned int OCEANESS_NOISE_OCTAVES = 3u;
constexpr int MIN_DIRT_OFFSET_Z = 3;
constexpr int MAX_DIRT_OFFSET_Z = 4;
constexpr float MIN_SAND_HUMIDITY = 0.4f;
constexpr float MAX_SAND_HUMIDITY = 0.7f;
constexpr int SEA_LEVEL = 60;
constexpr float ICE_TEMPERATURE_MAX = 0.37f;
constexpr float ICE_TEMPERATURE_MIN = 0.0f;
constexpr float ICE_DEPTH_MIN = 0.0f;
constexpr float ICE_DEPTH_MAX = 8.0f;
constexpr float MIN_SAND_DEPTH_HUMIDITY = 0.4f;
constexpr float MAX_SAND_DEPTH_HUMIDITY = 0.0f;
constexpr float SAND_DEPTH_MIN = 0.0f;
constexpr float SAND_DEPTH_MAX = 6.0f;
constexpr float COAL_CHANCE = 0.05f;
constexpr float IRON_CHANCE = 0.02f;
constexpr float GOLD_CHANCE = 0.005f;
constexpr float DIAMOND_CHANCE = 0.0001f;
constexpr int OBSIDIAN_Z = 1;
constexpr int LAVA_Z = 0;

constexpr unsigned int TEMPERATURE_SEED = GAME_SEED + 100;
constexpr unsigned int HUMIDITY_SEED = GAME_SEED + 101;
constexpr unsigned int CONTINENTAL_SEED = GAME_SEED + 102;
constexpr unsigned int EROSION_SEED = GAME_SEED + 103;
constexpr unsigned int WEIRDNESS_SEED = GAME_SEED + 104;
constexpr unsigned int DENSITY_SEED = GAME_SEED + 105;
constexpr unsigned int CAVE_CHEESE_SEED = GAME_SEED + 106;
constexpr unsigned int CAVE_SPAGHETTI_SEED = GAME_SEED + 107;

static constexpr float WORLD_TO_REAL_TIME_RATIO = 200.0f;
static constexpr float TIME_ACCELERATION_MULTIPLIER = 50.0f;

struct WorldConstants
{
	float CameraWorldPosition[3];
	float Padding1;
    
	float IndoorLightColor[4];   
	float OutdoorLightColor[4];  
	float SkyColor[4];           

    
	float FogNearDistance;
	float FogFarDistance;
	float FogMaxAlpha;        // 雾的最大不透明度
	float Padding3;
};
static constexpr int k_worldConstantsSlot = 5;