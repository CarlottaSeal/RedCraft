#include "Engine/Window/Window.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/SimpleTriangleFont.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/DebugRenderSystem.hpp"
#include "Engine/Input/KeyButtonState.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Math/MathUtils.hpp"

#include "Game/App.hpp"
#include "Game/Game.hpp"
#include "Game/Gamecommon.hpp"

#include <math.h>

#include "Engine/Save/SaveSystem.h"

App* g_theApp = nullptr;
Renderer* g_theRenderer = nullptr;
InputSystem* g_theInput = nullptr;
//AudioSystem* g_theAudio = nullptr;
Window* g_theWindow = nullptr;
SaveSystem* g_theSaveSystem = nullptr;
Game* g_theGame = nullptr;

App::App()
{
	//Parse GameConfig
	XmlDocument gameConfigDoc;
	XmlResult loadResult = gameConfigDoc.LoadFile("Data/GameConfig.xml");
	if (loadResult != XmlResult::XML_SUCCESS)
	{
		ERROR_AND_DIE("Cannot find the GameConfig doc!")
	}
	//if load fails, sth will be wrong
	XmlElement* rootElement = gameConfigDoc.RootElement();
	if (rootElement)
	{
		g_gameConfigBlackboard.PopulateFromXmlElementAttributes(*rootElement);
	}

	//Create all engine subsystems
	InputSystemConfig inputConfig;
	g_theInput = new InputSystem(inputConfig);

	WindowConfig windowConfig;
	windowConfig.m_aspectRatio = 2.f;
	windowConfig.m_inputSystem = g_theInput;
	windowConfig.m_windowTitle = g_gameConfigBlackboard.GetValue("windowTitle", "Protogame3D");
	windowConfig.m_isFullscreen = g_gameConfigBlackboard.GetValue("windowFullscreen", false);
	g_theWindow = new Window(windowConfig);

	RendererConfig rendererConfig;
	rendererConfig.m_window = g_theWindow;
	g_theRenderer = new Renderer(rendererConfig);

	EventSystemConfig eventSystemConfig;
	g_theEventSystem = new EventSystem(eventSystemConfig);

	DevConsoleConfig devConsoleConfig;
	devConsoleConfig.m_defaultRenderer = g_theRenderer;
	devConsoleConfig.m_defaultFontName = "SquirrelFixedFont";
	Camera* devCamera = new Camera();
	//devCamera->SetOrthographicView(Vec2(0.f, 0.f), Vec2(1600.f, 800.f));
	devCamera->SetNewAspectRatio(g_theWindow->m_currentAspectRatio, 800.f);
	//devConsoleConfig.m_camera = &g_theGame->m_screenCamera;
	devConsoleConfig.m_camera = devCamera;
	g_theDevConsole = new DevConsole(devConsoleConfig);

	/*AudioSystemConfig audioSystemConfig;
	g_theAudio = new AudioSystem(audioSystemConfig);*/

	unsigned int numCores = std::thread::hardware_concurrency();
	JobSystemConfig jobSystemConfig;
	numCores = GetClampedInt((int)numCores, 0, MAX_CONCURRENT_JOBS);
	jobSystemConfig.m_numWorkerThreads = (int)numCores - 2;
	jobSystemConfig.m_numIOThreads = 1;
	g_theJobSystem = new JobSystem(jobSystemConfig);

	SaveConfig saveConfig;
	g_theSaveSystem = new SaveSystem(saveConfig);

	UIConfig uiConfig;
    uiConfig.m_window = g_theWindow;
    uiConfig.m_renderer = g_theRenderer;
    uiConfig.m_inputSystem = g_theInput;
    uiConfig.m_bitmapFontName = "SquirrelFixedFont";
    g_theUISystem = new UISystem(uiConfig);

	DebugRenderConfig debugRenderConfig;
	debugRenderConfig.m_renderer = g_theRenderer;

	g_theWindow->Startup();
	g_theRenderer->Startup();
	//g_theAudio->Startup();
	g_theEventSystem->StartUp();
	g_theDevConsole->Startup();
	g_theInput->Startup();
	g_theJobSystem->Startup();
    g_theUISystem->Startup();
	DebugRenderSystemStartup(debugRenderConfig);

	g_theGame = new Game();
	//g_theGame->m_gameClock = new Clock(Clock::GetSystemClock());

	g_theEventSystem->SubscribeEventCallBackFunction("quit", OnQuitEvent);
	g_theDevConsole->AddLine(Rgba8::BLUE, "Type help for a list of commands");
}

