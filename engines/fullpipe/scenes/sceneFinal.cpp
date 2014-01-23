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

#include "fullpipe/fullpipe.h"

#include "fullpipe/objectnames.h"
#include "fullpipe/constants.h"

#include "fullpipe/gameloader.h"
#include "fullpipe/motion.h"
#include "fullpipe/scenes.h"
#include "fullpipe/statics.h"

#include "fullpipe/interaction.h"
#include "fullpipe/behavior.h"


namespace Fullpipe {

void sceneFinal_initScene() {
	g_fp->_gameLoader->loadScene(SC_FINAL2);
	g_fp->accessScene(SC_FINAL2)->setPictureObjectsFlag4();
	g_fp->_gameLoader->loadScene(SC_FINAL3);
	g_fp->accessScene(SC_FINAL3)->setPictureObjectsFlag4();
	g_fp->_gameLoader->loadScene(SC_FINAL4);
	g_fp->accessScene(SC_FINAL4)->setPictureObjectsFlag4();

	getGameLoaderInventory()->setIsLocked(0);
	getGameLoaderInventory()->slideIn();

	g_fp->_updateFlag = 0;
	g_fp->_flgCanOpenMap = 0;

	g_vars->sceneFinal_var01 = 0;
	g_vars->sceneFinal_var02 = 0;
	g_vars->sceneFinal_var03 = 0;
}

} // End of namespace Fullpipe
