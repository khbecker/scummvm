/* ScummVM - Graphic Adventure Engine
*
* ScummVM is the legal property of its developers, whose names
* are too numerous to list here. Please refer to the COPYRIGHT
* file distributed with this source distribution.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.

* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.

* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*/

#include "common/scummsys.h"

#include "zvision/script_manager.h"

#include "zvision/zvision.h"
#include "zvision/render_manager.h"
#include "zvision/cursor_manager.h"
#include "zvision/save_manager.h"
#include "zvision/actions.h"
#include "zvision/utility.h"

#include "common/algorithm.h"
#include "common/hashmap.h"
#include "common/debug.h"
#include "common/stream.h"


namespace ZVision {

ScriptManager::ScriptManager(ZVision *engine)
	: _engine(engine),
	  _currentlyFocusedControl(0),
	  _activeControls(NULL) {
}

ScriptManager::~ScriptManager() {
	cleanScriptScope(universe);
	cleanScriptScope(world);
	cleanScriptScope(room);
	cleanScriptScope(nodeview);
}

void ScriptManager::initialize() {
	cleanScriptScope(universe);
	cleanScriptScope(world);
	cleanScriptScope(room);
	cleanScriptScope(nodeview);

	_currentLocation.node = '0';
	_currentLocation.world = '0';
	_currentLocation.room = '0';
	_currentLocation.view = '0';

	parseScrFile("universe.scr", universe);
	changeLocation('g', 'a', 'r', 'y', 0);
}

void ScriptManager::update(uint deltaTimeMillis) {
	if (_currentLocation.node != _nextLocation.node ||
	        _currentLocation.room != _nextLocation.room ||
	        _currentLocation.view != _nextLocation.view ||
	        _currentLocation.world != _nextLocation.world)
		do_changeLocation();

	updateNodes(deltaTimeMillis);
	execScope(nodeview);
	execScope(room);
	execScope(world);
	execScope(universe);
	updateControls(deltaTimeMillis);
}

void ScriptManager::execScope(script_scope &scope) {
	// Swap queues
	PuzzleList *tmp = scope.exec_queue;
	scope.exec_queue = scope.scope_queue;
	scope.scope_queue = tmp;

	for (PuzzleList::iterator PuzzleIter = scope._puzzles.begin(); PuzzleIter != scope._puzzles.end(); ++PuzzleIter)
		(*PuzzleIter)->addedBySetState = 0;

	if (scope.proc_count < 2 || getStateValue(76)) {
		for (PuzzleList::iterator PuzzleIter = scope._puzzles.begin(); PuzzleIter != scope._puzzles.end(); ++PuzzleIter)
			checkPuzzleCriteria(*PuzzleIter, scope.proc_count);
	} else {
		for (PuzzleList::iterator PuzzleIter = scope.exec_queue->begin(); PuzzleIter != scope.exec_queue->end(); ++PuzzleIter)
			checkPuzzleCriteria(*PuzzleIter, scope.proc_count);
	}

	scope.exec_queue->clear();

	if (scope.proc_count < 2) {
		scope.proc_count++;
	}
}

void ScriptManager::referenceTableAddPuzzle(uint32 key, puzzle_ref ref) {
	if (_referenceTable.contains(key)) {
		Common::Array<puzzle_ref> *arr = &_referenceTable[key];
		for (uint32 i = 0; i < arr->size(); i++)
			if ((*arr)[i].puz == ref.puz)
				return;
	}

	_referenceTable[key].push_back(ref);
}

void ScriptManager::addPuzzlesToReferenceTable(script_scope &scope) {
	// Iterate through each local Puzzle
	for (PuzzleList::iterator PuzzleIter = scope._puzzles.begin(); PuzzleIter != scope._puzzles.end(); ++PuzzleIter) {
		Puzzle *puzzlePtr = (*PuzzleIter);

		puzzle_ref ref;
		ref.scope = &scope;
		ref.puz = puzzlePtr;

		referenceTableAddPuzzle(puzzlePtr->key, ref);

		// Iterate through each CriteriaEntry and add a reference from the criteria key to the Puzzle
		for (Common::List<Common::List<Puzzle::CriteriaEntry> >::iterator criteriaIter = (*PuzzleIter)->criteriaList.begin(); criteriaIter != (*PuzzleIter)->criteriaList.end(); ++criteriaIter)
			for (Common::List<Puzzle::CriteriaEntry>::iterator entryIter = criteriaIter->begin(); entryIter != criteriaIter->end(); ++entryIter)
				referenceTableAddPuzzle(entryIter->key, ref);
	}
}

void ScriptManager::updateNodes(uint deltaTimeMillis) {
	// If process() returns true, it means the node can be deleted
	for (SideFXList::iterator iter = _activeSideFx.begin(); iter != _activeSideFx.end();) {
		if ((*iter)->process(deltaTimeMillis)) {
			delete(*iter);
			// Remove the node
			iter = _activeSideFx.erase(iter);
		} else {
			++iter;
		}
	}
}

void ScriptManager::updateControls(uint deltaTimeMillis) {
	if (!_activeControls)
		return;
	for (ControlList::iterator iter = _activeControls->begin(); iter != _activeControls->end(); iter++)
		(*iter)->process(deltaTimeMillis);
}

void ScriptManager::checkPuzzleCriteria(Puzzle *puzzle, uint counter) {
	// Check if the puzzle is already finished
	// Also check that the puzzle isn't disabled
	if (getStateValue(puzzle->key) == 1 &&
	        (getStateFlag(puzzle->key) & Puzzle::DISABLED) == 0) {
		return;
	}

	// Check each Criteria
	if (counter == 0 && (getStateFlag(puzzle->key) & Puzzle::DO_ME_NOW) == 0)
		return;

	bool criteriaMet = false;
	for (Common::List<Common::List<Puzzle::CriteriaEntry> >::iterator criteriaIter = puzzle->criteriaList.begin(); criteriaIter != puzzle->criteriaList.end(); ++criteriaIter) {
		criteriaMet = false;

		for (Common::List<Puzzle::CriteriaEntry>::iterator entryIter = criteriaIter->begin(); entryIter != criteriaIter->end(); ++entryIter) {
			// Get the value to compare against
			int argumentValue;
			if (entryIter->argumentIsAKey)
				argumentValue = getStateValue(entryIter->argument);
			else
				argumentValue = entryIter->argument;

			// Do the comparison
			switch (entryIter->criteriaOperator) {
			case Puzzle::EQUAL_TO:
				criteriaMet = getStateValue(entryIter->key) == argumentValue;
				break;
			case Puzzle::NOT_EQUAL_TO:
				criteriaMet = getStateValue(entryIter->key) != argumentValue;
				break;
			case Puzzle::GREATER_THAN:
				criteriaMet = getStateValue(entryIter->key) > argumentValue;
				break;
			case Puzzle::LESS_THAN:
				criteriaMet = getStateValue(entryIter->key) < argumentValue;
				break;
			}

			// If one check returns false, don't keep checking
			if (!criteriaMet) {
				break;
			}
		}

		// If any of the Criteria are *fully* met, then execute the results
		if (criteriaMet) {
			break;
		}
	}

	// criteriaList can be empty. Aka, the puzzle should be executed immediately
	if (puzzle->criteriaList.empty() || criteriaMet) {
		debug(1, "Puzzle %u criteria passed. Executing its ResultActions", puzzle->key);

		// Set the puzzle as completed
		setStateValue(puzzle->key, 1);

		bool shouldContinue = true;
		for (Common::List<ResultAction *>::iterator resultIter = puzzle->resultActions.begin(); resultIter != puzzle->resultActions.end(); ++resultIter) {
			shouldContinue = shouldContinue && (*resultIter)->execute();
			if (!shouldContinue) {
				break;
			}
		}

		if (!shouldContinue) {
			return;
		}
	}
}

void ScriptManager::cleanStateTable() {
	for (StateMap::iterator iter = _globalState.begin(); iter != _globalState.end(); ++iter) {
		// If the value is equal to zero, we can purge it since getStateValue()
		// will return zero if _globalState doesn't contain a key
		if (iter->_value == 0) {
			// Remove the node
			_globalState.erase(iter);
		}
	}
}

void ScriptManager::cleanScriptScope(script_scope &scope) {
	scope._priv_queue_one.clear();
	scope._priv_queue_two.clear();
	scope.scope_queue = &scope._priv_queue_one;
	scope.exec_queue = &scope._priv_queue_two;
	for (PuzzleList::iterator iter = scope._puzzles.begin(); iter != scope._puzzles.end(); ++iter)
		delete(*iter);

	scope._puzzles.clear();

	for (ControlList::iterator iter = scope._controls.begin(); iter != scope._controls.end(); ++iter)
		delete(*iter);

	scope._controls.clear();

	scope.proc_count = 0;
}

int ScriptManager::getStateValue(uint32 key) {
	if (_globalState.contains(key))
		return _globalState[key];
	else
		return 0;
}

void ScriptManager::queuePuzzles(uint32 key) {
	if (_referenceTable.contains(key)) {
		Common::Array<puzzle_ref> *arr = &_referenceTable[key];
		for (int32 i = arr->size() - 1; i >= 0; i--)
			if (!(*arr)[i].puz->addedBySetState) {
				(*arr)[i].scope->scope_queue->push_back((*arr)[i].puz);
				(*arr)[i].puz->addedBySetState = true;
			}
	}
}

void ScriptManager::setStateValue(uint32 key, int value) {
	if (value == 0)
		_globalState.erase(key);
	else
		_globalState[key] = value;

	queuePuzzles(key);
}

uint ScriptManager::getStateFlag(uint32 key) {
	if (_globalStateFlags.contains(key))
		return _globalStateFlags[key];
	else
		return 0;
}

void ScriptManager::setStateFlag(uint32 key, uint value) {
	queuePuzzles(key);

	_globalStateFlags[key] |= value;
}

void ScriptManager::unsetStateFlag(uint32 key, uint value) {
	queuePuzzles(key);

	if (_globalStateFlags.contains(key)) {
		_globalStateFlags[key] &= ~value;

		if (_globalStateFlags[key] == 0)
			_globalStateFlags.erase(key);
	}
}

Control *ScriptManager::getControl(uint32 key) {

	return nullptr;
}

void ScriptManager::focusControl(uint32 key) {
	if (!_activeControls)
		return;
	for (ControlList::iterator iter = _activeControls->begin(); iter != _activeControls->end(); ++iter) {
		uint32 controlKey = (*iter)->getKey();

		if (controlKey == key) {
			(*iter)->focus();
		} else if (controlKey == _currentlyFocusedControl) {
			(*iter)->unfocus();
		}
	}

	_currentlyFocusedControl = key;
}

void ScriptManager::addSideFX(SideFX *fx) {
	_activeSideFx.push_back(fx);
}

SideFX *ScriptManager::getSideFX(uint32 key) {
	for (SideFXList::iterator iter = _activeSideFx.begin(); iter != _activeSideFx.end(); ++iter) {
		if ((*iter)->getKey() == key) {
			return (*iter);
		}
	}

	return nullptr;
}

void ScriptManager::deleteSideFx(uint32 key) {
	for (SideFXList::iterator iter = _activeSideFx.begin(); iter != _activeSideFx.end(); ++iter) {
		if ((*iter)->getKey() == key) {
			delete(*iter);
			_activeSideFx.erase(iter);
			break;
		}
	}
}

void ScriptManager::stopSideFx(uint32 key) {
	for (SideFXList::iterator iter = _activeSideFx.begin(); iter != _activeSideFx.end(); ++iter) {
		if ((*iter)->getKey() == key) {
			bool ret = (*iter)->stop();
			if (ret) {
				delete(*iter);
				_activeSideFx.erase(iter);
			}
			break;
		}
	}
}

void ScriptManager::killSideFx(uint32 key) {
	for (SideFXList::iterator iter = _activeSideFx.begin(); iter != _activeSideFx.end(); ++iter) {
		if ((*iter)->getKey() == key) {
			(*iter)->kill();
			delete(*iter);
			_activeSideFx.erase(iter);
			break;
		}
	}
}

void ScriptManager::killSideFxType(SideFX::SideFXType type) {
	for (SideFXList::iterator iter = _activeSideFx.begin(); iter != _activeSideFx.end();) {
		if ((*iter)->getType() & type) {
			(*iter)->kill();
			delete(*iter);
			_activeSideFx.erase(iter);
		} else {
			++iter;
		}
	}
}

void ScriptManager::onMouseDown(const Common::Point &screenSpacePos, const Common::Point &backgroundImageSpacePos) {
	if (!_activeControls)
		return;
	for (ControlList::iterator iter = _activeControls->reverse_begin(); iter != _activeControls->end(); iter--) {
		if ((*iter)->onMouseDown(screenSpacePos, backgroundImageSpacePos))
			return;
	}
}

void ScriptManager::onMouseUp(const Common::Point &screenSpacePos, const Common::Point &backgroundImageSpacePos) {
	if (!_activeControls)
		return;
	for (ControlList::iterator iter = _activeControls->reverse_begin(); iter != _activeControls->end(); iter--) {
		if ((*iter)->onMouseUp(screenSpacePos, backgroundImageSpacePos))
			return;
	}
}

bool ScriptManager::onMouseMove(const Common::Point &screenSpacePos, const Common::Point &backgroundImageSpacePos) {
	if (!_activeControls)
		return false;

	for (ControlList::iterator iter = _activeControls->reverse_begin(); iter != _activeControls->end(); iter--) {
		if ((*iter)->onMouseMove(screenSpacePos, backgroundImageSpacePos))
			return true;
	}

	return false;
}

void ScriptManager::onKeyDown(Common::KeyState keyState) {
	if (!_activeControls)
		return;
	for (ControlList::iterator iter = _activeControls->begin(); iter != _activeControls->end(); ++iter) {
		(*iter)->onKeyDown(keyState);
	}
}

void ScriptManager::onKeyUp(Common::KeyState keyState) {
	if (!_activeControls)
		return;
	for (ControlList::iterator iter = _activeControls->begin(); iter != _activeControls->end(); ++iter) {
		(*iter)->onKeyUp(keyState);
	}
}

void ScriptManager::changeLocation(char _world, char _room, char _node, char _view, uint32 offset) {
	_nextLocation.world = _world;
	_nextLocation.room = _room;
	_nextLocation.node = _node;
	_nextLocation.view = _view;
	_nextLocation.offset = offset;
	// If next location 0000 - it's indicate to go to previous location.
	if (_nextLocation.world == '0' && _nextLocation.room == '0' && _nextLocation.node == '0' && _nextLocation.view == '0') {
		if (getStateValue(StateKey_World) != 'g' || getStateValue(StateKey_Room) != 'j') {
			_nextLocation.world = getStateValue(StateKey_LastWorld);
			_nextLocation.room = getStateValue(StateKey_LastRoom);
			_nextLocation.node = getStateValue(StateKey_LastNode);
			_nextLocation.view = getStateValue(StateKey_LastView);
			_nextLocation.offset = getStateValue(StateKey_LastViewPos);
		} else {
			_nextLocation.world = getStateValue(StateKey_Menu_LastWorld);
			_nextLocation.room = getStateValue(StateKey_Menu_LastRoom);
			_nextLocation.node = getStateValue(StateKey_Menu_LastNode);
			_nextLocation.view = getStateValue(StateKey_Menu_LastView);
			_nextLocation.offset = getStateValue(StateKey_Menu_LastViewPos);
		}
	}
}

void ScriptManager::do_changeLocation() {
	assert(_nextLocation.world != 0);
	debug(1, "Changing location to: %c %c %c %c %u", _nextLocation.world, _nextLocation.room, _nextLocation.node, _nextLocation.view, _nextLocation.offset);

	if (getStateValue(StateKey_World) != 'g' || getStateValue(StateKey_Room) != 'j') {
		if (_nextLocation.world != 'g' || _nextLocation.room != 'j') {
			setStateValue(StateKey_LastWorld, getStateValue(StateKey_World));
			setStateValue(StateKey_LastRoom, getStateValue(StateKey_Room));
			setStateValue(StateKey_LastNode, getStateValue(StateKey_Node));
			setStateValue(StateKey_LastView, getStateValue(StateKey_View));
			setStateValue(StateKey_LastViewPos, getStateValue(StateKey_ViewPos));
		} else {
			setStateValue(StateKey_Menu_LastWorld, getStateValue(StateKey_World));
			setStateValue(StateKey_Menu_LastRoom, getStateValue(StateKey_Room));
			setStateValue(StateKey_Menu_LastNode, getStateValue(StateKey_Node));
			setStateValue(StateKey_Menu_LastView, getStateValue(StateKey_View));
			setStateValue(StateKey_Menu_LastViewPos, getStateValue(StateKey_ViewPos));
		}
	}

	// Auto save
	//_engine->getSaveManager()->autoSave();

	setStateValue(StateKey_World, _nextLocation.world);
	setStateValue(StateKey_Room, _nextLocation.room);
	setStateValue(StateKey_Node, _nextLocation.node);
	setStateValue(StateKey_View, _nextLocation.view);
	setStateValue(StateKey_ViewPos, _nextLocation.offset);

	// Clear all the containers
	_referenceTable.clear();
	cleanScriptScope(nodeview);
	cleanScriptScope(room);
	cleanScriptScope(world);

	addPuzzlesToReferenceTable(universe);

	// Parse into puzzles and controls
	Common::String fileName = Common::String::format("%c%c%c%c.scr", _nextLocation.world, _nextLocation.room, _nextLocation.node, _nextLocation.view);
	parseScrFile(fileName, nodeview);
	addPuzzlesToReferenceTable(nodeview);

	fileName = Common::String::format("%c%c.scr", _nextLocation.world, _nextLocation.room);
	parseScrFile(fileName, room);
	addPuzzlesToReferenceTable(room);

	fileName = Common::String::format("%c.scr", _nextLocation.world);
	parseScrFile(fileName, world);
	addPuzzlesToReferenceTable(world);

	_activeControls = &nodeview._controls;

	// Revert to the idle cursor
	_engine->getCursorManager()->revertToIdle();

	// Reset the background velocity
	_engine->getRenderManager()->setBackgroundVelocity(0);

	// Remove any alphaEntries
	_engine->getRenderManager()->clearAlphaEntries();

	// Change the background position
	_engine->getRenderManager()->setBackgroundPosition(_nextLocation.offset);

	// Update _currentLocation
	_currentLocation = _nextLocation;

	execScope(room);
	execScope(nodeview);
}

void ScriptManager::serializeStateTable(Common::WriteStream *stream) {
	// Write the number of state value entries
	stream->writeUint32LE(_globalState.size());

	for (StateMap::iterator iter = _globalState.begin(); iter != _globalState.end(); ++iter) {
		// Write out the key/value pair
		stream->writeUint32LE(iter->_key);
		stream->writeUint32LE(iter->_value);
	}
}

void ScriptManager::deserializeStateTable(Common::SeekableReadStream *stream) {
	// Clear out the current table values
	_globalState.clear();

	// Read the number of key/value pairs
	uint32 numberOfPairs = stream->readUint32LE();

	for (uint32 i = 0; i < numberOfPairs; ++i) {
		uint32 key = stream->readUint32LE();
		uint32 value = stream->readUint32LE();
		// Directly access the state table so we don't trigger Puzzle checks
		_globalState[key] = value;
	}
}

Location ScriptManager::getCurrentLocation() const {
	Location location = _currentLocation;
	location.offset = _engine->getRenderManager()->getCurrentBackgroundOffset();

	return location;
}

ValueSlot::ValueSlot(ScriptManager *sc_man, const char *slot_val):
	_sc_man(sc_man) {
	value = 0;
	slot = false;
	const char *is_slot = strstr(slot_val, "[");
	if (is_slot) {
		slot = true;
		value = atoi(is_slot + 1);
	} else {
		slot = false;
		value = atoi(slot_val);
	}
}
int16 ValueSlot::getValue() {
	if (slot) {
		if (value >= 0)
			return _sc_man->getStateValue(value);
		else
			return 0;
	} else
		return value;
}

} // End of namespace ZVision