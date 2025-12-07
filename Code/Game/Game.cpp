#include "Game.hpp"

#include "Block.h"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/AABB2.hpp"    
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/DebugRenderSystem.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/SpriteDefinition.hpp"
#include "Engine/Audio/AudioSystem.hpp"

#include "Game/Player.hpp"
#include "Game/Gamecommon.hpp"
#include "BlockDefinition.h"
#include "Chunk.h"
#include "ChunkUtils.h"
#include "World.h"
#include "Engine/UI/UIManager.h"
#include "ThirdParty/ImGui/imgui.h"
#include "UI/GameUIManager.h"

RandomNumberGenerator* g_theRNG = nullptr;
//extern AudioSystem* g_theAudio;
extern Clock* s_theSystemClock;
extern Game* g_theGame;

Game::Game()
{
	if(!m_gameClock)
		m_gameClock = new Clock(Clock::GetSystemClock());

	m_isInAttractMode = true;
	m_screenCamera.SetNewAspectRatio(g_theWindow->m_currentAspectRatio, 800.f);
	//m_screenCamera.SetOrthographicView(Vec2(0.f, 0.f), Vec2(1600.f, 1000.f));

	m_player = new Player(this, Vec3(-50.f, -50.f, 150.f));

	m_player->m_worldCamera.SetCameraMode(Camera::CameraMode::eMode_Perspective);
	m_player->m_worldCamera.SetPerspectiveView(2.f, 60.f, 0.1f, 3000.f);
	Mat44 mat;
	mat.SetIJK3D(Vec3(0.f, 0.f,1.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f));
	m_player->m_worldCamera.SetCameraToRenderTransform(mat);
	
	PrintGameControlToDevConsole();

	DebugAddWorldBasis(mat, -1.f, DebugRenderMode::USE_DEPTH);
	DebugAddWorldAxisText(mat);

	g_theRNG = new RandomNumberGenerator();

	m_gameUIManager = new GameUIManager(g_theUISystem);
	g_theEventSystem->SubscribeEventCallBackFunction("ResumeGame", Event_ResumeGame);
	g_theEventSystem->SubscribeEventCallBackFunction("OpenSettings", Event_OpenSettings);
	g_theEventSystem->SubscribeEventCallBackFunction("SaveGame", Event_SaveGame);
	g_theEventSystem->SubscribeEventCallBackFunction("BackToMainMenu", Event_BackToMainMenu);
}

Game::~Game()
{
	if (m_currentWorld)
	{
		m_currentWorld->SaveAllModifiedChunks();
		m_currentWorld->ForceDeactivateAllChunks();
	}

	delete m_gameUIManager;
	m_gameUIManager = nullptr;
	delete m_player;
	m_player = nullptr;
	delete m_spriteSheet;
	m_spriteSheet = nullptr;

	BlockDefinition::ClearDefinitions();
}

void Game::Startup()
{
	World* oldWorld = m_currentWorld;

	Texture* blockTex = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/CustomSheet.png", true);
	m_spriteSheet = new SpriteSheet(*blockTex, IntVec2(16,16));

	BlockDefinition::InitializeBlockDefs();

	m_currentWorld = new World(this);

	if (oldWorld)                     
		m_worldsToDelete.push_back(oldWorld);
}

void Game::ForceShutdownCurrentWorld()
{
}

