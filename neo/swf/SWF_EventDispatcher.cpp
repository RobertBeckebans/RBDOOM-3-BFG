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
#include "precompiled.h"
#pragma hdrstop

#include "SWF_EventDispatcher.h"

/*
========================
idSWFScriptObject_EventDispatcherPrototype
========================
*/
#define SWF_EVENTDISPATCHER_FUNCTION_DEFINE( x ) idSWFScriptVar idSWFScriptObject_EventDispatcherPrototype::idSWFScriptFunction_##x::Call( idSWFScriptObject * thisObject, const idSWFParmList & parms )
#define SWF_EVENTDISPATCHER_NATIVE_VAR_DEFINE_GET( x ) idSWFScriptVar idSWFScriptObject_EventDispatcherPrototype::idSWFScriptNativeVar_##x::Get( class idSWFScriptObject * object )
#define SWF_EVENTDISPATCHER_NATIVE_VAR_DEFINE_SET( x ) void  idSWFScriptObject_EventDispatcherPrototype::idSWFScriptNativeVar_##x::Set( class idSWFScriptObject * object, const idSWFScriptVar & value )

#define SWF_EVENTDISPATCHER_PTHIS_FUNC( x ) idSWFScriptObject * pThis = thisObject ? thisObject : NULL; if ( !verify( pThis != NULL ) ) { idLib::Warning( "SWF: tried to call " x " on NULL object" ); return idSWFScriptVar(); }
#define SWF_EVENTDISPATCHER_PTHIS_GET( x ) idSWFScriptObject * pThis = object ? object : NULL; if ( pThis == NULL ) { return idSWFScriptVar(); }
#define SWF_EVENTDISPATCHER_PTHIS_SET( x ) idSWFScriptObject * pThis = object ? object : NULL; if ( pThis == NULL ) { return; }

#define SWF_EVENTDISPATCHER_FUNCTION_SET( x ) scriptFunction_##x.AddRef(); Set( #x, &scriptFunction_##x );
#define SWF_EVENTDISPATCHER_NATIVE_VAR_SET( x ) SetNative( #x, &swfScriptVar_##x );


idSWFScriptObject_EventDispatcherPrototype::idSWFScriptObject_EventDispatcherPrototype()
{
	SWF_EVENTDISPATCHER_NATIVE_VAR_SET( MouseEvent );
	SWF_EVENTDISPATCHER_NATIVE_VAR_SET( Event );
	SWF_EVENTDISPATCHER_FUNCTION_SET( addEventListener );
}

SWF_EVENTDISPATCHER_NATIVE_VAR_DEFINE_SET( MouseEvent ) {}
SWF_EVENTDISPATCHER_NATIVE_VAR_DEFINE_SET( Event ) {}

SWF_EVENTDISPATCHER_NATIVE_VAR_DEFINE_GET( MouseEvent )
{
	static idSWFScriptObject* mouseEventObj = nullptr;
	if( mouseEventObj == nullptr )
	{
		mouseEventObj = idSWFScriptObject::Alloc( );
		idSWFScriptObject* eventParms = idSWFScriptObject::Alloc( );

		eventParms->Set( "type", "MouseEvent" );
		mouseEventObj->Set( "[MouseEvent]", eventParms );
		//constants
		mouseEventObj->Set( "CLICK",				"click" );
		mouseEventObj->Set( "CONTEXT_MENU",			"contextMenu" );
		mouseEventObj->Set( "DOUBLE_CLICK",			"doubleClick" );
		mouseEventObj->Set( "MIDDLE_CLICK",			"middleClick" );
		mouseEventObj->Set( "MIDDLE_MOUSE_DOWN",	"middleMouseDown" );
		mouseEventObj->Set( "MIDDLE_MOUSE_UP",		"middleMouseUp" );
		mouseEventObj->Set( "MOUSE_DOWN",			"mouseDown" );
		mouseEventObj->Set( "MOUSE_MOVE",			"mouseMove" );
		mouseEventObj->Set( "MOUSE_OUT",			"mouseOut" );
		mouseEventObj->Set( "MOUSE_OVER",			"mouseOver" );
		mouseEventObj->Set( "MOUSE_UP",				"mouseUp" );
		mouseEventObj->Set( "MOUSE_WHEEL ",			"mouseWheel" );
		mouseEventObj->Set( "RELEASE_OUTSIDE",		"releaseOutside" );
		mouseEventObj->Set( "RIGHT_CLICK",			"rightClick" );
		mouseEventObj->Set( "RIGHT_MOUSE_DOWN",		"rightMouseDown" );
		mouseEventObj->Set( "RIGHT_MOUSE_UP",		"rightMouseUp" );
		mouseEventObj->Set( "ROLL_OUT",				"rollOut" );
		mouseEventObj->Set( "ROLL_OVER",			"rollOver" );
	}
	return mouseEventObj;
}

