#include "Entity.h"
#include "Game/Game.hpp"
#include "Game/World.h"
#include "PhysicsUtils.h"  
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/ChunkUtils.h"

extern Game* g_theGame;
extern Renderer* g_theRenderer;

constexpr float GROUND_CHECK_DISTANCE = 0.01f;
constexpr float COLLISION_EPSILON = 0.001f;

Entity::Entity(Game* owner, Vec3 const& startPos)
	: m_game(owner)
	, m_position(startPos)
	, m_velocity(Vec3())
	, m_orientation(EulerAngles())
{
	m_physicsHeight = PLAYER_HEIGHT;
	m_physicsRadius = PLAYER_WIDTH * 0.5f;
	m_eyeHeight = PLAYER_EYE_HEIGHT;
	
	m_walkSpeed = WALK_SPEED;
	m_runSpeed = RUN_SPEED;
	m_flySpeed = FLY_SPEED;
	m_jumpSpeed = JUMP_VELOCITY;
	
	// Setup physics body (local space AABB centered at origin)
	m_physicsBody.m_mins = Vec3(-m_physicsRadius, -m_physicsRadius, 0.0f);
	m_physicsBody.m_maxs = Vec3(m_physicsRadius, m_physicsRadius, m_physicsHeight);
}

Entity::~Entity()
{
}

void Entity::Update(float deltaSeconds)
{
	UpdatePhysics(deltaSeconds);
}

void Entity::Render() const
{
	if (!m_isVisible)
		return;

	// Get world-space physics bounds
	AABB3 worldBounds = GetPhysicsBounds();
	
	// Render AABB in cyan wireframe
	std::vector<Vertex_PCU> verts;
	AddVertsForAABB3D(verts, worldBounds, Rgba8::CYAN);
	//AddVertsForIndexAABBZWireframe3D(verts, worldBounds, Rgba8::CYAN);
	
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetModelConstants();
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->DrawVertexArray((int)verts.size(), verts.data());
	
	// Render velocity vector if moving
	if (m_velocity.GetLength() > 0.01f)
	{
		verts.clear();
		Vec3 endPos = m_position + m_velocity * 0.5f;
		AddVertsForArrow3D(verts, m_position, endPos, 0.05f, Rgba8::YELLOW);
		g_theRenderer->DrawVertexArray((int)verts.size(), verts.data());
	}
}

void Entity::UpdatePhysics(float deltaSeconds)
{
	switch (m_physicsMode)
	{
	case PhysicsMode::WALKING:
		UpdateWalkingPhysics(deltaSeconds);
		break;
	case PhysicsMode::FLYING:
		UpdateFlyingPhysics(deltaSeconds);
		break;
	case PhysicsMode::NOCLIP:
		UpdateNoClipPhysics(deltaSeconds);
		break;
	}
}

void Entity::UpdateWalkingPhysics(float deltaSeconds)
{
	// 使用PhysicsUtils中的GRAVITY常量
	m_velocity.z += GRAVITY * deltaSeconds;
	
	if (m_velocity.z < TERMINAL_VELOCITY)
	{
		m_velocity.z = TERMINAL_VELOCITY;
	}
	
	// 使用PhysicsUtils中的ApplyFriction函数
	float friction = m_isOnGround ? m_groundFriction : m_airFriction;
	m_velocity = ApplyFriction(m_velocity, friction, deltaSeconds);
	
	// Apply velocity and resolve collisions
	ResolveWorldCollisions(deltaSeconds);
	
	// Check ground status
	m_isOnGround = CheckGroundStatus();
}

void Entity::UpdateFlyingPhysics(float deltaSeconds)
{
	// No gravity in flying mode
	
	// Apply friction (similar to air friction)
	m_velocity = ApplyFriction(m_velocity, m_airFriction, deltaSeconds);
	
	// Apply velocity and resolve collisions
	ResolveWorldCollisions(deltaSeconds);
	
	m_isOnGround = false; // Never on ground in flying mode
}

void Entity::UpdateNoClipPhysics(float deltaSeconds)
{
	// No gravity, no friction, no collisions
	
	// Just apply velocity directly
	m_position += m_velocity * deltaSeconds;
	
	m_isOnGround = false;
}

void Entity::AddForce(Vec3 const& force)
{
	// F = ma, so a = F/m (assuming mass = 1)
	m_velocity += force;
}

void Entity::AddImpulse(Vec3 const& impulse)
{
	m_velocity += impulse;
}

