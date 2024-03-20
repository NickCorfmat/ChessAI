#include "Game.h"
#include "Bit.h"
#include "BitHolder.h"
#include "Turn.h"
#include "../Application.h"

#include <cmath>
#include <thread>
#include <chrono>

Game::Game()
{
	_gameOptions.AIPlayer = false;
	_gameOptions.HumanPlayer = false;
	_gameOptions.AIPlaying = false;
	_gameOptions.currentTurnNo = 0;
	_gameOptions.gameNumber = -1;
	_gameOptions.numberOfPlayers = 0;
	_gameOptions.rowX = 0;
	_gameOptions.rowY = 0;
	_gameOptions.score = 0;
	_gameOptions.AIDepthSearches = 0;
	
	_table = nullptr;
	_winner = nullptr;
	_lastMove = "";
	// everything else
	_dragBit = nullptr;
	_dragMoved = false;
	_dropTarget = nullptr;
	_oldHolder = nullptr;
	_dragStartPos = ImVec2(0,0);
	_dragOffset = ImVec2(0,0);
	_oldPos = ImVec2(0,0);
}


Game::~Game()
{
	for (auto & _turn : _turns) {
		delete _turn;
	}
	_turns.clear();
	for (auto & _player : _players) {
		delete _player;
	}
	_players.clear();

	_gameOptions.score = 0;
	_table = nullptr;
	_winner = nullptr;
	_gameOptions.currentTurnNo = 0;
	_lastMove = "";
}

void Game::setNumberOfPlayers(unsigned int n)
{
	_players.clear();
	for (unsigned int i = 1; i <= n; i++)
	{
		Player *player = Player::initWithGame(this);
		player->setName( "Player" );
		player->setPlayerNumber(i-1);			// player numbers are zero-based
		_players.push_back(player);
	}
	_winner = nullptr;

	_gameOptions.gameNumber = 0;
	_gameOptions.numberOfPlayers = n;

	Turn *turn = Turn::initStartOfGame(this);
	_turns.clear();
	_turns.push_back(turn);
}

void Game::setAIPlayer(unsigned int playerNumber)
{
	_players.at(playerNumber)->setAIPlayer(true);
	_gameOptions.AIPlayer = playerNumber;
}

void Game::startGame()
{
	std::string startState = stateString();
	Turn *turn = _turns.at(0);
	turn->_boardState = startState;
	turn->_gameNumber = _gameOptions.gameNumber;
	_gameOptions.currentTurnNo = 0;
}

void Game::endTurn()
{
	_gameOptions.currentTurnNo++;
	std::string startState = stateString();
	Turn *turn = new Turn;
	turn->_boardState = stateString();
	turn->_date = (int)_gameOptions.currentTurnNo;
	turn->_score = _gameOptions.score;
	turn->_gameNumber = _gameOptions.gameNumber;
	_turns.push_back(turn);
	ClassGame::EndOfTurn();
}

void Game::findDropTarget(ImVec2& pos)
{
	for (int y=0; y<_gameOptions.rowY; y++) {
		for (int x=0; x<_gameOptions.rowX; x++) {
			BitHolder &holder = getHolderAt(x,y);
			if( &holder == _oldHolder ) {
				continue;
			}
			if (holder.isMouseOver(pos)) {
				if( _dropTarget && &holder != _dropTarget ) {
					_dropTarget->willNotDropBit(_dragBit);
					_dropTarget->setHighlighted(false);
					_dropTarget = nullptr;
				}

				if (holder.canDropBitAtPoint(_dragBit, pos) && canBitMoveFromTo(*_dragBit, *_oldHolder, holder)) {
					_dropTarget = &holder;
					_dropTarget->setHighlighted(true);
				}
			}
		}
	}
}

void Game::bitMovedFromTo(Bit& bit, BitHolder& src, BitHolder& dst)
{
	endTurn();
}

Bit* Game::bitToPlaceInHolder(BitHolder& holder)
{
	return nullptr;
}

bool Game::actionForEmptyHolder(BitHolder &holder)
{
	return false;
}

bool Game::clickedBit(Bit& bit)
{
	return true;
}

