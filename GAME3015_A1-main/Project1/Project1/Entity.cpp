//***************************************************************************************
// Entity.cpp
// by Zijie Wang and Wanhao Sun
//***************************************************************************************
#include "Entity.hpp"

// The constructor initializes an Entity object with a given State pointer and a zero velocity
Entity::Entity(State* state) : SceneNode(state), mVelocity(0, 0, 0)
{
}

// Sets the velocity of the Entity object to the given XMFLOAT3 velocity vector
void Entity::setVelocity(XMFLOAT3 velocity)
{
	mVelocity = velocity;
}

// Sets the velocity of the Entity object to the given x, y, and z velocity components
void Entity::setVelocity(float vx, float vy, float vz)
{
	mVelocity.x = vx;
	mVelocity.y = vy;
	mVelocity.z = vz;
}

// Returns the velocity of the Entity object
XMFLOAT3 Entity::getVelocity() const
{
	return mVelocity;
}

// Adds the given XMFLOAT3 velocity vector to the current velocity of the Entity object
void Entity::accelerate(XMFLOAT3 velocity)
{
	mVelocity.x = mVelocity.x + velocity.x;
	mVelocity.y = mVelocity.y + velocity.y;
	mVelocity.z = mVelocity.z + velocity.z;
}

// Adds the given x, y, and z velocity components to the current velocity of the Entity objecT
void Entity::accelerate(float vx, float vy, float vz)
{
	mVelocity.x = mVelocity.x + vx;
	mVelocity.y = mVelocity.y + vy;
	mVelocity.z = mVelocity.z + vz;
}

// Updates the current position of the Entity object based on its current velocity and the given GameTimer object
void Entity::updateCurrent(const GameTimer& gt) 
{
	XMFLOAT3 mV;
	mV.x = mVelocity.x * gt.DeltaTime();
	mV.y = mVelocity.y * gt.DeltaTime();
	mV.z = mVelocity.z * gt.DeltaTime();

	move(mV.x, mV.y, mV.z);

	renderer->World = getWorldTransform();
	renderer->NumFramesDirty++;
}
