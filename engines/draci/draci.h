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
 * $URL$
 * $Id$
 *
 */

#ifndef DRACI_H
#define DRACI_H
 
#include "common/system.h"
#include "engines/engine.h"
#include "engines/advancedDetector.h"

#include "draci/screen.h"
#include "draci/font.h"

namespace Draci {

class DraciEngine : public Engine {
public:
	DraciEngine(OSystem *syst, const ADGameDescription *gameDesc);
	~DraciEngine();

	int init();
	int go();
	Common::Error run();

	bool hasFeature(Engine::EngineFeature f) const;

	Font _font;
	Screen *_screen;

	int _screenWidth;
	int _screenHeight;

private:
	Common::RandomSource _rnd;
};

enum {
	kDraciGeneralDebugLevel = 1 << 0,
	kDraciBytecodeDebugLevel = 1 << 1,
	kDraciArchiverDebugLevel = 1 << 2
};

} // End of namespace Draci

#endif // DRACI_H