void Entity::MoveInDirection(Vec3 const& direction, float deltaSeconds)
{
	Vec3 inputDir = direction;
	if (inputDir.GetLengthSquared() > 0.0f)
	{
		inputDir = inputDir.GetNormalized();
	}
	else
	{
		return; // No input
	}
	
	// Calculate desired velocity based on mode
	float targetSpeed = m_walkSpeed;
	if (m_physicsMode == PhysicsMode::FLYING || m_physicsMode == PhysicsMode::NOCLIP)
	{
		targetSpeed = m_flySpeed;
	}

	Vec3 wishDir = inputDir;
	if (m_physicsMode == PhysicsMode::WALKING)
	{
		// Only accelerate in XY for walking
		wishDir.z = 0.0f;
		if (wishDir.GetLengthSquared() > 0.0f)
		{
			wishDir = wishDir.GetNormalized();
		}
	}
	
	m_velocity = Accelerate(m_velocity, wishDir, targetSpeed, m_acceleration, deltaSeconds);
}

void Entity::Jump()
{
	if (m_physicsMode == PhysicsMode::WALKING && m_isOnGround)
	{
		m_velocity.z = m_jumpSpeed;
		m_isOnGround = false;
	}
}

void Entity::ResolveWorldCollisions(float deltaSeconds)
{
	if (m_physicsMode == PhysicsMode::NOCLIP)
	{
		m_position += m_velocity * deltaSeconds;
		return;
	}
	
	World* world = m_game->m_currentWorld;
	if (!world)
	{
		m_position += m_velocity * deltaSeconds;
		return;
	}
	
	// Move and resolve collisions per axis (Quake-style collision)
	
	// X axis
	m_position.x += m_velocity.x * deltaSeconds;
	if (IsCollidingWithWorld())
	{
		PushOutOfBlocksX();
		m_velocity.x = 0.0f;
	}
	
	// Y axis
	m_position.y += m_velocity.y * deltaSeconds;
	if (IsCollidingWithWorld())
	{
		PushOutOfBlocksY();
		m_velocity.y = 0.0f;
	}
	
	// Z axis
	m_position.z += m_velocity.z * deltaSeconds;
	if (IsCollidingWithWorld())
	{
		PushOutOfBlocksZ();
		
		if (m_velocity.z < 0.0f)
		{
			m_isOnGround = true;
		}
		
		m_velocity.z = 0.0f;
	}
}

bool Entity::CheckGroundStatus()
{
	if (m_physicsMode != PhysicsMode::WALKING)
		return false;
	
	World* world = m_game->m_currentWorld;
	if (!world)
		return false;
	
	AABB3 bounds = GetPhysicsBounds();
	
	// Check slightly below the bottom of the entity
	Vec3 checkPos = m_position;
	checkPos.z -= GROUND_CHECK_DISTANCE;
	
	AABB3 checkBounds;
	checkBounds.m_mins = checkPos + m_physicsBody.m_mins;
	checkBounds.m_maxs = checkPos + m_physicsBody.m_maxs;
	
	// Check all 4 bottom corners
	Vec3 corners[4] = {
		Vec3(checkBounds.m_mins.x, checkBounds.m_mins.y, checkBounds.m_mins.z),
		Vec3(checkBounds.m_maxs.x, checkBounds.m_mins.y, checkBounds.m_mins.z),
		Vec3(checkBounds.m_mins.x, checkBounds.m_maxs.y, checkBounds.m_mins.z),
		Vec3(checkBounds.m_maxs.x, checkBounds.m_maxs.y, checkBounds.m_mins.z)
	};
	
	for (int i = 0; i < 4; i++)
	{
		int blockX = (int)floorf(corners[i].x);
		int blockY = (int)floorf(corners[i].y);
		int blockZ = (int)floorf(corners[i].z);

		Block block = world->GetBlockAtWorldCoords(blockX, blockY, blockZ);
		if (IsSolid(block.m_typeIndex))
		{
			return true;
		}
	}
	
	return false;
}

bool Entity::IsCollidingWithWorld() const
{
	World* world = m_game->m_currentWorld;
	if (!world)
		return false;
	
	AABB3 bounds = GetPhysicsBounds();
	int minX = (int)floorf(bounds.m_mins.x);
	int minY = (int)floorf(bounds.m_mins.y);
	int minZ = (int)floorf(bounds.m_mins.z);
	int maxX = (int)ceilf(bounds.m_maxs.x);
	int maxY = (int)ceilf(bounds.m_maxs.y);
	int maxZ = (int)ceilf(bounds.m_maxs.z);
	
	for (int x = minX; x <= maxX; x++)
	{
		for (int y = minY; y <= maxY; y++)
		{
			for (int z = minZ; z <= maxZ; z++)
			{
				Block block = world->GetBlockAtWorldCoords(x, y, z);
				if (IsSolid(block.m_typeIndex))
				{
					AABB3 blockBounds(
						Vec3((float)x, (float)y, (float)z),
						Vec3((float)(x + 1), (float)(y + 1), (float)(z + 1))
					);
					
					if (DoAABBsOverlap3D(bounds, blockBounds))
					{
						return true;
					}
				}
			}
		}
	}
	
	return false;
}