void Game::Update()
{
	if (g_theApp->WasKeyJustPressed(KEYCODE_ESC)
		|| g_theInput->GetController(0).WasButtonJustPressed(XboxButtonID::X))
	{
		if (m_isInAttractMode)
		{
			g_theApp->g_isQuitting = true;
		}
		if (!m_isInAttractMode)
		{
			m_isInAttractMode = true;
		}
	}

	// if (g_theApp->WasKeyJustPressed(KEYCODE_F8))
	// {
	// 	delete m_currentWorld;
	// 	m_currentWorld = new World(this);
	// }

	AdjustForPauseAndTimeDistortion();

	if (m_isInAttractMode)
	{
		AttractModeUpdate();

		if (!m_hasPlayedAttractSound)
		{
			//SoundID Attract = g_theAudio->CreateOrGetSound("Data/Audio/Attract.MP3");
			//m_attractSoundID = g_theAudio->StartSound(Attract, false, 1.0f, 0.5f, 1.0f, false);
			m_hasPlayedAttractSound = true;
		}
	}

	if (!m_isInAttractMode)
	{
		GameStateUpdate();
	}

	CleanupOldWorlds();

	if (!g_theGame)
	{
		return;
	}
	if (g_theDevConsole->GetMode() == OPEN_FULL)
	{
		m_openDevConsole = true;
	}
	if (g_theDevConsole->GetMode() == HIDDEN)
	{
		m_openDevConsole = false;
	}

	m_varyTime += (float)m_gameClock->GetDeltaSeconds();
	if (m_varyTime > 360.f)
	{
		m_varyTime = 0.f;
	}
}

void Game::Render() const
{
	if (!g_theGame)
		return;
	if (m_isInAttractMode)
	{
		g_theRenderer->ClearScreen();
		g_theRenderer->BeginCamera(m_screenCamera);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
		AttractStateRender();
		g_theRenderer->EndCamera(m_screenCamera);
	}

	if (!m_isInAttractMode)// && !IsInSelectInterface)
	{
		//g_theRenderer->ClearScreen(Rgba8(0, 0, 0));
		g_theRenderer->ClearScreen(m_currentWorld->m_skyColor);
		g_theRenderer->BeginCamera(((Player*)m_player)->m_worldCamera);

		GameStateRender();

		DebugRenderWorld(((Player*)m_player)->m_worldCamera);
		DebugRenderScreen(m_screenCamera);
	}

	g_theDevConsole->Render(AABB2(m_screenCamera.GetOrthographicBottomLeft(), m_screenCamera.GetOrthographicTopRight()), g_theRenderer);
}

void Game::AdjustForPauseAndTimeDistortion()
{
	if (!g_theGame)
	{
		return;
	}
	if (g_theApp->WasKeyJustPressed('T'))
	{
		m_isSlowMo = !m_isSlowMo;
		m_isUsingUserTimeScale = false;
	}
	if (m_isUsingUserTimeScale) // 如果用户通过 set_time_scale 设定了时间缩放，保持该值
	{
		m_gameClock->SetTimeScale(m_userTimeScale);
	}
	else
	{
		m_gameClock->SetTimeScale(m_isSlowMo ? 0.1f : 1.0f);
	}

	if (g_theApp->WasKeyJustPressed('P'))
	{
		m_gameClock->TogglePause();
		if (!m_gameClock->IsPaused())
		{
			m_gameClock->Unpause();
			m_gameClock->Reset();
			//SoundID Pause = g_theAudio->CreateOrGetSound("Data/Audio/Pause.mp3");
			//g_theAudio->StartSound(Pause, false, 0.05f, 0.5f, 1.f, false);
		}
		if (m_gameClock->IsPaused())
		{
			m_gameClock->Reset();
			//SoundID Unpause = g_theAudio->CreateOrGetSound("Data/Audio/Unpause.mp3");
			//g_theAudio->StartSound(Unpause, false, 0.05f, 0.5f, 1.f, false);
		}
	}

	if (g_theApp->WasKeyJustPressed('O'))
	{
		m_gameClock->StepSingleFrame();
	}
}

void Game::CleanupOldWorlds()
{
	if (m_worldsToDelete.empty()) return;

	if (g_theJobSystem &&
		g_theJobSystem->GetPendingJobCount() == 0 &&
		g_theJobSystem->GetExecutingJobCount() == 0)
	{
		for (World* world : m_worldsToDelete)
			delete world;
		m_worldsToDelete.clear();
	}
}

void Game::AttractModeUpdate()
{
	if (g_theApp->IsKeyDown(' ') ||
		g_theApp->IsKeyDown('N') ||
		g_theInput->GetController(0).WasButtonJustPressed(XboxButtonID::START))
		//||g_theInput->GetController(0).WasButtonJustPressed(XboxButtonID::A))
	{
		m_isInAttractMode = false;
	}
}