App::~App()
{
}

void App::Startup()
{
	g_theGame->Startup();
}

void App::Shutdown()
{
	g_theJobSystem->Shutdown();
	delete g_theGame;
	g_theGame = nullptr;

	g_theUISystem->Shutdown();
	g_theEventSystem->Shutdown();
	//g_theAudio->Shutdown();
	g_theRenderer->ShutDown();
	g_theWindow->Shutdown();
	g_theInput->Shutdown();
	g_theDevConsole->Shutdown();

	DebugRenderSystemShutdown();

	delete g_theDevConsole;
	g_theDevConsole = nullptr;

	delete g_theEventSystem;
	g_theEventSystem = nullptr;

	/*delete g_theAudio;
	g_theAudio = nullptr;*/

	delete g_theRenderer;
	g_theRenderer = nullptr;

	delete g_theWindow;
	g_theWindow = nullptr;

	delete g_theInput;
	g_theInput = nullptr;

}

void App::BeginFrame()
{
	Clock::TickSystemClock();
	g_theWindow->BeginFrame();
	g_theInput->BeginFrame();
	g_theRenderer->BeginFrame();
	//g_theAudio->BeginFrame();
	g_theEventSystem->BeginFrame();
	g_theUISystem->BeginFrame();
	g_theDevConsole->BeginFrame();

	DebugRenderBeginFrame();
}

bool App::IsKeyDown(unsigned char keyCode) const
{
	return g_theInput->IsKeyDown(keyCode);
	//return m_keystates[keyCode].IsPressed();
	//return m_currentKeyStates[keyCode]; // 当前帧按键状态
}

bool App::WasKeyJustPressed(unsigned char keyCode) const
{
	return g_theInput->WasKeyJustPressed(keyCode);
	//return m_keystates[keyCode].WasJustPressed();
	//return m_currentKeyStates[keyCode] && !m_previousKeyStates[keyCode]; // 当前帧按下，上一帧未按下
}

void App::HandleKeyPressed(unsigned char keyCode)
{
	g_theInput->HandleKeyPressed(keyCode);
	//m_keystates[keyCode].UpdateStatus(true);
	//m_currentKeyStates[keyCode] = true;
}

void App::HandleKeyReleased(unsigned char keyCode)
{
	g_theInput->HandleKeyReleased(keyCode);
	//m_keystates[keyCode].UpdateStatus(false);
	m_currentKeyStates[keyCode] = false;
}

bool App::IsKeyReleased(unsigned char keyCode) const
{
	return 	m_currentKeyStates[keyCode];
}

void App::WorldRestart()
{
	g_theGame->ForceShutdownCurrentWorld();
	g_theGame->Startup();
	/*delete g_theGame;
	g_theGame = new Game();

	g_theGame->m_isInAttractMode = false;
	g_theGame->Startup();*/
}

void App::HandleQuitRequested()
{
	g_isQuitting = true;
}

void App::RunFrame()
{
	BeginFrame();
	Update();
	Render();
	EndFrame();
}

void App::Update()
{
	XboxController const& controller = g_theInput->GetController(0);
	if (g_theApp->WasKeyJustPressed(KEYCODE_F8) || controller.WasButtonJustPressed(XboxButtonID::A))
	{
		WorldRestart();
	}
	if(g_theGame)
		g_theGame->Update(g_theGame->m_gameClock->GetDeltaSeconds());

	UpdateCursor();
}

void App::Render() const
{
	g_theGame->Render();
}

void App::EndFrame()
{
	// let renderer deal with buffers
	g_theWindow->EndFrame();
	g_theRenderer->EndFrame();
	g_theInput->EndFrame();
	//g_theAudio->EndFrame();
	g_theEventSystem->EndFrame();
	g_theUISystem->EndFrame();
	g_theDevConsole->EndFrame();

	for (int i = 0; i < 256; ++i)
	{
		m_keystates[i].EndFrame();
	}

	DebugRenderEndFrame();
}

void App::UpdateCursor()
{
	if (g_theGame->m_isInAttractMode || g_theGame->m_openDevConsole || !g_theWindow->WindowHasFocus())
	{
		g_theInput->SetCursorMode(CursorMode::POINTER);
	}
	else
	{
		g_theInput->SetCursorMode(CursorMode::FPS);
	}
}

bool OnQuitEvent(EventArgs& args)
{
	UNUSED(args);
	g_theApp->HandleQuitRequested();
	return true;
}