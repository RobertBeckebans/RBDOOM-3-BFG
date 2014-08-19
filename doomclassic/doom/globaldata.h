/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company. 

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").  

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef _GLOBAL_DATA_H
#define _GLOBAL_DATA_H


#include "d_main.h"
#include "d_net.h"
#include "d_player.h"
#include "defs.h"
#include "doomlib.h"
#include "doomtype.h"
#include "dstrings.h"
#include "hu_lib.h"
#include "hu_stuff.h"
#include "info.h"
#include "m_cheat.h"
#include "m_fixed.h"
#include "p_local.h"
#include "p_spec.h"
#include "r_bsp.h"
#include "r_defs.h" 
#include "sounds.h"
#include "st_lib.h"
#include "st_stuff.h"
#include "structs.h"
#include "typedefs.h"
#include "w_wad.h"
#include "z_zone.h"

struct Globals {
	void InitGlobals();
#include "vars.h"
};

extern Globals *g;

#define GLOBAL( type, name ) type name
#define GLOBAL_ARRAY( type, name, count ) type name[count]

extern void localCalculateAchievements(bool epComplete);


#endif
