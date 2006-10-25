/* ScummVM - Scumm Interpreter
 * Copyright (C) 2005-2006 Neil Millstone
 * Copyright (C) 2006 The ScummVM project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
 
 #ifndef _BLITTERS_H_
 #define _BLITTERS_H_

namespace DS {

void asmDrawStripToScreen(int height, int width, byte const* text, byte const* src, byte* dst, 
	int vsPitch, int vmScreenWidth, int textSurfacePitch);
void asmCopy8Col(byte* dst, int dstPitch, const byte* src, int height);
void Rescale_320xPAL8Scanline_To_256x1555Scanline(u16* dest, const u8* src, const u16* palette);
void Rescale_320x1555Scanline_To_256x1555Scanline(u16* dest, const u16* src);

}
	
#endif
