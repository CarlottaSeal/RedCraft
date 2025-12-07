#include "BlockDefinition.h"

#include "Game.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"

std::vector<BlockDefinition> BlockDefinition::s_blockDefs;
BlockDefinition const* BlockDefinition::s_blockDefsByType[NUM_BLOCK_TYPES] = { nullptr };

extern Game* g_theGame;
extern Renderer* g_theRenderer;

BlockDefinition::BlockDefinition(XmlElement const& blockDefElement)
{
	s_blockDefs.reserve(NUM_BLOCK_TYPES);   
    m_name = ParseXmlAttribute(blockDefElement, "name", m_name);

    m_isVisible = ParseXmlAttribute(blockDefElement, "isVisible", m_isVisible);
    m_isSolid = ParseXmlAttribute(blockDefElement, "isSolid", m_isSolid);
    m_isOpaque = ParseXmlAttribute(blockDefElement, "isOpaque", m_isOpaque);
    m_isTransparent = ParseXmlAttribute(blockDefElement, "isTransparent", m_isTransparent);

    m_topSpriteCoords = ParseXmlAttribute(blockDefElement, "topSpriteCoords", IntVec2());
    m_bottomSpriteCoords = ParseXmlAttribute(blockDefElement, "bottomSpriteCoords", IntVec2());
    m_sideSpriteCoords = ParseXmlAttribute(blockDefElement, "sideSpriteCoords", IntVec2());

    m_indoorLightInfluence = (uint8_t)ParseXmlAttribute(blockDefElement, "indoorLighting", 0);
    m_outdoorLightInfluence = (uint8_t)ParseXmlAttribute(blockDefElement, "outdoorLighting", 0);

    if (!m_isVisible)
        return;
    //std::vector<Vertex_PCUTBN> verts;
    AABB3 bounds = AABB3(Vec3(), Vec3(1.f,1.f,1.f));
	const Vec3 mn = bounds.m_mins;
	const Vec3 mx = bounds.m_maxs;

	// 8 corners
	Vec3 nnn(mn.x, mn.y, mn.z); // (0,0,0)
	Vec3 xnn(mx.x, mn.y, mn.z); // (1,0,0)
	Vec3 nxn(mn.x, mx.y, mn.z); // (0,1,0)
	Vec3 xxn(mx.x, mx.y, mn.z); // (1,1,0)
	Vec3 nnx(mn.x, mn.y, mx.z); // (0,0,1)
	Vec3 xnx(mx.x, mn.y, mx.z); // (1,0,1)
	Vec3 nxx(mn.x, mx.y, mx.z); // (0,1,1)
	Vec3 xxx(mx.x, mx.y, mx.z); // (1,1,1)
	auto sideUV = g_theGame->m_spriteSheet->GetSpriteUVs(m_sideSpriteCoords);
	auto topUV = g_theGame->m_spriteSheet->GetSpriteUVs(m_topSpriteCoords);
	auto bottomUV = g_theGame->m_spriteSheet->GetSpriteUVs(m_bottomSpriteCoords);

    //South (-Y)
	AddVertsForIndexQuad3D(m_verts, m_indexes, nnn, xnn, xnx, nnx, Rgba8(200, 200, 200), sideUV);

	// East   (+X): plane x = mx.x, normal (1,0,0)
	AddVertsForIndexQuad3D(m_verts, m_indexes, xnn, xxn, xxx, xnx, Rgba8(230,230,230), sideUV);

	// North  (+Y): plane y = mx.y, normal (0,1,0)
	AddVertsForIndexQuad3D(m_verts, m_indexes, xxn, nxn, nxx, xxx, Rgba8(200,200,200), sideUV);

	// West   (-X): plane x = mn.x, normal (-1,0,0)
	AddVertsForIndexQuad3D(m_verts, m_indexes, nxn, nnn, nnx, nxx, Rgba8(230, 230, 230), sideUV);

	// Top    (+Z): plane z = mx.z, normal (0,0,1)
	AddVertsForIndexQuad3D(m_verts, m_indexes, nnx, xnx, xxx, nxx, Rgba8::WHITE, topUV);

	// Bottom (-Z): plane z = mn.z, normal (0,0,-1)
	AddVertsForIndexQuad3D(m_verts, m_indexes, nnn, nxn, xxn, xnn, Rgba8::WHITE, bottomUV);

    //m_vertexBuffer = g_theRenderer->CreateVertexBuffer((unsigned int)verts.size() * sizeof(Vertex_PCUTBN), sizeof(Vertex_PCUTBN));
    //m_indexBuffer = g_theRenderer->CreateIndexBuffer((unsigned int)m_indexes.size()* sizeof(unsigned int), sizeof(unsigned int));

    //g_theRenderer->CopyCPUToGPU(verts.data(), (unsigned int)(verts.size() * sizeof(Vertex_PCUTBN)), m_vertexBuffer);
    //g_theRenderer->CopyCPUToGPU(m_indexes.data(), (unsigned int)(m_indexes.size() * sizeof(unsigned int)), m_indexBuffer);
}

