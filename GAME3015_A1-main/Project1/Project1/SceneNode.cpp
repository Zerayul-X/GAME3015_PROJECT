//***************************************************************************************
// SceneNode.cpp
// by Zijie Wang and Wanhao Sun
//***************************************************************************************
#include "SceneNode.hpp"
#include "Game.hpp"

// Constructor
SceneNode::SceneNode(State* state)
	: mChildren()
	, mParent(nullptr)
	, mState(state)
{
	// Initialize world position, scaling, and rotation to default values
	mWorldPosition = XMFLOAT3(0, 0, 0);
	mWorldScaling = XMFLOAT3(1, 1, 1);
	mWorldRotation = XMFLOAT3(0, 0, 0);
}

// Attach a child SceneNode to the current SceneNode
void SceneNode::attachChild(Ptr child)
{
	child->mParent = this;
	mChildren.push_back(std::move(child));
}

// Detach a child SceneNode from the current SceneNode
SceneNode::Ptr SceneNode::detachChild(const SceneNode& node)
{
	auto found = std::find_if(mChildren.begin(), mChildren.end(), [&](Ptr& p) { return p.get() == &node; });
	assert(found != mChildren.end());

	Ptr result = std::move(*found);
	result->mParent = nullptr;
	mChildren.erase(found);
	return result;
}

// Update the current SceneNode and its children
void SceneNode::update(const GameTimer& gt)
{
	updateCurrent(gt);
	updateChildren(gt);
}

// Update the current SceneNode
void SceneNode::updateCurrent(const GameTimer& gt)
{

}

// Update the children of the current SceneNode
void SceneNode::updateChildren(const GameTimer& gt)
{
	for (Ptr& child : mChildren)
	{
		child->update(gt);
	}
}

// Draw the current SceneNode and its children
void SceneNode::draw() const
{
	drawCurrent();
	drawChildren();
}

// Draw the current SceneNode
void SceneNode::drawCurrent() const
{

}

// Draw the children of the current SceneNode
void SceneNode::drawChildren() const
{
	for (const Ptr& child : mChildren)
	{
		child->draw();
	}
}

// Build the current SceneNode and its children
void SceneNode::build()
{
	buildCurrent();
	buildChildren();
}

// Build the current SceneNode 
void SceneNode::buildCurrent()
{
	//Empty
}

// Build the children of the current SceneNode
void SceneNode::buildChildren()
{
	for (const Ptr& child : mChildren)
	{
		child->build();
	}
}

// Get the world position of the current SceneNode
XMFLOAT3 SceneNode::getWorldPosition() const
{
	return mWorldPosition;
}

// Set the current position
void SceneNode::setPosition(float x, float y, float z)
{
	mWorldPosition = XMFLOAT3(x, y, z);
}

// Returns the world rotation of the scene node
XMFLOAT3 SceneNode::getWorldRotation() const
{
	return mWorldRotation;
}

// Sets the world rotation of the scene node with the given X, Y, and Z values
void SceneNode::setWorldRotation(float x, float y, float z)
{
	mWorldRotation = XMFLOAT3(x, y, z);
}

// Returns the world scaling of the scene node
XMFLOAT3 SceneNode::getWorldScale() const
{
	return mWorldScaling;
}

// Sets the world scaling of the scene node with the given X, Y, and Z values
void SceneNode::setScale(float x, float y, float z)
{
	mWorldScaling = XMFLOAT3(x, y, z);
}

// Calculates the world transform of the scene node by combining the transforms of all parent nodes
XMFLOAT4X4 SceneNode::getWorldTransform() const
{
	XMFLOAT4X4 transform = MathHelper::Identity4x4();
	XMMATRIX T = XMLoadFloat4x4(&transform);

	// Traverse the parent nodes and multiply their transforms with the current transform
	for (const SceneNode* node = this; node != nullptr; node = node->mParent)
	{
		XMMATRIX Tp = XMLoadFloat4x4(&node->getTransform());
		T = Tp * T;
	}
	XMStoreFloat4x4(&transform, T);

	return transform;
}

// Calculates the local transform of the scene node based on its position, rotation, and scaling
XMFLOAT4X4 SceneNode::getTransform() const
{
	XMFLOAT4X4 transform;
	XMStoreFloat4x4(&transform, XMMatrixScaling(mWorldScaling.x, mWorldScaling.y, mWorldScaling.z) *
		XMMatrixRotationX(mWorldRotation.x) *
		XMMatrixRotationY(mWorldRotation.y) *
		XMMatrixRotationZ(mWorldRotation.z) *
		XMMatrixTranslation(mWorldPosition.x, mWorldPosition.y, mWorldPosition.z));
	return transform;
}

// Moves the scene node by the given X, Y, and Z values
void SceneNode::move(float x, float y, float z)
{
	mWorldPosition.x += x;
	mWorldPosition.y += y;
	mWorldPosition.z += z;
}

void SceneNode::onCommand(const Command& command, const GameTimer& gt)
{
	// Executes the given command on the current node if its category matches
	if (command.category & getCategory())
		command.action(*this, gt);

	// Executes the command on all child nodes
	for (Ptr& child : mChildren)
		child->onCommand(command, gt);
}

// Returns the category of the scene node
unsigned int SceneNode::getCategory() const
{
	return Category::Scene;
}