void Game::AttractStateRender() const
{
	DebugDrawRing(m_screenCamera.GetOrthographicCenter(), 100.f, 10.f + sinf(m_varyTime) * 10.f, Rgba8::YELLOW);
}

void Game::GameStateUpdate()
{
	//XboxController const& controller = g_theInput->GetController(0);

	if(m_player)
		m_player->Update((float)s_theSystemClock->GetDeltaSeconds());
	if(m_currentWorld)
		m_currentWorld->Update((float)s_theSystemClock->GetDeltaSeconds());

	if (g_theApp->WasKeyJustPressed(KEYCODE_F2))
	{
		m_currentWorld->ToggleDebugMode();
	}
	if (g_theApp->WasKeyJustPressed(KEYCODE_F3))
	{
		m_currentWorld->ToggleDebugPrintingMode();
	}
	DebugRenderSystemInputUpdate();

	ImGuiUpdate();
}

void Game::GameStateRender() const
{
	m_currentWorld->Render();
	
	g_theRenderer->BindTexture(nullptr);
	m_player->Render();
	GameAxisDebugRender();
}

void Game::GameAxisDebugRender() const
{
	Mat44 mat;
	mat.SetIJK3D(Vec3(0.f, 0.f, 1.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f));
	Vec3 worldI = mat.GetIBasis3D();
	Vec3 worldJ = mat.GetJBasis3D();
	Vec3 worldK = mat.GetKBasis3D();

	Vec3 subtractedI = (worldI.GetNormalized());
	Vec3 subtractedJ = (worldJ.GetNormalized());
	Vec3 subtractedK = (worldK.GetNormalized());

	Vec3 playerI, playerJ, playerK;
	m_player->m_orientation.GetAsVectors_IFwd_JLeft_KUp(playerI, playerJ, playerK);

	Vec3 fakeAxisPos = m_player->m_position + playerI.GetNormalized() * 50.f;
	Vec3 newK = fakeAxisPos + subtractedK;
	Vec3 newI = fakeAxisPos + subtractedI;
	Vec3 newJ = fakeAxisPos - subtractedJ;
	DebugAddWorldArrow(fakeAxisPos, newI, 0.05f, 0.f, Rgba8::AQUA, Rgba8::AQUA, DebugRenderMode::ALWAYS);
	DebugAddWorldArrow(fakeAxisPos, newK, 0.05f, 0.f, Rgba8::MINTGREEN, Rgba8::MINTGREEN, DebugRenderMode::ALWAYS);
	DebugAddWorldArrow(fakeAxisPos, newJ, 0.05f, 0.f, Rgba8::MAGENTA, Rgba8::MAGENTA, DebugRenderMode::ALWAYS);
}

