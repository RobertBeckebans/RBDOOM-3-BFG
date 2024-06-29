/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 2023 Harrie van Ginneken

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
#ifndef __SWF_EVENTDISPATCHER_H__
#define __SWF_EVENTDISPATCHER_H__

#include "SWF_ScriptObject.h"
#include "SWF_ParmList.h"
#include "SWF_Types.h"
#include "SWF_Sprites.h"
#include "SWF.h"
#include "SWF_Abc.h"

class idSWFScriptObject_EventDispatcherPrototype : public idSWFScriptObject
{
public:
	idSWFScriptObject_EventDispatcherPrototype( );
#define SWF_EVENTDISPATCHER_FUNCTION_DECLARE( x ) \
	class idSWFScriptFunction_##x : public idSWFScriptFunction { \
	public: \
		void			AddRef() {} \
		void			Release() {} \
		idSWFScriptVar Call( idSWFScriptObject * thisObject, const idSWFParmList & parms ); \
	} scriptFunction_##x

	SWF_NATIVE_VAR_DECLARE( MouseEvent );
	SWF_NATIVE_VAR_DECLARE( Event );

	SWF_EVENTDISPATCHER_FUNCTION_DECLARE( addEventListener );
	//SWF_EVENTDISPATHCER_FUNCTION_DECLARE( removeEventListener );
	//SWF_EVENTDISPATHCER_FUNCTION_DECLARE( dispatchEvent );
	//SWF_EVENTDISPATHCER_FUNCTION_DECLARE( dispatchQueue );

	//object.addEventListener = _fEventDispatcher.addEventListener;
	//object.removeEventListener = _fEventDispatcher.removeEventListener;
	//object.dispatchEvent = _fEventDispatcher.dispatchEvent;
	//object.dispatchQueue = _fEventDispatcher.dispatchQueue;
};

#endif //__SWF_EVENTDISPATCHER_H__
