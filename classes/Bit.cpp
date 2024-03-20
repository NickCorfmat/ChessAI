
#include "Bit.h"
#include "BitHolder.h"
#include <iostream>


Bit::~Bit()
{
}

BitHolder* Bit::getHolder()
{
	// Look for my nearest ancestor that's a BitHolder:
	for (Entity *layer = getParent(); layer; layer = layer->getParent()) {
		if (layer->getEntityType() == EntityBitHolder)
			return (BitHolder *)layer;
		else if (layer->getEntityType() == EntityBit)
			return nullptr;
	}
	return nullptr;
}

void Bit::setPickedUp(bool up)
{
	if (up != _pickedUp) {
		float opacity = 0.0f;
		float scale = 1.0f;
		float rotation = 0.0f;
		int z;

		if (up) {
			opacity = kPickedUpOpacity;
			z = bitz::kPickupUpZ;
			_restingZ = getLocalZOrder();
			_restingTransform = getRotation();
			scale = kPickedUpScale;
		}
		else {
			opacity = 1.0f;
			z = getLocalZOrder();// _restingZ;
			if (z == bitz::kPickupUpZ) {
				z = _restingZ;
			}
			rotation = _restingTransform;
			scale = 1.0f;
		}
		setScale(scale);					// todo: animate this
		setLocalZOrder(z);
		setOpacity( opacity );
		setRotation( rotation );
		_pickedUp = up;
	}
}

void Bit::setMoving(const ImVec2 &srcPos, const ImVec2 &dstPos) {
	_isMoving = true;
	_moveFrom = srcPos;
	_moveTo = dstPos;
	ImVec2 delta = ImVec2(_moveTo.x - _moveFrom.x, _moveTo.y - _moveFrom.y);
	_directionStep = ImVec2(delta.x * 0.05f, delta.y * 0.05f);
}

void Bit::animateMove() {
	if (!_isMoving) {
		return;
	}

	ImVec2 delta = ImVec2(_moveTo.x - _moveFrom.x, _moveTo.y - _moveFrom.y);

	if (fabs(delta.x) >= 0.1f || fabs(delta.y) > 0.1f) {
		_moveFrom.x += _directionStep.x;
		_moveFrom.y += _directionStep.y;
		ImVec2 newPosition = ImVec2(_moveFrom.x, _moveFrom.y);
		setPosition(newPosition);
	} else {
		setPosition(_moveTo);
		_isMoving = false;
	}
}

bool Bit::isMoving() {
	return _isMoving;
}

bool Bit::friendly()
{
	return true;
}

bool Bit::unfriendly()
{
	return !friendly();
}

bool Bit::getPickedUp()
{
	return _pickedUp;
}

Player* Bit::getOwner()
{
	return _owner;
}