void Game::ImGuiUpdate()
{
	ImGui::Begin("World Generation Settings");

	if (ImGui::Button("Regenerate", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
	{
		g_theApp->WorldRestart();
		g_theSaveSystem->ForceDeleteFolder();
		g_theSaveSystem->ForceCreateDefaultSaveFolder();
		//return;
	}
	ImGui::Separator(); 
	ImGui::Spacing();  

    // ========== Density Noise ==========
    if (ImGui::CollapsingHeader("Density Noise Settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Indent();
        ImGui::Text("Density Noise");
        ImGui::DragFloat("Density Noise Scale", &g_densityNoiseScale, 1.0f, 0.0f, 1000.0f);
        ImGui::DragInt("Density Noise Octaves", &g_densityNoiseOctaves, 1, 1, 16);
        
        ImGui::Text("Density Noise Bias");
        ImGui::DragFloat("Terrain Height", &g_terrainHeight, 1.0f, 0.0f, 256.0f);
        ImGui::DragFloat("Reference Terrain Height", &g_terrainReferenceHeight, 1.0f, 0.0f, 256.0f);
        ImGui::DragFloat("Bias Per Z", &g_biasPerZ, 0.001f, 0.0f, 1.0f, "%.3f");
        ImGui::Unindent();
    }
    
    // ========== Continent ==========
    if (ImGui::CollapsingHeader("Continent Settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
    	ImGui::Indent();
    	ImGui::DragFloat("Continent Noise Scale", &g_continentNoiseScale, 1.0f, 0.0f, 5000.0f);
    	ImGui::DragInt("Continent Noise Octaves", &g_continentNoiseOctaves, 1, 1, 16);
        
    	// Height Offset Curve
    	if (ImGui::TreeNode("Height Offset Curve"))
    	{
    		ImGuiDrawCurveEditor("HeightOffsetCurve", g_heightOffsetCurvePoints);
    		ImGui::TreePop();
    	}
        
    	// Height Scale Curve
    	if (ImGui::TreeNode("Height Scale Curve"))
    	{
    		ImGuiDrawCurveEditor("HeightScaleCurve", g_heightScaleCurvePoints);
    		ImGui::TreePop();
    	}
        
    	ImGui::Unindent();
    }
    
    // ========== Erosion ==========
    if (ImGui::CollapsingHeader("Erosion Settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Indent();
        ImGui::DragFloat("Erosion Noise Scale", &g_erosionNoiseScale, 1.0f, 0.0f, 5000.0f);
        ImGui::DragInt("Erosion Noise Octaves", &g_erosionNoiseOctaves, 1, 1, 16);
        ImGui::Unindent();
    }
    
    // ========== Peaks and Valleys ==========
    if (ImGui::CollapsingHeader("Peaks and Valleys Settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Indent();
        ImGui::DragFloat("Peaks and Valleys Noise Scale", &g_peaksValleysNoiseScale, 1.0f, 0.0f, 5000.0f);
        ImGui::DragInt("Peaks and Valleys Noise Octaves", &g_peaksValleysNoiseOctaves, 1, 1, 16);
        ImGui::Unindent();
    }
    
    // ========== Temperature ==========
    if (ImGui::CollapsingHeader("Temperature Settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Indent();
        ImGui::DragFloat("Temperature Noise Scale", &g_temperatureNoiseScale, 1.0f, 0.0f, 5000.0f);
        ImGui::DragInt("Temperature Noise Octaves", &g_temperatureNoiseOctaves, 1, 1, 16);
        ImGui::Unindent();
    }
    
    // ========== Humidity ==========
    if (ImGui::CollapsingHeader("Humidity Settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Indent();
        ImGui::DragFloat("Humidity Noise Scale", &g_humidityNoiseScale, 1.0f, 0.0f, 5000.0f);
        ImGui::DragInt("Humidity Noise Octaves", &g_humidityNoiseOctaves, 1, 1, 16);
        ImGui::Unindent();
    }
    
    // ========== Other Settings ==========
    if (ImGui::CollapsingHeader("Other Settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Indent();
        ImGui::Checkbox("Sea Enabled", &g_seaEnabled);
        ImGui::DragInt("Sea Level", &g_seaLevel, 1.0f, 0, 256);
        ImGui::Checkbox("Density Noise Enabled", &g_densityNoiseEnabled);
        ImGui::Checkbox("Density Noise Bias Enabled", &g_densityNoiseBiasEnabled);
        ImGui::Checkbox("Continent Height Offset Enabled", &g_continentHeightOffsetEnabled);
        ImGui::Checkbox("Continent Height Scale Enabled", &g_continentHeightScaleEnabled);
        ImGui::Checkbox("Cave Carving Enabled", &g_caveCarvingEnabled);
        ImGui::Checkbox("Block Replacement Enabled", &g_blockReplacementEnabled);
        ImGui::Checkbox("Tree Generation Enabled", &g_treeGenerationEnabled);
        ImGui::Unindent();
    }
    
    // ========== Debug ==========
    if (ImGui::CollapsingHeader("Debug", ImGuiTreeNodeFlags_DefaultOpen))
    {
    	ImGui::Indent();
    
    	ImGui::RadioButton("None", &g_debugVisualizationMode, 0);
    	ImGui::RadioButton("Continents", &g_debugVisualizationMode, 1);
    	ImGui::RadioButton("Erosion", &g_debugVisualizationMode, 2);
    	ImGui::RadioButton("Peaks and Valleys", &g_debugVisualizationMode, 3);
    	ImGui::RadioButton("Temperature", &g_debugVisualizationMode, 4);
    	ImGui::RadioButton("Humidity", &g_debugVisualizationMode, 5);
    	ImGui::RadioButton("Biomes", &g_debugVisualizationMode, 6);
    	ImGui::RadioButton("Smoothed", &g_debugVisualizationMode, 7);
    
    	ImGui::Checkbox("Chunk Bounds", &g_showChunkBounds);
    
    	ImGui::Unindent();
    }
    
    ImGui::End();
}

void Game::ImGuiDrawCurveEditor(const char* label, std::vector<Vec2>& points)
{
	ImGui::PushID(label);
    
    ImGui::Text("Settings");
    ImGui::Separator();
    
    ImGui::PushItemWidth(80);
    ImGui::DragFloat("Grid Size", &g_curveGridSize, 0.01f, 0.01f, 1.0f, "%.2f");
    ImGui::SameLine();
    ImGui::Text("Snap");
    
    ImGui::DragFloat("X Min", &g_curveXMin, 0.1f, -10.0f, 10.0f, "%.2f");
    ImGui::SameLine();
    ImGui::DragFloat("##XMax", &g_curveXMax, 0.1f, -10.0f, 10.0f, "%.2f");
    
    ImGui::DragFloat("Y Min", &g_curveYMin, 0.1f, -10.0f, 10.0f, "%.2f");
    ImGui::SameLine();
    ImGui::DragFloat("##YMax", &g_curveYMax, 0.1f, -10.0f, 10.0f, "%.2f");
    ImGui::PopItemWidth();
    
    ImGui::Spacing();
    
    ImGui::Text("Points");
    ImGui::Separator();
    
    if (ImGui::Button("Add Point"))
    {
        points.push_back(Vec2(0.0f, 0.0f));
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear Points"))
    {
        points.clear();
    }
    
    ImGui::Spacing();
    
    ImGui::PushItemWidth(80);
    for (int i = 0; i < points.size(); i++)
    {
        ImGui::PushID(i);
        
        ImGui::DragFloat("##X", &points[i].x, 0.01f, g_curveXMin, g_curveXMax, "%.2f");
        ImGui::SameLine();
        
        ImGui::Text("x");
        ImGui::SameLine();
        
        ImGui::DragFloat("##Y", &points[i].y, 0.01f, g_curveYMin, g_curveYMax, "%.2f");
        ImGui::SameLine();
        
        if (ImGui::Button("-"))
        {
            points.erase(points.begin() + i);
            i--;
        }
        
        ImGui::PopID();
    }
    ImGui::PopItemWidth();
    
    ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Text("Preview");

	// 获取绘图区域
	ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size = ImVec2(ImGui::GetContentRegionAvail().x, 150);
	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	// 绘制背景
	draw_list->AddRectFilled(canvas_pos, 
		ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), 
		IM_COL32(40, 40, 40, 255));
	
	// 绘制曲线点和连线
	if (points.size() > 0)
	{
		for (int i = 0; i < points.size(); i++)
		{
			// 将曲线坐标转换为屏幕坐标
			float x = canvas_pos.x + (points[i].x - g_curveXMin) / (g_curveXMax - g_curveXMin) * canvas_size.x;
			float y = canvas_pos.y + canvas_size.y - (points[i].y - g_curveYMin) / (g_curveYMax - g_curveYMin) * canvas_size.y;
        
			// 绘制点
			draw_list->AddCircleFilled(ImVec2(x, y), 4, IM_COL32(100, 150, 255, 255));
        
			// 绘制连线
			if (i > 0)
			{
				float prev_x = canvas_pos.x + (points[i-1].x - g_curveXMin) / (g_curveXMax - g_curveXMin) * canvas_size.x;
				float prev_y = canvas_pos.y + canvas_size.y - (points[i-1].y - g_curveYMin) / (g_curveYMax - g_curveYMin) * canvas_size.y;
				draw_list->AddLine(ImVec2(prev_x, prev_y), ImVec2(x, y), IM_COL32(100, 150, 255, 255), 2.0f);
			}
		}
	}

	ImGui::Dummy(canvas_size); // 占位
	ImGui::Spacing();
    
    if (ImGui::Button("Save Points"))
    {
        // 保存点到文件或应用到世界生成
    }
    
    ImGui::PopID();
}

void Game::PrintGameControlToDevConsole()
{
	g_theDevConsole->AddLine(Rgba8::CYAN, "Mouse - Aim");
	g_theDevConsole->AddLine(Rgba8::CYAN, "W / A - Move");
	g_theDevConsole->AddLine(Rgba8::CYAN, "S / D - Strafe");
	g_theDevConsole->AddLine(Rgba8::CYAN, "Z / C - Elevate");
	g_theDevConsole->AddLine(Rgba8::CYAN, "Shift - Sprint");
	g_theDevConsole->AddLine(Rgba8::CYAN, "~     - Open Dev Console");
	g_theDevConsole->AddLine(Rgba8::CYAN, "Space - Start Game");
	g_theDevConsole->AddLine(Rgba8::CYAN, "ESC   - Exit game");
	g_theDevConsole->AddLine(Rgba8::CYAN, "F2    - Toggle Debug");
	g_theDevConsole->AddLine(Rgba8::CYAN, "F8    - Reload");
}

void Game::DebugRenderSystemInputUpdate()
{
	// Vec3 pos = m_player->m_position;
	// std::string reportHUD = " Player Position: " +
	// 	RoundToOneDecimalString(pos.x) + ", " + RoundToOneDecimalString(pos.y) + ", " + RoundToOneDecimalString(pos.z);
	// DebugAddMessage(reportHUD, 0.f, m_screenCamera);
	
	//Add fps...
	float timeTotal = (float)m_gameClock->GetTotalSeconds();
	float fps = (float)m_gameClock->GetFrameRate();
	//float timeScale = m_gameClock->GetTimeScale();
	// std::string timeReportHUD = " Time: " + RoundToTwoDecimalsString(timeTotal) 
	// + " FPS: " + RoundToOneDecimalString(fps) + " Scale: " + RoundToTwoDecimalsString(timeScale);
	std::string timeReportHUD = "Time: " + RoundToTwoDecimalsString(timeTotal) + " FPS: " + RoundToOneDecimalString(fps);
	float textWidth = GetTextWidth(12.f, timeReportHUD, 0.7f);
	DebugAddScreenText(timeReportHUD, m_screenCamera.GetOrthographicTopRight() - Vec2(textWidth + 1.f, 15.f), 12.f, Vec2::ZERO, 0.f);

	std::string typeMsg = "[LMB] Dig [RMB] Add " + BlockDefinition::GetBlockDef(m_currentWorld->m_typeToPlace).m_name
			+ " [1] Glowstone [2]CobbleStone [3]ChiseledBrick";
	DebugAddScreenText(
		typeMsg,
		Vec2(m_screenCamera.GetOrthographicBottomLeft().x, m_screenCamera.GetOrthographicTopRight().y) -
		Vec2(0.f, 15.f), 12.f, Vec2::ZERO, 0.f);
	std::string modeMsg = m_player->m_playerModeString + " " + m_player->m_gameCamera->m_cameraModeString;
	DebugAddScreenText(
		modeMsg,
		Vec2(m_screenCamera.GetOrthographicBottomLeft().x, m_screenCamera.GetOrthographicTopRight().y) -
		Vec2(0.f, 30.f), 12.f, Vec2::ZERO, 0.f);

	if (m_currentWorld->IsDebuggingPrinting())
	{
		// std::string worldSection = "World\n";
		// worldSection += "\n";
		// worldSection += Stringf("chunksPendingGeneration: %d\n", (int)m_currentWorld->m_processingChunks.size());
		// worldSection += "\n";
		// worldSection += Stringf("chunksGenerating: %d\n", (int)m_currentWorld->m_generatingChunksCount);
		// worldSection += "\n";
		// worldSection += Stringf("chunks: %d\n", (int)m_currentWorld->m_activeChunks.size());
		// worldSection += "\n";
		// std::string jobSection = "JobSystem\n";
		// worldSection += "\n";
		// jobSection += Stringf("Pending Jobs: %d\n", g_theJobSystem->GetPendingJobCount());
		// worldSection += "\n";
		// jobSection += Stringf("Executing Jobs: %d\n", g_theJobSystem->GetExecutingJobCount());
		// worldSection += "\n";
		// jobSection += Stringf("Completed Jobs: %d\n", g_theJobSystem->GetCompletedJobCount());
		//
		// worldSection += jobSection;
		// DebugAddScreenText(
		// worldSection, m_screenCamera.GetOrthographicBottomLeft()+Vec2(0.f, 8.f * 15.f), 12.f, Vec2::ZERO, 0.f);
	}

	// if (g_theApp->WasKeyJustPressed('0'))
	// {
	// 	Chunk* chunk = m_currentWorld->GetChunkFromPlayerCameraPosition(m_player->m_position);
	// 	if (!chunk) return;
	// 	IntVec3 globalCoords(
	// 		static_cast<int>(std::floor(m_player->m_position.x)),
	// 		static_cast<int>(std::floor(m_player->m_position.y)),
	// 		static_cast<int>(std::floor(m_player->m_position.z))
	// 	);
	//
	// 	IntVec3 localCoords = GlobalCoordsToLocalCoords(globalCoords);
	// 	std::string biomeType = std::to_string(chunk->m_chunkGenData.m_biomes[localCoords.x][localCoords.y]);
	// 	float c = chunk->m_chunkGenData.m_biomeParams[localCoords.x][localCoords.y].m_continentalness;
	// 	DebugAddMessage(" Biome Type at Player: " + biomeType + "Continentalness: " + std::to_string(c),
	// 		2.f, m_screenCamera);
	// }
}

void Game::DebugAddWorldAxisText(Mat44 worldMat)
{
	Mat44 xMat;
	xMat.SetIJK3D(Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 0.f, 1.f), Vec3(0.f, 1.f, 0.f));
	xMat.Append(worldMat);
	DebugAddWorldText("x-Forward", xMat, 0.2f, Vec2(-0.05f, -0.3f), -1.f, Rgba8::MAGENTA);

	Mat44 yMat;
	yMat.SetIJK3D(Vec3(0.f, 1.f, 0.f), Vec3(0.f, 0.f, 1.f), Vec3(1.f, 0.f, 0.f));
	yMat.Append(worldMat);
	DebugAddWorldText("y-Left", yMat, 0.2f, Vec2( 1.f,-0.3f), -1.f, Rgba8::MINTGREEN);

	Mat44 zMat;
	zMat.SetIJK3D(Vec3(0.f, 0.f, -1.f), Vec3(0.f, 1.f, 0.f), Vec3(1.f, 0.f, 0.f));
	zMat.Append(worldMat);
	DebugAddWorldText("z-Up", zMat, 0.2f, Vec2(-0.3f, 1.5f), -1.f, Rgba8::AQUA);
}

bool Event_ResumeGame(EventArgs& args)
{
	UNUSED(args);
	g_theGame->m_gameUIManager->ClosePauseMenu();
	return true;
}

bool Event_OpenSettings(EventArgs& args)
{
	UNUSED(args);
	g_theGame->m_gameUIManager->OpenSettings();
	return true;
}

bool Event_SaveGame(EventArgs& args)
{
	UNUSED(args);
	g_theGame->m_gameUIManager->ShowActionMessage("Game Saved!", 2.0f);
	return true;
}

bool Event_BackToMainMenu(EventArgs& args)
{
	UNUSED(args);
	g_theGame->m_gameUIManager->CloseAllScreens();
	g_theGame->m_gameUIManager->OpenMainMenu();
	return true;
}