void Entity::PushOutOfBlocksX()
{
	World* world = m_game->m_currentWorld;
	if (!world)
		return;
	
	AABB3 bounds = GetPhysicsBounds();
	
	// Determine push direction based on velocity
	if (m_velocity.x > 0.0f)
	{
		// Moving right, push left
		int blockX = (int)floorf(bounds.m_maxs.x);
		float blockLeft = (float)blockX;
		float pushAmount = blockLeft - bounds.m_maxs.x - COLLISION_EPSILON;
		m_position.x += pushAmount;
	}
	else if (m_velocity.x < 0.0f)
	{
		// Moving left, push right
		int blockX = (int)floorf(bounds.m_mins.x);
		float blockRight = (float)(blockX + 1);
		float pushAmount = blockRight - bounds.m_mins.x + COLLISION_EPSILON;
		m_position.x += pushAmount;
	}
}

void Entity::PushOutOfBlocksY()
{
	World* world = m_game->m_currentWorld;
	if (!world)
		return;
	
	AABB3 bounds = GetPhysicsBounds();
	
	if (m_velocity.y > 0.0f)
	{
		// Moving north, push south
		int blockY = (int)floorf(bounds.m_maxs.y);
		float blockSouth = (float)blockY;
		float pushAmount = blockSouth - bounds.m_maxs.y - COLLISION_EPSILON;
		m_position.y += pushAmount;
	}
	else if (m_velocity.y < 0.0f)
	{
		// Moving south, push north
		int blockY = (int)floorf(bounds.m_mins.y);
		float blockNorth = (float)(blockY + 1);
		float pushAmount = blockNorth - bounds.m_mins.y + COLLISION_EPSILON;
		m_position.y += pushAmount;
	}
}

void Entity::PushOutOfBlocksZ()
{
	World* world = m_game->m_currentWorld;
	if (!world)
		return;
	
	AABB3 bounds = GetPhysicsBounds();
	
	if (m_velocity.z > 0.0f)
	{
		// Moving up, push down
		int blockZ = (int)floorf(bounds.m_maxs.z);
		float blockBottom = (float)blockZ;
		float pushAmount = blockBottom - bounds.m_maxs.z - COLLISION_EPSILON;
		m_position.z += pushAmount;
	}
	else if (m_velocity.z < 0.0f)
	{
		// Moving down, push up
		int blockZ = (int)floorf(bounds.m_mins.z);
		float blockTop = (float)(blockZ + 1);
		float pushAmount = blockTop - bounds.m_mins.z + COLLISION_EPSILON;
		m_position.z += pushAmount;
	}
}

Vec3 Entity::GetEyePosition() const
{
	return m_position + Vec3(0.0f, 0.0f, m_eyeHeight);
}

AABB3 Entity::GetPhysicsBounds() const
{
	AABB3 worldBounds;
	worldBounds.m_mins = m_position + m_physicsBody.m_mins;
	worldBounds.m_maxs = m_position + m_physicsBody.m_maxs;
	return worldBounds;
}

Vec3 Entity::GetForwardVector() const
{
	Vec3 forward, left, up;
	m_orientation.GetAsVectors_IFwd_JLeft_KUp(forward, left, up);
	// float yawRadians = m_orientation.m_yawDegrees * (3.14159265f / 180.0f);
	// float pitchRadians = m_orientation.m_pitchDegrees * (3.14159265f / 180.0f);
	//
	// float cosPitch = cosf(pitchRadians);
	// Vec3 forward;
	// forward.x = cosPitch * cosf(yawRadians);
	// forward.y = cosPitch * sinf(yawRadians);
	// forward.z = -sinf(pitchRadians);
	
	return forward;
}

Vec3 Entity::GetLeftVector() const
{
	float yawRadians = m_orientation.m_yawDegrees * (3.14159265f / 180.0f);
	
	Vec3 left;
	left.x = -sinf(yawRadians);
	left.y = cosf(yawRadians);
	left.z = 0.0f;
	
	return left;
}