void BlockDefinition::InitializeBlockDefs()
{
    XmlDocument blockDefDoc;
    XmlResult weaponLoadResult = blockDefDoc.LoadFile("Data/Definitions/BlockSpriteSheet_BlockDefinitions.xml");
    if (weaponLoadResult != XmlResult::XML_SUCCESS)
    {
        ERROR_AND_DIE(Stringf("Cannot load block definitions!"));
    }

    XmlElement* blockRootElement = blockDefDoc.RootElement();
    XmlElement* blockFirstElement = blockRootElement->FirstChildElement();

    // ✅ 先加载所有定义到vector
    while (blockFirstElement)
    {
        BlockDefinition blockDef(*blockFirstElement);
        s_blockDefs.push_back(blockDef);
        blockFirstElement = blockFirstElement->NextSiblingElement();
    }

    // ✅ 然后建立指针数组映射
    for (BlockDefinition& def : s_blockDefs)
    {
        if (def.m_name == "Air") s_blockDefsByType[BLOCK_TYPE_AIR] = &def;
        else if (def.m_name == "Water") s_blockDefsByType[BLOCK_TYPE_WATER] = &def;
        else if (def.m_name == "Sand") s_blockDefsByType[BLOCK_TYPE_SAND] = &def;
        else if (def.m_name == "Snow") s_blockDefsByType[BLOCK_TYPE_SNOW] = &def;
        else if (def.m_name == "Ice") s_blockDefsByType[BLOCK_TYPE_ICE] = &def;
        else if (def.m_name == "Dirt") s_blockDefsByType[BLOCK_TYPE_DIRT] = &def;
        else if (def.m_name == "Stone") s_blockDefsByType[BLOCK_TYPE_STONE] = &def;
        else if (def.m_name == "Coal") s_blockDefsByType[BLOCK_TYPE_COAL] = &def;
        else if (def.m_name == "Iron") s_blockDefsByType[BLOCK_TYPE_IRON] = &def;
        else if (def.m_name == "Gold") s_blockDefsByType[BLOCK_TYPE_GOLD] = &def;
        else if (def.m_name == "Diamond") s_blockDefsByType[BLOCK_TYPE_DIAMOND] = &def;
        else if (def.m_name == "Obsidian") s_blockDefsByType[BLOCK_TYPE_OBSIDIAN] = &def;
        else if (def.m_name == "Lava") s_blockDefsByType[BLOCK_TYPE_LAVA] = &def;
        else if (def.m_name == "Glowstone") s_blockDefsByType[BLOCK_TYPE_GLOWSTONE] = &def;
        else if (def.m_name == "Cobblestone") s_blockDefsByType[BLOCK_TYPE_COBBLESTONE] = &def;
        else if (def.m_name == "ChiseledBrick") s_blockDefsByType[BLOCK_TYPE_CHISELED_BRICK] = &def;
        else if (def.m_name == "Grass") s_blockDefsByType[BLOCK_TYPE_GRASS] = &def;
        else if (def.m_name == "AcaciaLog") s_blockDefsByType[BLOCK_TYPE_ACACIA_LOG] = &def;
        else if (def.m_name == "AcaciaPlanks") s_blockDefsByType[BLOCK_TYPE_ACACIA_PLANKS] = &def;
        else if (def.m_name == "AcaciaLeaves") s_blockDefsByType[BLOCK_TYPE_ACACIA_LEAVES] = &def;
        else if (def.m_name == "CactusLog") s_blockDefsByType[BLOCK_TYPE_CACTUS_LOG] = &def;
        else if (def.m_name == "OakLog") s_blockDefsByType[BLOCK_TYPE_OAK_LOG] = &def;
        else if (def.m_name == "OakPlanks") s_blockDefsByType[BLOCK_TYPE_OAK_PLANKS] = &def;
        else if (def.m_name == "OakLeaves") s_blockDefsByType[BLOCK_TYPE_OAK_LEAVES] = &def;
        else if (def.m_name == "BirchLog") s_blockDefsByType[BLOCK_TYPE_BIRCH_LOG] = &def;
        else if (def.m_name == "BirchPlanks") s_blockDefsByType[BLOCK_TYPE_BIRCH_PLANKS] = &def;
        else if (def.m_name == "BirchLeaves") s_blockDefsByType[BLOCK_TYPE_BIRCH_LEAVES] = &def;
        else if (def.m_name == "JungleLog") s_blockDefsByType[BLOCK_TYPE_JUNGLE_LOG] = &def;
        else if (def.m_name == "JunglePlanks") s_blockDefsByType[BLOCK_TYPE_JUNGLE_PLANKS] = &def;
        else if (def.m_name == "JungleLeaves") s_blockDefsByType[BLOCK_TYPE_JUNGLE_LEAVES] = &def;
        else if (def.m_name == "SpruceLog") s_blockDefsByType[BLOCK_TYPE_SPRUCE_LOG] = &def;
        else if (def.m_name == "SprucePlanks") s_blockDefsByType[BLOCK_TYPE_SPRUCE_PLANKS] = &def;
        else if (def.m_name == "SpruceLeaves") s_blockDefsByType[BLOCK_TYPE_SPRUCE_LEAVES] = &def;
        else if (def.m_name == "SpruceLeavesSnow") s_blockDefsByType[BLOCK_TYPE_SPRUCE_LEAVES_SNOW] = &def;
    
else if (def.m_name == "RedstoneWireDot")    s_blockDefsByType[BLOCK_TYPE_REDSTONE_WIRE_DOT] = &def;
        else if (def.m_name == "RedstoneWireNS")     s_blockDefsByType[BLOCK_TYPE_REDSTONE_WIRE_NS] = &def;
        else if (def.m_name == "RedstoneWireEW")     s_blockDefsByType[BLOCK_TYPE_REDSTONE_WIRE_EW] = &def;
        else if (def.m_name == "RedstoneWireCorner") s_blockDefsByType[BLOCK_TYPE_REDSTONE_WIRE_CORNER] = &def;
        else if (def.m_name == "RedstoneWireCross")  s_blockDefsByType[BLOCK_TYPE_REDSTONE_WIRE_CROSS] = &def;

        // --- Redstone 火把 / 方块 / 灯 ---
        else if (def.m_name == "RedstoneTorch")      s_blockDefsByType[BLOCK_TYPE_REDSTONE_TORCH] = &def;
        else if (def.m_name == "RedstoneTorchOff")   s_blockDefsByType[BLOCK_TYPE_REDSTONE_TORCH_OFF] = &def;
        else if (def.m_name == "RedstoneBlock")      s_blockDefsByType[BLOCK_TYPE_REDSTONE_BLOCK] = &def;

        else if (def.m_name == "RedstoneLamp")       s_blockDefsByType[BLOCK_TYPE_REDSTONE_LAMP] = &def;
        else if (def.m_name == "RedstoneLampOn")     s_blockDefsByType[BLOCK_TYPE_REDSTONE_LAMP_ON] = &def;

        // --- Redstone 逻辑元件 ---
        else if (def.m_name == "RepeaterOff")        s_blockDefsByType[BLOCK_TYPE_REPEATER_OFF] = &def;
        else if (def.m_name == "RepeaterOn")         s_blockDefsByType[BLOCK_TYPE_REPEATER_ON] = &def;

        else if (def.m_name == "Lever")              s_blockDefsByType[BLOCK_TYPE_LEVER] = &def;
        else if (def.m_name == "StoneButton")        s_blockDefsByType[BLOCK_TYPE_BUTTON_STONE] = &def;

        else if (def.m_name == "Piston")             s_blockDefsByType[BLOCK_TYPE_PISTON] = &def;
        else if (def.m_name == "StickyPiston")       s_blockDefsByType[BLOCK_TYPE_STICKY_PISTON] = &def;
        else if (def.m_name == "PistonHead")         s_blockDefsByType[BLOCK_TYPE_PISTON_HEAD] = &def;

        else if (def.m_name == "Observer")           s_blockDefsByType[BLOCK_TYPE_OBSERVER] = &def;
        //else if (def.m_name == "NoteBlock")          s_blockDefsByType[BLOCK_TYPE_NOTE_BLOCK] = &def;

        else if (def.m_name == "Farmland")           s_blockDefsByType[BLOCK_TYPE_FARMLAND] = &def;
        else if (def.m_name == "WetFarmland")           s_blockDefsByType[BLOCK_TYPE_FARMLAND_WET] = &def;

        else if (def.m_name == "Wheat0")             s_blockDefsByType[BLOCK_TYPE_WHEAT_0] = &def;
        else if (def.m_name == "Wheat1")             s_blockDefsByType[BLOCK_TYPE_WHEAT_1] = &def;
        else if (def.m_name == "Wheat2")             s_blockDefsByType[BLOCK_TYPE_WHEAT_2] = &def;
        else if (def.m_name == "Wheat3")             s_blockDefsByType[BLOCK_TYPE_WHEAT_3] = &def;
        else if (def.m_name == "Wheat4")             s_blockDefsByType[BLOCK_TYPE_WHEAT_4] = &def;
        else if (def.m_name == "Wheat5")             s_blockDefsByType[BLOCK_TYPE_WHEAT_5] = &def;
        else if (def.m_name == "Wheat6")             s_blockDefsByType[BLOCK_TYPE_WHEAT_6] = &def;
        else if (def.m_name == "Wheat7")             s_blockDefsByType[BLOCK_TYPE_WHEAT_7] = &def;

        else if (def.m_name == "Carrots0")           s_blockDefsByType[BLOCK_TYPE_CARROTS_0] = &def;
        else if (def.m_name == "Carrots1")           s_blockDefsByType[BLOCK_TYPE_CARROTS_1] = &def;
        else if (def.m_name == "Carrots2")           s_blockDefsByType[BLOCK_TYPE_CARROTS_2] = &def;
        else if (def.m_name == "Carrots3")           s_blockDefsByType[BLOCK_TYPE_CARROTS_3] = &def;

        else if (def.m_name == "Beetroots0")         s_blockDefsByType[BLOCK_TYPE_BEETROOTS_0] = &def;
        else if (def.m_name == "Beetroots1")         s_blockDefsByType[BLOCK_TYPE_BEETROOTS_1] = &def;
        else if (def.m_name == "Beetroots2")         s_blockDefsByType[BLOCK_TYPE_BEETROOTS_2] = &def;
        else if (def.m_name == "Beetroots3")         s_blockDefsByType[BLOCK_TYPE_BEETROOTS_3] = &def;

        else if (def.m_name == "Potatoes0")          s_blockDefsByType[BLOCK_TYPE_POTATOES_0] = &def;
        else if (def.m_name == "Potatoes1")          s_blockDefsByType[BLOCK_TYPE_POTATOES_1] = &def;
        else if (def.m_name == "Potatoes2")          s_blockDefsByType[BLOCK_TYPE_POTATOES_2] = &def;
        else if (def.m_name == "Potatoes3")          s_blockDefsByType[BLOCK_TYPE_POTATOES_3] = &def;

        else if (def.m_name == "PumpkinStem0") s_blockDefsByType[BLOCK_TYPE_PUMPKIN_STEM_0] = &def;
        else if (def.m_name == "PumpkinStem1") s_blockDefsByType[BLOCK_TYPE_PUMPKIN_STEM_1] = &def;
        else if (def.m_name == "PumpkinStem2") s_blockDefsByType[BLOCK_TYPE_PUMPKIN_STEM_2] = &def;
        else if (def.m_name == "PumpkinStem3") s_blockDefsByType[BLOCK_TYPE_PUMPKIN_STEM_3] = &def;
        else if (def.m_name == "PumpkinStem4") s_blockDefsByType[BLOCK_TYPE_PUMPKIN_STEM_4] = &def;
        else if (def.m_name == "PumpkinStem5") s_blockDefsByType[BLOCK_TYPE_PUMPKIN_STEM_5] = &def;
        else if (def.m_name == "PumpkinStem6") s_blockDefsByType[BLOCK_TYPE_PUMPKIN_STEM_6] = &def;
        else if (def.m_name == "PumpkinStem7") s_blockDefsByType[BLOCK_TYPE_PUMPKIN_STEM_7] = &def;

        else if (def.m_name == "MelonStem0")   s_blockDefsByType[BLOCK_TYPE_MELON_STEM_0] = &def;
        else if (def.m_name == "MelonStem1")   s_blockDefsByType[BLOCK_TYPE_MELON_STEM_1] = &def;
        else if (def.m_name == "MelonStem2")   s_blockDefsByType[BLOCK_TYPE_MELON_STEM_2] = &def;
        else if (def.m_name == "MelonStem3")   s_blockDefsByType[BLOCK_TYPE_MELON_STEM_3] = &def;
        else if (def.m_name == "MelonStem4")   s_blockDefsByType[BLOCK_TYPE_MELON_STEM_4] = &def;
        else if (def.m_name == "MelonStem5")   s_blockDefsByType[BLOCK_TYPE_MELON_STEM_5] = &def;
        else if (def.m_name == "MelonStem6")   s_blockDefsByType[BLOCK_TYPE_MELON_STEM_6] = &def;
        else if (def.m_name == "MelonStem7")   s_blockDefsByType[BLOCK_TYPE_MELON_STEM_7] = &def;
    	
        else if (def.m_name == "Pumpkin") s_blockDefsByType[BLOCK_TYPE_PUMPKIN] = &def;
        else if (def.m_name == "Melon")   s_blockDefsByType[BLOCK_TYPE_MELON]   = &def;

        // --- Sugar cane / 水草 / 珊瑚 / 海绵 ---
        else if (def.m_name == "SugarCane")          s_blockDefsByType[BLOCK_TYPE_SUGAR_CANE] = &def;

        else if (def.m_name == "Kelp")               s_blockDefsByType[BLOCK_TYPE_KELP] = &def;
        else if (def.m_name == "KelpTop")            s_blockDefsByType[BLOCK_TYPE_KELP_TOP] = &def;

        else if (def.m_name == "Seagrass")           s_blockDefsByType[BLOCK_TYPE_SEAGRASS] = &def;
        else if (def.m_name == "TallSeagrassBottom") s_blockDefsByType[BLOCK_TYPE_TALL_SEAGRASS_BOTTOM] = &def;
        else if (def.m_name == "TallSeagrassTop")    s_blockDefsByType[BLOCK_TYPE_TALL_SEAGRASS_TOP] = &def;

        else if (def.m_name == "CoralBlock")         s_blockDefsByType[BLOCK_TYPE_CORAL_BLOCK_RED] = &def;
        else if (def.m_name == "CoralPurple")         s_blockDefsByType[BLOCK_TYPE_CORAL_BLOCK_PURPLE] = &def;
        else if (def.m_name == "CoralYellow")           s_blockDefsByType[BLOCK_TYPE_CORAL_BLOCK_YELLOW] = &def;
        else if (def.m_name == "DeadCoralBlock")     s_blockDefsByType[BLOCK_TYPE_CORAL_BLOCK_DEAD] = &def;
        else if (def.m_name == "DeadCoralFan")       s_blockDefsByType[BLOCK_TYPE_CORAL_BLOCK_FAN_DEAD] = &def;

        else if (def.m_name == "Sponge")             s_blockDefsByType[BLOCK_TYPE_SPONGE] = &def;
        else if (def.m_name == "WetSponge")          s_blockDefsByType[BLOCK_TYPE_WET_SPONGE] = &def;

        // else if (def.m_name == "SeaLantern")         s_blockDefsByType[BLOCK_TYPE_SEA_LANTERN] = &def;
        // else if (def.m_name == "Campfire")           s_blockDefsByType[BLOCK_TYPE_CAMPFIRE] = &def;

    	// Nether wart growth stages
        else if (def.m_name == "NetherWart0") s_blockDefsByType[BLOCK_TYPE_NETHER_WART_0] = &def;
        else if (def.m_name == "NetherWart1") s_blockDefsByType[BLOCK_TYPE_NETHER_WART_1] = &def;
        else if (def.m_name == "NetherWart2") s_blockDefsByType[BLOCK_TYPE_NETHER_WART_2] = &def;
        else if (def.m_name == "NetherWart3") s_blockDefsByType[BLOCK_TYPE_NETHER_WART_3] = &def;
        else if (def.m_name == "SoulSand") s_blockDefsByType[BLOCK_TYPE_SOUL_SAND] = &def;

    	// Fish / Turtle eggs
        else if (def.m_name == "FishEgg")         s_blockDefsByType[BLOCK_TYPE_FISH_EGG] = &def;
        else if (def.m_name == "FishEggHatching") s_blockDefsByType[BLOCK_TYPE_FISH_EGG_HATCHING] = &def;

    }
}

void BlockDefinition::ClearDefinitions()
{
    // for (BlockDefinition blockDef : s_blockDefs)
    // {
    //     delete blockDef.m_vertexBuffer;
    //     blockDef.m_vertexBuffer = nullptr;
    //     delete blockDef.m_indexBuffer;
    //     blockDef.m_indexBuffer = nullptr;
    // }
    s_blockDefs.clear();
    for (int i = 0; i < NUM_BLOCK_TYPES; i++)
    {
        s_blockDefsByType[i] = nullptr;
    }
}

BlockDefinition const& BlockDefinition::GetBlockDef(std::string const& blockDefName)
{
	for (int i = 0; i < NUM_BLOCK_TYPES; i++)
	{
		if (s_blockDefs[i].m_name == blockDefName)
		{
			return s_blockDefs[i];
		}
	}
	ERROR_AND_DIE(Stringf("Unknown BlockDef \"%s\"!", blockDefName.c_str()))
}

BlockDefinition const& BlockDefinition::GetBlockDef(uint8_t const& blockUint)
{
    return *s_blockDefsByType[blockUint];
}

