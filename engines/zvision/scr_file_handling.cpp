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

#include "zvision/utility.h"
#include "zvision/puzzle.h"
#include "zvision/actions.h"
#include "zvision/push_toggle_control.h"
#include "zvision/lever_control.h"

#include "common/textconsole.h"
#include "common/file.h"
#include "common/tokenizer.h"


namespace ZVision {

void ScriptManager::parseScrFile(const Common::String &fileName, script_scope &scope) {
	Common::File file;
	if (!file.open(fileName)) {
		warning("Script file not found: %s", fileName.c_str());
		return;
	}

	while (!file.eos()) {
		Common::String line = file.readLine();
		if (file.err()) {
			warning("Error parsing scr file: %s", fileName.c_str());
			return;
		}

		trimCommentsAndWhiteSpace(&line);
		if (line.empty())
			continue;

		if (line.matchString("puzzle:*", true)) {
			Puzzle *puzzle = new Puzzle();
			sscanf(line.c_str(), "puzzle:%u", &(puzzle->key));
			if (getStateFlag(puzzle->key) & Puzzle::ONCE_PER_INST)
				setStateValue(puzzle->key, 0);
			parsePuzzle(puzzle, file);
			scope._puzzles.push_back(puzzle);

		} else if (line.matchString("control:*", true)) {
			Control *ctrl = parseControl(line, file);
			if (ctrl)
				scope._controls.push_back(ctrl);
		}
	}
	scope.proc_count = 0;
}

void ScriptManager::parsePuzzle(Puzzle *puzzle, Common::SeekableReadStream &stream) {
	Common::String line = stream.readLine();
	trimCommentsAndWhiteSpace(&line);

	while (!stream.eos() && !line.contains('}')) {
		if (line.matchString("criteria {", true)) {
			parseCriteria(stream, puzzle->criteriaList);
		} else if (line.matchString("results {", true)) {
			parseResults(stream, puzzle->resultActions);
		} else if (line.matchString("flags {", true)) {
			setStateFlag(puzzle->key, parseFlags(stream));
		}

		line = stream.readLine();
		trimCommentsAndWhiteSpace(&line);
	}

	puzzle->addedBySetState = 0;
}

bool ScriptManager::parseCriteria(Common::SeekableReadStream &stream, Common::List<Common::List<Puzzle::CriteriaEntry> > &criteriaList) const {
	// Loop until we find the closing brace
	Common::String line = stream.readLine();
	trimCommentsAndWhiteSpace(&line);

	// Criteria can be empty
	if (line.contains('}')) {
		return false;
	}

	// Create a new List to hold the CriteriaEntries
	criteriaList.push_back(Common::List<Puzzle::CriteriaEntry>());

	while (!stream.eos() && !line.contains('}')) {
		Puzzle::CriteriaEntry entry;

		// Split the string into tokens using ' ' as a delimiter
		Common::StringTokenizer tokenizer(line);
		Common::String token;

		// Parse the id out of the first token
		token = tokenizer.nextToken();
		sscanf(token.c_str(), "[%u]", &(entry.key));

		// Parse the operator out of the second token
		token = tokenizer.nextToken();
		if (token.c_str()[0] == '=')
			entry.criteriaOperator = Puzzle::EQUAL_TO;
		else if (token.c_str()[0] == '!')
			entry.criteriaOperator = Puzzle::NOT_EQUAL_TO;
		else if (token.c_str()[0] == '>')
			entry.criteriaOperator = Puzzle::GREATER_THAN;
		else if (token.c_str()[0] == '<')
			entry.criteriaOperator = Puzzle::LESS_THAN;

		// First determine if the last token is an id or a value
		// Then parse it into 'argument'
		token = tokenizer.nextToken();
		if (token.contains('[')) {
			sscanf(token.c_str(), "[%u]", &(entry.argument));
			entry.argumentIsAKey = true;
		} else {
			sscanf(token.c_str(), "%u", &(entry.argument));
			entry.argumentIsAKey = false;
		}

		criteriaList.back().push_back(entry);

		line = stream.readLine();
		trimCommentsAndWhiteSpace(&line);
	}

	return true;
}

void ScriptManager::parseResults(Common::SeekableReadStream &stream, Common::List<ResultAction *> &actionList) const {
	// Loop until we find the closing brace
	Common::String line = stream.readLine();
	trimCommentsAndWhiteSpace(&line);

	// TODO: Re-order the if-then statements in order of highest occurrence
	while (!stream.eos() && !line.contains('}')) {
		if (line.empty()) {
			line = stream.readLine();
			trimCommentsAndWhiteSpace(&line);

			continue;
		}

		// Parse for the action type
		if (line.matchString("*:add*", true)) {
			actionList.push_back(new ActionAdd(_engine, line));
		} else if (line.matchString("*:animplay*", true)) {
			actionList.push_back(new ActionPlayAnimation(_engine, line));
		} else if (line.matchString("*:animpreload*", true)) {
			actionList.push_back(new ActionPreloadAnimation(_engine, line));
		} else if (line.matchString("*:animunload*", true)) {
			//actionList.push_back(new ActionUnloadAnimation(_engine, line));
		} else if (line.matchString("*:attenuate*", true)) {
			// TODO: Implement ActionAttenuate
		} else if (line.matchString("*:assign*", true)) {
			actionList.push_back(new ActionAssign(_engine, line));
		} else if (line.matchString("*:change_location*", true)) {
			actionList.push_back(new ActionChangeLocation(_engine, line));
		} else if (line.matchString("*:crossfade*", true)) {
			// TODO: Implement ActionCrossfade
		} else if (line.matchString("*:debug*", true)) {
			// TODO: Implement ActionDebug
		} else if (line.matchString("*:delay_render*", true)) {
			// TODO: Implement ActionDelayRender
		} else if (line.matchString("*:disable_control*", true)) {
			actionList.push_back(new ActionDisableControl(_engine, line));
		} else if (line.matchString("*:disable_venus*", true)) {
			// TODO: Implement ActionDisableVenus
		} else if (line.matchString("*:display_message*", true)) {
			// TODO: Implement ActionDisplayMessage
		} else if (line.matchString("*:dissolve*", true)) {
			// TODO: Implement ActionDissolve
		} else if (line.matchString("*:distort*", true)) {
			// TODO: Implement ActionDistort
		} else if (line.matchString("*:enable_control*", true)) {
			actionList.push_back(new ActionEnableControl(_engine, line));
		} else if (line.matchString("*:flush_mouse_events*", true)) {
			// TODO: Implement ActionFlushMouseEvents
		} else if (line.matchString("*:inventory*", true)) {
			// TODO: Implement ActionInventory
		} else if (line.matchString("*:kill*", true)) {
			actionList.push_back(new ActionKill(_engine, line));
		} else if (line.matchString("*:menu_bar_enable*", true)) {
			// TODO: Implement ActionMenuBarEnable
		} else if (line.matchString("*:music*", true)) {
			actionList.push_back(new ActionMusic(_engine, line, false));
		} else if (line.matchString("*:pan_track*", true)) {
			// TODO: Implement ActionPanTrack
		} else if (line.matchString("*:playpreload*", true)) {
			actionList.push_back(new ActionPlayPreloadAnimation(_engine, line));
		} else if (line.matchString("*:preferences*", true)) {
			// TODO: Implement ActionPreferences
		} else if (line.matchString("*:quit*", true)) {
			actionList.push_back(new ActionQuit(_engine));
		} else if (line.matchString("*:random*", true)) {
			actionList.push_back(new ActionRandom(_engine, line));
		} else if (line.matchString("*:region*", true)) {
			// TODO: Implement ActionRegion
		} else if (line.matchString("*:restore_game*", true)) {
			// TODO: Implement ActionRestoreGame
		} else if (line.matchString("*:rotate_to*", true)) {
			// TODO: Implement ActionRotateTo
		} else if (line.matchString("*:save_game*", true)) {
			// TODO: Implement ActionSaveGame
		} else if (line.matchString("*:set_partial_screen*", true)) {
			actionList.push_back(new ActionSetPartialScreen(_engine, line));
		} else if (line.matchString("*:set_screen*", true)) {
			actionList.push_back(new ActionSetScreen(_engine, line));
		} else if (line.matchString("*:set_venus*", true)) {
			// TODO: Implement ActionSetVenus
		} else if (line.matchString("*:stop*", true)) {
			actionList.push_back(new ActionStop(_engine, line));
		} else if (line.matchString("*:streamvideo*", true)) {
			actionList.push_back(new ActionStreamVideo(_engine, line));
		} else if (line.matchString("*:syncsound*", true)) {
			// TODO: Implement ActionSyncSound
		} else if (line.matchString("*:timer*", true)) {
			actionList.push_back(new ActionTimer(_engine, line));
		} else if (line.matchString("*:ttytext*", true)) {
			// TODO: Implement ActionTTYText
		} else if (line.matchString("*:universe_music*", true)) {
			actionList.push_back(new ActionMusic(_engine, line, true));
		} else if (line.matchString("*:copy_file*", true)) {
			// Not used. Purposely left empty
		} else {
			warning("Unhandled result action type: %s", line.c_str());
		}

		line = stream.readLine();
		trimCommentsAndWhiteSpace(&line);
	}

	return;
}

uint ScriptManager::parseFlags(Common::SeekableReadStream &stream) const {
	uint flags = 0;

	// Loop until we find the closing brace
	Common::String line = stream.readLine();
	trimCommentsAndWhiteSpace(&line);

	while (!stream.eos() && !line.contains('}')) {
		if (line.matchString("ONCE_PER_INST", true)) {
			flags |= Puzzle::ONCE_PER_INST;
		} else if (line.matchString("DO_ME_NOW", true)) {
			flags |= Puzzle::DO_ME_NOW;
		} else if (line.matchString("DISABLED", true)) {
			flags |= Puzzle::DISABLED;
		}

		line = stream.readLine();
		trimCommentsAndWhiteSpace(&line);
	}

	return flags;
}

Control *ScriptManager::parseControl(Common::String &line, Common::SeekableReadStream &stream) {
	uint32 key;
	char controlTypeBuffer[20];

	sscanf(line.c_str(), "control:%u %s {", &key, controlTypeBuffer);

	Common::String controlType(controlTypeBuffer);

	if (controlType.equalsIgnoreCase("push_toggle")) {
		return new PushToggleControl(_engine, key, stream);
	} else if (controlType.equalsIgnoreCase("flat")) {
		Control::parseFlatControl(_engine);
		return NULL;
	} else if (controlType.equalsIgnoreCase("pana")) {
		Control::parsePanoramaControl(_engine, stream);
		return NULL;
	} else if (controlType.equalsIgnoreCase("tilt")) {
		Control::parseTiltControl(_engine, stream);
		return NULL;
	} else if (controlType.equalsIgnoreCase("lever")) {
		return new LeverControl(_engine, key, stream);
	}
	return NULL;
}

} // End of namespace ZVision