SWF_EVENTDISPATCHER_NATIVE_VAR_DEFINE_GET( Event )
{
	static idSWFScriptObject* eventObj = nullptr;
	if( eventObj == nullptr )
	{
		eventObj = idSWFScriptObject::Alloc( );
		idSWFScriptObject* eventParms = idSWFScriptObject::Alloc( );

		eventParms->Set( "type", "Event" );
		eventObj->Set( "[Event]", eventParms );

		//constants
		eventObj->Set( "ACTIVATE",						"activate" );
		eventObj->Set( "ADDED", 						"added" );
		eventObj->Set( "ADDED_TO_STAGE",				"addedToStage" );
		eventObj->Set( "BROWSER_ZOOM_CHANGE",			"browserZoomChange" );
		eventObj->Set( "CANCEL",						"cancel" );
		eventObj->Set( "CHANGE",						"change" );
		eventObj->Set( "CHANNEL_MESSAGE",				"channelMessage" );
		eventObj->Set( "CHANNEL_STATE",					"channelState" );
		eventObj->Set( "CLEAR",							"clear" );
		eventObj->Set( "CLOSE",							"close" );
		eventObj->Set( "CLOSING",						"closing" );
		eventObj->Set( "COMPLETE",						"complete" );
		eventObj->Set( "CONNECT",						"connect" );
		eventObj->Set( "CONTEXT3D_CREATE",				"context3DCreate" );
		eventObj->Set( "COPY",							"copy" );
		eventObj->Set( "CUT",							"cut" );
		eventObj->Set( "DEACTIVATE",					"deactivate" );
		eventObj->Set( "DISPLAYING",					"displaying" );
		eventObj->Set( "ENTER_FRAME",					"enterFrame" );
		eventObj->Set( "EXIT_FRAME",					"exitFrame" );
		eventObj->Set( "EXITING",						"exiting" );
		eventObj->Set( "FRAME_CONSTRUCTED",				"frameConstructed" );
		eventObj->Set( "FRAME_LABEL",					"frameLabel" );
		eventObj->Set( "FULLSCREEN",					"fullScreen" );
		eventObj->Set( "HTML_BOUNDS_CHANGE",			"htmlBoundsChange" );
		eventObj->Set( "HTML_DOM_INITIALIZE",			"htmlDOMInitialize" );
		eventObj->Set( "HTML_RENDER",					"htmlRender" );
		eventObj->Set( "ID3",							"id3" );
		eventObj->Set( "INIT",							"init" );
		eventObj->Set( "LOCATION_CHANGE",				"locationChange" );
		eventObj->Set( "MOUSE_LEAVE",					"mouseLeave" );
		eventObj->Set( "NETWORK_CHANGE",				"networkChange" );
		eventObj->Set( "OPEN",							"open" );
		eventObj->Set( "PASTE",							"paste" );
		eventObj->Set( "PREPARING",						"preparing" );
		eventObj->Set( "REMOVED",						"removed" );
		eventObj->Set( "REMOVED_FROM_STAGE",			"removedFromStage" );
		eventObj->Set( "RENDER",						"render" );
		eventObj->Set( "RESIZE",						"resize" );
		eventObj->Set( "SCROLL",						"scroll" );
		eventObj->Set( "SELECT",						"select" );
		eventObj->Set( "SELECT_ALL",					"selectAll" );
		eventObj->Set( "SOUND_COMPLETE",				"soundComplete" );
		eventObj->Set( "STANDARD_ERROR_CLOSE",			"standardErrorClose" );
		eventObj->Set( "STANDARD_INPUT_CLOSE",			"standardInputClose" );
		eventObj->Set( "STANDARD_OUTPUT_CLOSE",			"standardOutputClose" );
		eventObj->Set( "SUSPEND",						"suspend" );
		eventObj->Set( "TAB_CHILDREN_CHANGE",			"tabChildrenChange" );
		eventObj->Set( "TAB_ENABLED_CHANGE",			"tabEnabledChange" );
		eventObj->Set( "TAB_INDEX_CHANGE",				"tabIndexChange" );
		eventObj->Set( "TEXT_INTERACTION_MODE_CHANGE",	"textInteractionModeChange" );
		eventObj->Set( "TEXTURE_READY",					"textureReady" );
		eventObj->Set( "UNLOAD",						"unload" );
		eventObj->Set( "USER_IDLE",						"userIdle" );
		eventObj->Set( "USER_PRESENT",					"userPresent" );
		eventObj->Set( "VIDEO_FRAME",					"videoFrame" );
		eventObj->Set( "WORKER_STATE",					"workerState" );
	}
	return eventObj;
}

SWF_EVENTDISPATCHER_FUNCTION_DEFINE( addEventListener )
{
	SWF_EVENTDISPATCHER_PTHIS_FUNC( "addEventListener" );
	swfNamedVar_t* dispatcher = thisObject->GetVariable( "__eventDispatcher__", true );

	if( dispatcher->value.IsUndefined() )
	{
		dispatcher->value.SetObject( idSWFScriptObject::Alloc() );
	}

	dispatcher->value.GetObject()->Set( parms[0].ToString(), parms[1] );
	common->DPrintf( "{%s} AddEventListener(%s,%s)\n", thisObject->GetSprite()->name.c_str(), parms[0].ToString().c_str(), parms[1].ToString().c_str() );
	//add listener
	return idSWFScriptVar( );
}