void Game::animateAndPlaceBitFromTo(Bit& bit, BitHolder& src, BitHolder& dst) {
	ImVec2 startPos = src.getPosition();
    ImVec2 endPos = dst.getPosition();

	float stepX = ((float)(endPos.x) - (float)(startPos.x)) / (float)(1000);
	float stepY = ((float)(endPos.y) - (float)(startPos.y)) / (float)(1000);

	for (float x = startPos.x; x < endPos.x - stepX; x += stepX) {
		for (float y = startPos.y; y < endPos.y - stepY; y += stepY) {
			bit.setPosition({y, x});
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
	}
}

bool Game::gameHasAI()
{
    return true;
}

void Game::updateAI()
{
}

void Game::mouseDown( ImVec2& location, Entity* entity ) 
{
    bool placing = false;
    _dragStartPos = location;
	if (entity && entity->getEntityType() == Entity::EntityBit) {
	    _dragBit = (Bit *)entity;
	}

    if( ! _dragBit ) {
		if (entity && entity->getEntityType() == Entity::EntityBitHolder) {
			BitHolder *holder = (BitHolder *)entity;
			if( holder ) {
				_dragBit = bitToPlaceInHolder(*holder);	
				if( _dragBit ) {
					_dragOffset.x = _dragOffset.y = 0;
					_dragBit->setPosition( _dragStartPos );
					placing = true;
				}
			}
		}
    }

    if( ! _dragBit ) {
        return;
    }
    
    // Clicked on a Bit:
    _dragMoved = false;
    _dropTarget = nullptr;
    _oldHolder = _dragBit->getHolder();
    // Ask holder's and game's permission before dragging:
    if( _oldHolder ) 
	{
        _dragBit = _oldHolder->canDragBit(_dragBit);
        if (_dragBit && !(canBitMoveFrom(*_dragBit, *_oldHolder)))
		{
			_oldHolder->cancelDragBit(_dragBit);
            _dragBit = nullptr;
        }
        if( ! _dragBit ) {
            _oldHolder = nullptr;
            return;
        }
    }
    // Start dragging:
    _oldPos = _dragBit->getPosition();
    if (_dragBit) _dragBit->setPickedUp(true);
    
    if( placing && _dragBit) {
		_dragBit->setCenterPosition( _dragStartPos );      // animate Bit to new position
        _dragMoved = true;
		findDropTarget(_dragStartPos);
    }
}

void Game::mouseMoved( ImVec2& location, Entity* entity )
{
    if( _dragBit ) {
        // Get the mouse position, and see if we've moved 3 pixels since the mouseDown:
		ImVec2 pos = location;
        if( fabs(pos.x-_dragStartPos.x)>=3 || fabs(pos.y-_dragStartPos.y)>=3 )
            _dragMoved = true;
        
        // Move the _dragBit (without animation -- it's unnecessary and slows down responsiveness):
        pos.x += _dragOffset.x;
        pos.y += _dragOffset.y;
        
        _dragBit->setCenterPosition(pos);

        // Find what it's over:
		findDropTarget(pos);
    }
}

void Game::mouseUp( ImVec2& location, Entity* entity )
{
    if (!_dragBit) {
        // If no bit was clicked, see if it's a BitHolder the game will let the user add a Bit to:
		if (entity && entity->getEntityType() == Entity::EntityBitHolder) {
			BitHolder *holder = (BitHolder *)entity;
			if (actionForEmptyHolder(*holder)) {
				_dropTarget = nullptr;
				_dragBit = nullptr;
				return;
			}
        }
    }
    if( _dragBit ) {
        if( _dragMoved ) {
            // Update the drag tracking to the final mouse position:
			mouseMoved(location, entity);
			if (_dropTarget) _dropTarget->setHighlighted(false);
			if (_dragBit) _dragBit->setPickedUp(false);
			clearBoardHighlights();

            // Is the move legal?
            if( _dropTarget && _dropTarget->dropBitAtPoint(_dragBit, _dragBit->getPosition())) {
                // Yes, notify the interested parties:
				if (_dragBit) _dragBit->setMoving(_oldHolder->getPosition(), _dropTarget->getPosition());
				if (_oldHolder) _oldHolder->draggedBitTo(_dragBit, _dropTarget);
				bitMovedFromTo( *_dragBit, *_oldHolder, *_dropTarget);
            } else {
                // Nope, cancel:
				if( _dropTarget ) _dropTarget->willNotDropBit(_dragBit);
				_dragBit->setPosition( _oldPos );
            }
        } else {
            // Just a click, without a drag:
			if (_dropTarget) _dropTarget->setHighlighted(false);
			if (_dragBit) _dragBit->setPickedUp(false);
			if( _oldHolder ) _oldHolder->cancelDragBit(_dragBit);
			_dragBit->setPosition( _oldPos );
			clickedBit(*_dragBit);
        }
		_dropTarget = nullptr;
		_dragBit = nullptr;
    }
}

void Game::clearBoardHighlights()
{
}