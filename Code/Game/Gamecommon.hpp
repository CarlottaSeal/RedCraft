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
	
	BLOCK_TYPE_REDSTONE_WIRE_DOT = 34,
	BLOCK_TYPE_REDSTONE_WIRE_NS = 35,
	BLOCK_TYPE_REDSTONE_WIRE_EW = 36,
	BLOCK_TYPE_REDSTONE_WIRE_CORNER = 37,
	BLOCK_TYPE_REDSTONE_WIRE_CROSS = 38,
	
	BLOCK_TYPE_REDSTONE_TORCH = 39,
	BLOCK_TYPE_REDSTONE_TORCH_OFF = 40,
	BLOCK_TYPE_REDSTONE_BLOCK = 41,
	
	BLOCK_TYPE_REDSTONE_LAMP = 42,
	BLOCK_TYPE_REDSTONE_LAMP_ON = 43,
	
	BLOCK_TYPE_REPEATER_OFF = 44,
	BLOCK_TYPE_REPEATER_ON = 45,
	
	BLOCK_TYPE_LEVER = 46,
	BLOCK_TYPE_BUTTON_STONE = 47,
	
	BLOCK_TYPE_PISTON = 48,
	BLOCK_TYPE_STICKY_PISTON = 49,
	BLOCK_TYPE_PISTON_HEAD = 50,
	
	BLOCK_TYPE_OBSERVER = 51,
	
	// 农田
	BLOCK_TYPE_FARMLAND = 52,
	BLOCK_TYPE_FARMLAND_WET = 53,
	// 小麦（8个阶段）
	BLOCK_TYPE_WHEAT_0 = 54,
	BLOCK_TYPE_WHEAT_1 = 55,
	BLOCK_TYPE_WHEAT_2 = 56,
	BLOCK_TYPE_WHEAT_3 = 57,
	BLOCK_TYPE_WHEAT_4 = 58,
	BLOCK_TYPE_WHEAT_5 = 59,
	BLOCK_TYPE_WHEAT_6 = 60,
	BLOCK_TYPE_WHEAT_7 = 61,  // 成熟
	// 胡萝卜（4个阶段）
	BLOCK_TYPE_CARROTS_0 = 62,
	BLOCK_TYPE_CARROTS_1 = 63,
	BLOCK_TYPE_CARROTS_2 = 64,
	BLOCK_TYPE_CARROTS_3 = 65,  // 成熟
	// 土豆（4个阶段）
	BLOCK_TYPE_POTATOES_0 = 66,
	BLOCK_TYPE_POTATOES_1 = 67,
	BLOCK_TYPE_POTATOES_2 = 68,
	BLOCK_TYPE_POTATOES_3 = 69,  // 成熟
	// 甜菜根（4个阶段）
	BLOCK_TYPE_BEETROOTS_0 = 70,
	BLOCK_TYPE_BEETROOTS_1 = 71,
	BLOCK_TYPE_BEETROOTS_2 = 72,
	BLOCK_TYPE_BEETROOTS_3 = 73,  // 成熟
	// 甘蔗（向上生长）
	BLOCK_TYPE_SUGAR_CANE = 74,
	// 仙人掌（向上生长）- 注意：BLOCK_TYPE_CACTUS_LOG已在20号使用
	BLOCK_TYPE_CACTUS = 75,
	// 南瓜茎（8个阶段）
	BLOCK_TYPE_PUMPKIN_STEM_0 = 76,
	BLOCK_TYPE_PUMPKIN_STEM_1 = 77,
	BLOCK_TYPE_PUMPKIN_STEM_2 = 78,
	BLOCK_TYPE_PUMPKIN_STEM_3 = 79,
	BLOCK_TYPE_PUMPKIN_STEM_4 = 80,
	BLOCK_TYPE_PUMPKIN_STEM_5 = 81,
	BLOCK_TYPE_PUMPKIN_STEM_6 = 82,
	BLOCK_TYPE_PUMPKIN_STEM_7 = 83,  // 成熟
	BLOCK_TYPE_PUMPKIN = 84,          // 南瓜果实
	// 西瓜茎（8个阶段）
	BLOCK_TYPE_MELON_STEM_0 = 85,
	BLOCK_TYPE_MELON_STEM_1 = 86,
	BLOCK_TYPE_MELON_STEM_2 = 87,
	BLOCK_TYPE_MELON_STEM_3 = 88,
	BLOCK_TYPE_MELON_STEM_4 = 89,
	BLOCK_TYPE_MELON_STEM_5 = 90,
	BLOCK_TYPE_MELON_STEM_6 = 91,
	BLOCK_TYPE_MELON_STEM_7 = 92,  // 成熟
	BLOCK_TYPE_MELON = 93,          // 西瓜果实
	// 地狱疣（4个阶段）- 需要灵魂沙
	BLOCK_TYPE_SOUL_SAND = 94,
	BLOCK_TYPE_NETHER_WART_0 = 95,
	BLOCK_TYPE_NETHER_WART_1 = 96,
	BLOCK_TYPE_NETHER_WART_2 = 97,
	BLOCK_TYPE_NETHER_WART_3 = 98,  // 成熟
	
	// ========== 水产养殖系统 (99-112) ==========
	// 鱼卵（3个阶段）
	BLOCK_TYPE_FISH_EGG = 99,          // 刚放置
	BLOCK_TYPE_FISH_EGG_HATCHING = 100, // 即将孵化
	// 孵化后变成空气，生成鱼实体
	// 海带（水生植物）
	BLOCK_TYPE_KELP = 101,
	BLOCK_TYPE_KELP_TOP = 102,
	// 海草（装饰性水生植物）
	BLOCK_TYPE_SEAGRASS = 103,
	BLOCK_TYPE_TALL_SEAGRASS_BOTTOM = 104,
	BLOCK_TYPE_TALL_SEAGRASS_TOP = 105,
	// 珊瑚（装饰性，可选）
	BLOCK_TYPE_CORAL_BLOCK_RED = 106,
	BLOCK_TYPE_CORAL_BLOCK_PURPLE = 107,
	BLOCK_TYPE_CORAL_BLOCK_DEAD = 108,
	BLOCK_TYPE_CORAL_BLOCK_YELLOW = 109,
	BLOCK_TYPE_CORAL_BLOCK_FAN_DEAD = 110,
	// 海绵（吸水用，可选）
	BLOCK_TYPE_SPONGE = 111,
	BLOCK_TYPE_WET_SPONGE = 112,
	
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