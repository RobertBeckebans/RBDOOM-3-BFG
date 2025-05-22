/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2016 Leyland Needham

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

#pragma hdrstop
#include "precompiled.h"

#include "VRSystem.h"

idCVar vr_enable( "vr_enable", "1", CVAR_INTEGER | CVAR_INIT | CVAR_NEW, "" );
idCVar vr_resolutionScale( "vr_resolutionScale", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT | CVAR_NEW, "hmd resolution scaling, restart required" );
idCVar vr_playerHeightCM( "vr_playerHeightCM", "183", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT | CVAR_NEW, "player height for vr in centimeters" );
idCVar vr_aimLook( "vr_aimLook", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL | CVAR_NEW, "aim where you look" );
idCVar vr_seated( "vr_seated", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL | CVAR_NEW, "seated mode" );
idCVar vr_forceGamepad( "vr_forceGamepad", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL | CVAR_NEW, "force using the gamepad to control weapons" );
idCVar vr_knockbackScale( "vr_knockbackScale", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT | CVAR_NEW, "how much knockback affects you" );
idCVar vr_strafing( "vr_strafing", "1", CVAR_ARCHIVE | CVAR_BOOL | CVAR_NEW, "enable/disable left control strafing" );
idCVar vr_forwardOnly( "vr_forwardOnly", "0", CVAR_ARCHIVE | CVAR_BOOL | CVAR_NEW, "left touchpad only moves forward" );
idCVar vr_relativeAxis( "vr_relativeAxis", "0", CVAR_ARCHIVE | CVAR_BOOL | CVAR_NEW, "movement relative to initial touch" );
idCVar vr_maxRadius( "vr_maxRadius", "0.9", CVAR_ARCHIVE | CVAR_FLOAT | CVAR_NEW, "smaller values make it easier to hit max movement speed" );
idCVar vr_responseCurve( "vr_responseCurve", "0", CVAR_ARCHIVE | CVAR_FLOAT | CVAR_NEW, "interpoloate between linear and square curves, -1 for inverse square" );
idCVar vr_moveMode( "vr_moveMode", "8", CVAR_ARCHIVE | CVAR_INTEGER | CVAR_NEW, "0 touch walk | 1 touch walk & hold run | 2 touch walk & click run | 3 click walk | 4 click walk & hold run | 5 click walk & double click run | 6 hold walk" );
idCVar vr_moveSpeed( "vr_moveSpeed", "0.5", CVAR_ARCHIVE | CVAR_FLOAT | CVAR_NEW, "Touchpad player movement speed is multiplied by this value. Set to 1 for normal speed, or between 0 and 1 for slower movement." );


VRSystem* vrSystem = nullptr;



