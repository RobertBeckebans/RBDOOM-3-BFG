/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2023 Robert Beckebans

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
#include "precompiled.h"
#pragma hdrstop

// RB: rbdmap (DMAP) never constructs an idCinematic/idSndWindow - that code path is compiled
// out of renderer/Material.cpp via "#if !defined( DMAP )" - but some compilers still need the
// out-of-line destructor and the idSndWindow vtable's key function to be defined somewhere
// that gets linked into rbdmap. Provide minimal stand-ins here instead of pulling the full
// renderer/Cinematic.cpp (and its FFmpeg/BinkDec/sound dependencies) into this tool.
#include "../renderer/Cinematic.h"

idCinematic::~idCinematic()
{
}

bool idCinematic::InitFromFile( const char* qpath, bool looping, nvrhi::ICommandList* commandList )
{
	return false;
}

int idCinematic::AnimationLength()
{
	return 0;
}

bool idCinematic::IsPlaying() const
{
	return false;
}

cinData_t idCinematic::ImageForTime( int milliseconds, nvrhi::ICommandList* commandList )
{
	cinData_t c;
	memset( &c, 0, sizeof( c ) );
	return c;
}

void idCinematic::Close()
{
}

void idCinematic::ResetTime( int milliseconds )
{
}

int idCinematic::GetStartTime()
{
	return -1;
}

void idCinematic::ExportToTGA( bool skipExisting )
{
}

float idCinematic::GetFrameRate() const
{
	return 30.0f;
}

int idSndWindow::AnimationLength()
{
	return -1;
}
