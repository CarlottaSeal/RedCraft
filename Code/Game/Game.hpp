#pragma once
#include "Game/Gamecommon.hpp"
#include "Game/App.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Audio/AudioSystem.hpp"

#include <vector>

#include "Engine/Renderer/SpriteSheet.hpp"
#include "Generator/WorldGenPipeline.h"

class WorldGenPipeline;
class IconAtlas;
class GameUIManager;
class World;
class Player;
class Clock;
class Entity;

class Game 
{
public:
	Game();
	~Game();

	void Startup();
	void ForceShutdownCurrentWorld();

	void Update(float deltaSeconds);
	void Render() const;

	void AdjustForPauseAndTimeDistortion();
	void CleanupOldWorlds();

public:
	bool m_openDevConsole = false;
	bool m_isInAttractMode;
	Clock* m_gameClock;
	Camera m_screenCamera;

	Player* m_player;

	SpriteSheet* m_spriteSheet; //common spritesheet
	//IconAtlas* m_uiAtlas = nullptr;
	
	//Debug
	int m_numOfVerts;
	int m_numOfIndices;

	std::vector<World*> m_worldsToDelete;

private:
	void AttractModeUpdate();
	void AttractStateRender() const;

	void GameStateUpdate(float deltaSeconds);
	void GameStateRender() const;
	void GameAxisDebugRender() const;
	void ImGuiUpdate();
	void ImGuiDrawCurveEditor(const char* label, std::vector<Vec2>& points);

	void PrintGameControlToDevConsole();
	void DebugRenderSystemInputUpdate();
	void DebugAddWorldAxisText(Mat44 worldMat);
	void DebugTestSaveSystemPaths();

public:
	WorldGenPipeline::WorldGenMode m_selectedWorldType = WorldGenPipeline::WorldGenMode::NORMAL;
	World* m_infiniteWorld = nullptr;  // 存储无限世界实例
	World* m_farmWorld = nullptr;
	
	float g_densityNoiseScale = 128.0f;
	int g_densityNoiseOctaves = 8;
	float g_terrainHeight = 64.0f;
	float g_terrainReferenceHeight = 80.0f;
	float g_biasPerZ = 0.016f;
	// Continent
	float g_continentNoiseScale = 1024.0f;
	int g_continentNoiseOctaves = 4;
	std::vector<Vec2> g_heightOffsetCurvePoints =
	{Vec2(-1.f,-0.6f), Vec2(-0.4f,-0.4f), Vec2(0.4f,0.4f), Vec2(1.f,0.6f)};
	std::vector<Vec2> g_heightScaleCurvePoints =
	{Vec2(-1.f,0.f), Vec2(-0.5f,0.f), Vec2(-0.25f,2.f), Vec2(0.25f,2.f), Vec2(1.f,-1.5f)};
	float g_curveGridSize = 0.10f;
	float g_curveXMin = -1.00f;
	float g_curveXMax = 1.00f;
	float g_curveYMin = -1.00f;
	float g_curveYMax = 1.00f;
	// Erosion
	float g_erosionNoiseScale = 512.0f;
	int g_erosionNoiseOctaves = 8;
	// Peaks and Valleys
	float g_peaksValleysNoiseScale = 512.0f;
	int g_peaksValleysNoiseOctaves = 8;
	// Temperature
	float g_temperatureNoiseScale = 512.0f;
	int g_temperatureNoiseOctaves = 2;
	// Humidity
	float g_humidityNoiseScale = 512.0f;
	int g_humidityNoiseOctaves = 4;
	// Other Settings
	bool g_seaEnabled = true;
	int g_seaLevel = 64;
	bool g_densityNoiseEnabled = true;
	bool g_densityNoiseBiasEnabled = true;
	bool g_continentHeightOffsetEnabled = true;
	bool g_continentHeightScaleEnabled = true;
	bool g_blockReplacementEnabled = true;
	bool g_caveCarvingEnabled = true;
	bool g_treeGenerationEnabled = true;

	float g_caveGeneratingThreshold = 0.2f;
	// Debug
	int g_debugVisualizationMode = 0;
	bool g_showChunkBounds = true; 

	World* m_currentWorld;
	
private:
	float m_maxFPS;
	
	bool m_isSlowMo;
	bool m_isUsingUserTimeScale;

	float m_userTimeScale;

	bool m_hasPlayedAttractSound = false;
	SoundPlaybackID m_attractSoundID = MISSING_SOUND_ID;

	float m_varyTime = 0.f;

	std::vector<Vertex_PCU> m_gridVertexes;
};

// bool Event_ResumeGame(EventArgs& args);
// bool Event_OpenSettings(EventArgs& args);
// bool Event_SaveGame(EventArgs& args);
// bool Event_BackToMainMenu(EventArgs& args);



