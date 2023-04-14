//***************************************************************************************
// World.cpp
// by Zijie Wang and Wanhao Sun
//***************************************************************************************

#define NOMINMAX
#include "World.hpp"

// Constructor for World class
World::World(State* state)
	: mSceneGraph(new SceneNode(state))
	, mState(state)
	, mPlayerAircraft(nullptr)
	, mBackground(nullptr)
	, mWorldBounds(-4.25f, 4.25f, -3.0f, 3.0f) //Left, Right, Down, Up
	, mSpawnPosition(0.f, 0.f)
	, mScrollSpeed(1.0f)
{
}

// Processes commands in the command queue, updates the scene graph, and handles entity movements and rotations
void World::update(const GameTimer& gt)
{
	// Reset player velocity to 0 before processing new commands
	mPlayerAircraft->setVelocity(0, 0, 0);


	// Process all commands in the command queue
	while (!mCommandQueue.isEmpty())
		mSceneGraph->onCommand(mCommandQueue.pop(), gt);

	// Update the scene graph
	mSceneGraph->update(gt);

	// Set player rotation based on its velocity
	if (mPlayerAircraft->getVelocity().y > 0) mPlayerAircraft->setWorldRotation(-1, 0, 0);

	else if (mPlayerAircraft->getVelocity().y < 0) mPlayerAircraft->setWorldRotation(1, 0, 0);

	else if (mPlayerAircraft->getVelocity().x > 0) mPlayerAircraft->setWorldRotation(0, 0, -1);

	else if (mPlayerAircraft->getVelocity().x < 0) mPlayerAircraft->setWorldRotation(0, 0, 1);

	else if (mPlayerAircraft->getVelocity().y == 0 || mPlayerAircraft->getVelocity().x == 0)
		mPlayerAircraft->setWorldRotation(0, 0, 0);

	// Set enemy rotations based on their velocities
	for (int i = 0; i < totalEnemies; i++)
	{
		if (mEnemy[i]->getVelocity().y > 0) mEnemy[i]->setWorldRotation(-1, 135, 0);

		else if (mEnemy[i]->getVelocity().y < 0) mEnemy[i]->setWorldRotation(1, 135, 0);

		else if (mEnemy[i]->getVelocity().y == 0)
			mEnemy[i]->setWorldRotation(0, 0, 0);
	}

	// Handle player and enemy movements and bounds checking
	if (mPlayerAircraft->getWorldPosition().x < -maxWidth)
		mPlayerAircraft->setPosition(-maxWidth, mPlayerAircraft->getWorldPosition().y, mPlayerAircraft->getWorldPosition().z);

	else if (mPlayerAircraft->getWorldPosition().x > maxWidth)
		mPlayerAircraft->setPosition(maxWidth, mPlayerAircraft->getWorldPosition().y, mPlayerAircraft->getWorldPosition().z);

	if (mPlayerAircraft->getWorldPosition().y < minHeight)
		mPlayerAircraft->setPosition(mPlayerAircraft->getWorldPosition().x, minHeight, mPlayerAircraft->getWorldPosition().z);

	else if (mPlayerAircraft->getWorldPosition().y > maxHeight)
		mPlayerAircraft->setPosition(mPlayerAircraft->getWorldPosition().x, maxHeight, mPlayerAircraft->getWorldPosition().z);

	for (int i = 0; i < totalEnemies; i++)
	{
		if (mEnemy[i]->getWorldPosition().x < -maxWidth || mEnemy[i]->getWorldPosition().x > maxWidth)
		{
			mEnemy[i]->setVelocity(-mEnemy[i]->getVelocity().x, mEnemy[i]->getVelocity().y, mEnemy[i]->getVelocity().z);
		}

		if (mEnemy[i]->getWorldPosition().y < minHeight || mEnemy[i]->getWorldPosition().y > maxHeight)
		{
			mEnemy[i]->setVelocity(mEnemy[i]->getVelocity().x, -mEnemy[i]->getVelocity().y, mEnemy[i]->getVelocity().z);
		}
	}
}

// Returns the command queue for the world
CommandQueue& World::getCommandQueue()
{
	return mCommandQueue;
}

// Draws the world by drawing the scene graph
void World::draw()
{
	mSceneGraph->draw();
}

// Builds the scene by creating game objects and adding them to the scene graph
void World::buildScene()
{
	// Creates a player aircraft object, sets its properties, and adds it to the scene graph
	std::unique_ptr<Aircraft> player(new Aircraft(Aircraft::Type::Eagle, mState));
	mPlayerAircraft = player.get();
	mPlayerAircraft->setPosition(0, 0, -10);
	mPlayerAircraft->setScale(3.0, 3.0, 3.0);
	mPlayerAircraft->setVelocity(2.5f, 2.0f, 0.0f);
	mSceneGraph->attachChild(std::move(player));

	// Creates enemy aircraft objects, sets their properties, and adds them to the scene graph
	for (int i = 0; i < totalEnemies; i++)
	{
		std::unique_ptr<Aircraft> enemy1(new Aircraft(Aircraft::Type::Raptor, mState));
		mEnemy[i] = enemy1.get();
		mEnemy[i]->setPosition(mPlayerAircraft->getWorldPosition().x * i, (rand() % 5) + 5, mPlayerAircraft->getWorldPosition().z + 10.0f);
		mEnemy[i]->setScale(3.0, 3.0, 3.0);
		mEnemy[i]->setVelocity(2.f * i + i, 2.0f, 0.0f);
		mSceneGraph->attachChild(std::move(enemy1));
	}

	// Creates a background sprite object, sets its properties, and adds it to the scene graph
	std::unique_ptr<SpriteNode> backgroundSprite(new SpriteNode(mState));
	backgroundSprite->SetMatGeoDrawName("Desert", "boxGeo", "box");
	mBackground = backgroundSprite.get();
	mBackground->setPosition(0, -30.0, 0.0);
	mBackground->setScale(200.0, 1.0, 200.0);
	mBackground->setVelocity(0, 0, -mScrollSpeed);
	mBackground->setWorldRotation(20.0, 0.0, 0.0);
	mSceneGraph->attachChild(std::move(backgroundSprite));

	// Builds the scene graph hierarchy
	mSceneGraph->build();
}
