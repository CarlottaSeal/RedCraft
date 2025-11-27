#include "Player.hpp"
#include "Game.hpp"
#include "App.hpp"
#include "Physics/GameCamera.h"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Window/Window.hpp"

extern InputSystem* g_theInput;
extern Window* g_theWindow;
extern App* g_theApp;

Player::Player(Game* owner, Vec3 const& startPos)
	: Entity(owner, startPos)
{
	m_runSpeed = 12.f;
	
	// Player dimensions (1.80m tall, 0.60m wide)
	m_physicsHeight = 1.80f;
	m_physicsRadius = 0.30f;  // Half width = 0.30m
	m_eyeHeight = 1.65f;
	
	// Update physics body
	m_physicsBody.m_mins = Vec3(-m_physicsRadius, -m_physicsRadius, 0.0f);
	m_physicsBody.m_maxs = Vec3(m_physicsRadius, m_physicsRadius, m_physicsHeight);
	
	// Initialize orientation
	m_orientation = EulerAngles(45.0f, 0.0f, 0.0f);
	
	// Create game camera
	m_gameCamera = new GameCamera();
	m_gameCamera->SetPosition(GetEyePosition());
	m_gameCamera->SetOrientation(m_orientation);
	
	// Initialize world camera
	m_worldCamera.SetPosition(GetEyePosition());
	m_worldCamera.SetOrientation(m_orientation);
}

Player::~Player()
{
	if (m_gameCamera)
	{
		delete m_gameCamera;
		m_gameCamera = nullptr;
	}
}

void Player::Update(float deltaSeconds)
{
	UpdateInput(deltaSeconds);
	
	Entity::Update(deltaSeconds);
	
	// Update game camera
	if (m_gameCamera)
	{
		m_gameCamera->Update(deltaSeconds, this, m_game->m_currentWorld);
		
		// Sync world camera with game camera
		m_worldCamera.SetPosition(m_gameCamera->GetPosition());
		m_worldCamera.SetOrientation(m_gameCamera->GetOrientation());
	}
}

void Player::Render() const
{
	// Only render entity bounds if not in first person
	if (m_gameCamera && m_gameCamera->GetCameraMode() != GameCameraMode::FIRST_PERSON)
	{
		Entity::Render();
	}
}

void Player::UpdateInput(float deltaSeconds)
{
	// Check for mode cycling
	if (g_theInput->WasKeyJustPressed('V'))
	{
		CyclePhysicsMode();
	}
	
	if (g_theInput->WasKeyJustPressed('C'))
	{
		CycleCameraMode();
	}

	XboxController const& controller = g_theInput->GetController(0);

	if (m_gameCamera)
	{
		GameCameraMode cameraMode = m_gameCamera->GetCameraMode();
		
		// Only allow player orientation control in these modes
		if (cameraMode == GameCameraMode::FIRST_PERSON ||
			cameraMode == GameCameraMode::OVER_SHOULDER ||
			cameraMode == GameCameraMode::FIXED_ANGLE_TRACKING ||
			cameraMode == GameCameraMode::INDEPENDENT)
		{
			// Mouse look
			Vec2 mouseDelta = g_theInput->GetCursorClientDelta();
			m_orientation.m_yawDegrees -= mouseDelta.x * 0.075f;
			m_orientation.m_pitchDegrees += mouseDelta.y * 0.075f;
			
			// Controller look
			m_orientation.m_yawDegrees -= controller.GetRightStick().GetPosition().x * 90.0f * deltaSeconds;
			m_orientation.m_pitchDegrees += controller.GetRightStick().GetPosition().y * 90.0f * deltaSeconds;
			
			// Clamp pitch
			m_orientation.m_pitchDegrees = GetClamped(m_orientation.m_pitchDegrees, -85.0f, 85.0f);
			m_orientation.m_yawDegrees = fmodf(m_orientation.m_yawDegrees, 360.0f);
		}
	}
	
	// Movement input
	UpdateFromKeyboard(deltaSeconds);
	UpdateFromController(deltaSeconds);
	
	// Jump
	if (g_theInput->IsKeyDown(KEYCODE_SPACE) || controller.IsButtonDown(XboxButtonID::A))
	{
		Jump();
	}
}

void Player::UpdateFromKeyboard(float deltaSeconds)
{
	// Check for run
	m_isRunning = g_theInput->IsKeyDown(KEYCODE_SHIFT);
	
	// Update speed based on running
	if (m_isRunning)
	{
		m_maxSpeed = m_runSpeed;
		m_flySpeed = m_runSpeed * 2.0f;
	}
	else
	{
		m_maxSpeed = m_normalSpeed;
		m_flySpeed = m_normalSpeed * 2.0f;
	}
	
	// Build movement direction
	Vec3 moveDir = Vec3();
	
	Vec3 forward = GetForwardVector();
	Vec3 left = GetLeftVector();
	
	// For walking mode, flatten forward and left to XY plane
	if (m_physicsMode == PhysicsMode::WALKING)
	{
		forward.z = 0.0f;
		if (forward.GetLengthSquared() > 0.0f)
			forward = forward.GetNormalized();
		
		left.z = 0.0f;
		if (left.GetLengthSquared() > 0.0f)
			left = left.GetNormalized();
	}
	
	// WASD movement
	if (g_theInput->IsKeyDown('W'))
		moveDir += forward;
	if (g_theInput->IsKeyDown('S'))
		moveDir -= forward;
	if (g_theInput->IsKeyDown('A'))
		moveDir += left;
	if (g_theInput->IsKeyDown('D'))
		moveDir -= left;

	if (m_physicsMode == PhysicsMode::FLYING || m_physicsMode == PhysicsMode::NOCLIP)
	{
		if (g_theInput->IsKeyDown('Q'))
			moveDir.z += 1.0f;
		if (g_theInput->IsKeyDown('E'))
			moveDir.z -= 1.0f;
	}
	
	// Apply movement
	if (moveDir.GetLengthSquared() > 0.0f)
	{
		MoveInDirection(moveDir, deltaSeconds);
	}
}

void Player::UpdateFromController(float deltaSeconds)
{
	XboxController const& controller = g_theInput->GetController(0);
	if (controller.IsButtonDown(XboxButtonID::LS) || controller.IsButtonDown(XboxButtonID::RS))
	{
		m_isRunning = true;
		m_maxSpeed = m_runSpeed;
		m_flySpeed = m_runSpeed * 2.0f;
	}
	
	// Left stick for movement
	Vec2 leftStick = controller.GetLeftStick().GetPosition();
	if (leftStick.GetLengthSquared() > 0.01f)
	{
		Vec3 forward = GetForwardVector();
		Vec3 left = GetLeftVector();
		
		// For walking mode, flatten to XY
		if (m_physicsMode == PhysicsMode::WALKING)
		{
			forward.z = 0.0f;
			if (forward.GetLengthSquared() > 0.0f)
				forward = forward.GetNormalized();
			
			left.z = 0.0f;
			if (left.GetLengthSquared() > 0.0f)
				left = left.GetNormalized();
		}
		
		Vec3 moveDir = forward * leftStick.y + left * (-leftStick.x);
		MoveInDirection(moveDir, deltaSeconds);
	}
	
	// Triggers for vertical movement (flying/noclip only)
	if (m_physicsMode == PhysicsMode::FLYING || m_physicsMode == PhysicsMode::NOCLIP)
	{
		float verticalInput = controller.GetLeftTrigger() - controller.GetRightTrigger();
		if (fabsf(verticalInput) > 0.01f)
		{
			Vec3 moveDir(0.0f, 0.0f, verticalInput);
			MoveInDirection(moveDir, deltaSeconds);
		}
	}
}

void Player::CyclePhysicsMode()
{
	int currentMode = (int)m_physicsMode;
	currentMode = (currentMode + 1) % 3;
	m_physicsMode = (PhysicsMode)currentMode;
	
	// Print mode to console
	const char* modeNames[] = { "WALKING", "FLYING", "NOCLIP" };
	//DebuggerPrintf("Physics Mode: %s\n", modeNames[currentMode]);
	m_playerModeString = Stringf("Physics Mode: %s\n", modeNames[currentMode]);
}

void Player::CycleCameraMode()
{
	if (m_gameCamera)
	{
		m_gameCamera->CycleCameraMode();
	}
}

Direction Player::GetOrthoDirection()
{
	Vec3 forward = GetForwardVector();
	float absX = fabsf(forward.x);
	float absY = fabsf(forward.y);
	float absZ = fabsf(forward.z);
    
	if (absZ > absX && absZ > absY)
	{
		// 上下方向占主导
		return (forward.z >= 0.0f) ? DIRECTION_UP : DIRECTION_DOWN;
	}
	else if (absX > absY)
	{
		return (forward.x >= 0.0f) ? DIRECTION_EAST : DIRECTION_WEST;
	}
	else
	{
		return (forward.y >= 0.0f) ? DIRECTION_NORTH : DIRECTION_SOUTH;
	}
}

Direction Player::GetHorizontalDirection()
{
	Vec3 forward = GetForwardVector();

	forward.z = 0.0f;
	if (forward.GetLengthSquared() < 1e-6f)
	{
		return DIRECTION_NORTH;
	}

	forward = forward.GetNormalized();
	float absX = fabsf(forward.x);
	float absY = fabsf(forward.y);
	if (absX > absY)
	{
		return (forward.x >= 0.0f) ? DIRECTION_EAST : DIRECTION_WEST;
	}
	else
	{
		return (forward.y >= 0.0f) ? DIRECTION_NORTH : DIRECTION_SOUTH;
	}
}

Direction Player::GetOrthoDirectionOpposite()
{
	Direction ortho = GetOrthoDirection();
	switch (ortho)
	{
		case DIRECTION_NORTH:
		return DIRECTION_SOUTH;
		case DIRECTION_SOUTH:
		return DIRECTION_NORTH;
		case DIRECTION_EAST:
		return DIRECTION_WEST;
		case DIRECTION_WEST:
		return DIRECTION_EAST;
	case DIRECTION_UP:
		return DIRECTION_DOWN;
		default:
		return DIRECTION_UP;
	}
}
