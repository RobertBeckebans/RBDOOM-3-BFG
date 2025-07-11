/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014-2023 Robert Beckebans

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
#include "../Game_local.h"

const static int NUM_SYSTEM_OPTIONS_OPTIONS = 8;

extern idCVar r_graphicsAPI;
extern idCVar r_antiAliasing;
extern idCVar r_useFilmicPostFX;
extern idCVar r_swapInterval;
extern idCVar s_volume_dB;
extern idCVar r_exposure; // RB: use this to control HDR exposure or brightness in LDR mode
extern idCVar r_lightScale;
extern idCVar r_useSSR;

/*
========================
idMenuScreen_Shell_SystemOptions::Initialize
========================
*/
void idMenuScreen_Shell_SystemOptions::Initialize( idMenuHandler* data )
{
	idMenuScreen::Initialize( data );

	if( data != NULL )
	{
		menuGUI = data->GetGUI();
	}

	SetSpritePath( "menuSystemOptions" );

	options = new( TAG_SWF ) idMenuWidget_SystemOptionsList(); // RB: allow more options than defined in the SWF
	options->SetNumVisibleOptions( NUM_SYSTEM_OPTIONS_OPTIONS );
	options->SetSpritePath( GetSpritePath(), "info", "options" );
	options->SetWrappingAllowed( true );
	options->SetControlList( true );
	options->Initialize( data );

	btnBack = new( TAG_SWF ) idMenuWidget_Button();
	btnBack->Initialize( data );
	btnBack->SetLabel( "#str_swf_settings" );
	btnBack->SetSpritePath( GetSpritePath(), "info", "btnBack" );
	btnBack->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_GO_BACK );

	AddChild( options );
	AddChild( btnBack );

	idMenuWidget_ControlButton* control;

#ifdef _WIN32
	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TEXT );
	control->SetLabel( "Render API" );
	control->SetDataSource( &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_RENDERAPI );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, idMenuDataSource_SystemSettings::SYSTEM_FIELD_RENDERAPI );
	options->AddChild( control );
#endif

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TEXT );
	control->SetLabel( "#str_02154" );
	control->SetDataSource( &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_FULLSCREEN );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, idMenuDataSource_SystemSettings::SYSTEM_FIELD_FULLSCREEN );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TEXT );
	control->SetLabel( "#str_swf_framerate" );
	control->SetDataSource( &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_FRAMERATE );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, idMenuDataSource_SystemSettings::SYSTEM_FIELD_FRAMERATE );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TEXT );
	control->SetLabel( "#str_04126" );
	control->SetDataSource( &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_VSYNC );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, idMenuDataSource_SystemSettings::SYSTEM_FIELD_VSYNC );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TEXT );
	control->SetLabel( "#str_04128" );
	control->SetDataSource( &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_ANTIALIASING );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, idMenuDataSource_SystemSettings::SYSTEM_FIELD_ANTIALIASING );
	options->AddChild( control );

	// RB begin
	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TEXT );
	control->SetLabel( "Render Mode" );
	control->SetDataSource( &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_RENDERMODE );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, idMenuDataSource_SystemSettings::SYSTEM_FIELD_RENDERMODE );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_BAR );
	control->SetLabel( "Ambient Lighting" );
	control->SetDescription( "Sets the amount of indirect lighting. Needed for modern PBR reflections" );
	control->SetDataSource( &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_AMBIENT_BRIGHTNESS );
	control->SetupEvents( 2, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, idMenuDataSource_SystemSettings::SYSTEM_FIELD_AMBIENT_BRIGHTNESS );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TEXT );
	control->SetLabel( "SSAO" );
	control->SetDataSource( &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_SSAO );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, idMenuDataSource_SystemSettings::SYSTEM_FIELD_SSAO );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TEXT );
	control->SetLabel( "Blood Reflections" );
	control->SetDataSource( &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_BLOOD_REFLECTIONS );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, idMenuDataSource_SystemSettings::SYSTEM_FIELD_BLOOD_REFLECTIONS );
	options->AddChild( control );

	/*control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_BAR );
	control->SetLabel( "#str_swf_lodbias" );
	control->SetDataSource( &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_LODBIAS );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, idMenuDataSource_SystemSettings::SYSTEM_FIELD_LODBIAS );
	options->AddChild( control );*/

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TEXT );
	control->SetLabel( "Filmic Post FX" );
	control->SetDataSource( &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_FILMIC_POSTFX );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, idMenuDataSource_SystemSettings::SYSTEM_FIELD_FILMIC_POSTFX );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TEXT );
	control->SetLabel( "CRT Filter" );
	control->SetDataSource( &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_CRT_POSTFX );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, idMenuDataSource_SystemSettings::SYSTEM_FIELD_CRT_POSTFX );
	options->AddChild( control );
	// RB end

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_BAR );
	control->SetLabel( "#str_02155" );	// Brightness
	control->SetDataSource( &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_BRIGHTNESS );
	control->SetupEvents( 2, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, idMenuDataSource_SystemSettings::SYSTEM_FIELD_BRIGHTNESS );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_BAR );
	control->SetLabel( "#str_02163" );	// Volume
	control->SetDataSource( &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_VOLUME );
	control->SetupEvents( 2, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, idMenuDataSource_SystemSettings::SYSTEM_FIELD_VOLUME );
	options->AddChild( control );

	options->AddEventAction( WIDGET_EVENT_SCROLL_DOWN ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER, WIDGET_EVENT_SCROLL_DOWN ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_UP ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER, WIDGET_EVENT_SCROLL_UP ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_DOWN_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_RELEASE ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_UP_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_RELEASE ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_DOWN_LSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER, WIDGET_EVENT_SCROLL_DOWN_LSTICK ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_UP_LSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER, WIDGET_EVENT_SCROLL_UP_LSTICK ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_DOWN_LSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_LSTICK_RELEASE ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_UP_LSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_LSTICK_RELEASE ) );
}

/*
========================
idMenuScreen_Shell_SystemOptions::Update
========================
*/
void idMenuScreen_Shell_SystemOptions::Update()
{

	if( menuData != NULL )
	{
		idMenuWidget_CommandBar* cmdBar = menuData->GetCmdBar();
		if( cmdBar != NULL )
		{
			cmdBar->ClearAllButtons();
			idMenuWidget_CommandBar::buttonInfo_t* buttonInfo;
			buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY2 );
			if( menuData->GetPlatform() != 2 )
			{
				buttonInfo->label = "#str_00395";
			}
			buttonInfo->action.Set( WIDGET_ACTION_GO_BACK );

			buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY1 );
			buttonInfo->action.Set( WIDGET_ACTION_PRESS_FOCUSED );
		}
	}

	idSWFScriptObject& root = GetSWFObject()->GetRootObject();
	if( BindSprite( root ) )
	{
		idSWFTextInstance* heading = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtHeading" );
		if( heading != NULL )
		{
			heading->SetText( "#str_00183" );	// FULLSCREEN
			heading->SetStrokeInfo( true, 0.75f, 1.75f );
		}

		idSWFSpriteInstance* gradient = GetSprite()->GetScriptObject()->GetNestedSprite( "info", "gradient" );
		if( gradient != NULL && heading != NULL )
		{
			gradient->SetXPos( heading->GetTextLength() );
		}
	}

	if( btnBack != NULL )
	{
		btnBack->BindSprite( root );
	}

	idMenuScreen::Update();
}

/*
========================
idMenuScreen_Shell_SystemOptions::ShowScreen
========================
*/
void idMenuScreen_Shell_SystemOptions::ShowScreen( const mainMenuTransition_t transitionType )
{

	systemData.LoadData();

	idMenuScreen::ShowScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_SystemOptions::HideScreen
========================
*/
void idMenuScreen_Shell_SystemOptions::HideScreen( const mainMenuTransition_t transitionType )
{

	if( systemData.IsRestartRequired() )
	{
		class idSWFScriptFunction_Restart : public idSWFScriptFunction_RefCounted
		{
		public:
			idSWFScriptFunction_Restart( gameDialogMessages_t _msg, bool _restart )
			{
				msg = _msg;
				restart = _restart;
			}
			idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
			{
				common->Dialog().ClearDialog( msg );
				if( restart )
				{
					// DG: Sys_ReLaunch() doesn't need any options anymore
					//     (the old way would have been unnecessarily painful on POSIX systems)
					Sys_ReLaunch();
					// DG end
				}
				return idSWFScriptVar();
			}
		private:
			gameDialogMessages_t msg;
			bool restart;
		};
		idStaticList<idSWFScriptFunction*, 4> callbacks;
		idStaticList<idStrId, 4> optionText;
		callbacks.Append( new idSWFScriptFunction_Restart( GDM_GAME_RESTART_REQUIRED, false ) );
		callbacks.Append( new idSWFScriptFunction_Restart( GDM_GAME_RESTART_REQUIRED, true ) );
		optionText.Append( idStrId( "#str_00100113" ) ); // Continue
		optionText.Append( idStrId( "#str_02487" ) ); // Restart Now
		common->Dialog().AddDynamicDialog( GDM_GAME_RESTART_REQUIRED, callbacks, optionText, true, idStr() );
	}

	if( systemData.IsDataChanged() )
	{
		systemData.CommitData();
	}

	idMenuScreen::HideScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_SystemOptions::HandleAction h
========================
*/
bool idMenuScreen_Shell_SystemOptions::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{

	if( menuData == NULL )
	{
		return true;
	}

	if( menuData->ActiveScreen() != SHELL_AREA_SYSTEM_OPTIONS )
	{
		return false;
	}

	widgetAction_t actionType = action.GetType();
	const idSWFParmList& parms = action.GetParms();

	switch( actionType )
	{
		case WIDGET_ACTION_GO_BACK:
		{
			if( menuData != NULL )
			{
				menuData->SetNextScreen( SHELL_AREA_SETTINGS, MENU_TRANSITION_SIMPLE );
			}
			return true;
		}
		case WIDGET_ACTION_ADJUST_FIELD:
			if( widget->GetDataSourceFieldIndex() == idMenuDataSource_SystemSettings::SYSTEM_FIELD_FULLSCREEN )
			{
				menuData->SetNextScreen( SHELL_AREA_RESOLUTION, MENU_TRANSITION_SIMPLE );
				return true;
			}
			break;
		case WIDGET_ACTION_COMMAND:
		{

			if( options == NULL )
			{
				return true;
			}

			int selectionIndex = options->GetFocusIndex();
			if( parms.Num() > 0 )
			{
				selectionIndex = parms[0].ToInteger();
			}

			if( options && selectionIndex != options->GetFocusIndex() )
			{
				options->SetViewIndex( options->GetViewOffset() + selectionIndex );
				options->SetFocusIndex( selectionIndex );
			}

			switch( parms[0].ToInteger() )
			{
				case idMenuDataSource_SystemSettings::SYSTEM_FIELD_FULLSCREEN:
				{
					menuData->SetNextScreen( SHELL_AREA_RESOLUTION, MENU_TRANSITION_SIMPLE );
					return true;
				}
				default:
				{
					systemData.AdjustField( parms[0].ToInteger(), 1 );
					options->Update();
				}
			}

			return true;
		}
		case WIDGET_ACTION_START_REPEATER:
		{

			if( options == NULL )
			{
				return true;
			}

			if( parms.Num() == 4 )
			{
				int selectionIndex = parms[3].ToInteger();
				if( selectionIndex != options->GetFocusIndex() )
				{
					options->SetViewIndex( options->GetViewOffset() + selectionIndex );
					options->SetFocusIndex( selectionIndex );
				}
			}
			break;
		}
	}

	return idMenuWidget::HandleAction( action, event, widget, forceHandled );
}

/////////////////////////////////
// SCREEN SETTINGS
/////////////////////////////////

/*
========================
idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::idMenuDataSource_SystemSettings
========================
*/
idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::idMenuDataSource_SystemSettings()
{
}

/*
========================
idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::LoadData
========================
*/
void idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::LoadData()
{
	originalRenderAPI = r_graphicsAPI.GetString();
	originalFramerate = com_engineHz.GetInteger();
	originalAntialias = r_antiAliasing.GetInteger();
	originalVsync = r_swapInterval.GetInteger();
	originalBrightness = r_exposure.GetFloat();
	originalVolume = s_volume_dB.GetFloat();
	// RB begin
	originalRenderMode = r_renderMode.GetInteger();
	originalAmbientBrightness = r_forceAmbient.GetFloat();
	originalSSAO = r_useSSAO.GetInteger();
	originalBloodReflections = r_useSSR.GetInteger();
	originalPostProcessing = r_useFilmicPostFX.GetInteger();
	originalCRTPostFX = r_useCRTPostFX.GetInteger();
	// RB end

	const int fullscreen = r_fullscreen.GetInteger();
	if( fullscreen > 0 )
	{
		R_GetModeListForDisplay( fullscreen - 1, modeList );
	}
	else
	{
		modeList.Clear();
	}
}

/*
========================
idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::IsRestartRequired
========================
*/
bool idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::IsRestartRequired() const
{
	/*
	if( originalAntialias != r_antiAliasing.GetInteger() )
	{
		return true;
	}
	*/

	if( idStr::Icmp( r_graphicsAPI.GetString(), originalRenderAPI ) != 0 )
	{
		return true;
	}

	if( originalFramerate != com_engineHz.GetInteger() )
	{
		return true;
	}

	return false;
}

/*
========================
idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::CommitData
========================
*/
void idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::CommitData()
{
	cvarSystem->SetModifiedFlags( CVAR_ARCHIVE );
}

/*
========================
AdjustOption
Given a current value in an array of possible values, returns the next n value
========================
*/
int AdjustOption( int currentValue, const int values[], int numValues, int adjustment )
{
	int index = 0;
	for( int i = 0; i < numValues; i++ )
	{
		if( currentValue == values[i] )
		{
			index = i;
			break;
		}
	}
	index += adjustment;
	while( index < 0 )
	{
		index += numValues;
	}
	index %= numValues;
	return values[index];
}

/*
========================
LinearAdjust
Linearly converts a float from one scale to another
========================
*/
float LinearAdjust( float input, float currentMin, float currentMax, float desiredMin, float desiredMax )
{
	return ( ( input - currentMin ) / ( currentMax - currentMin ) ) * ( desiredMax - desiredMin ) + desiredMin;
}

/*
========================
idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::AdjustField
========================
*/
void idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::AdjustField( const int fieldIndex, const int adjustAmount )
{
	switch( fieldIndex )
	{
#ifdef _WIN32
		case SYSTEM_FIELD_RENDERAPI:
		{
			static const int numValues = 2;
			static const int values[numValues] = { 0, 1 };
			int option = 0;

			if( !idStr::Icmp( r_graphicsAPI.GetString(), "vulkan" ) )
			{
				option = 1;
			}
			else
			{
				option = 0;
			}

			option = AdjustOption( option, values, numValues, adjustAmount );

			if( option == 1 )
			{
				r_graphicsAPI.SetString( "vulkan" );
			}
			else
			{
				r_graphicsAPI.SetString( "dx12" );
			}
			break;
		}
#endif

		case SYSTEM_FIELD_FRAMERATE:
		{
			static const int numValues = 2;
			static const int values[numValues] = { 60, 120 };
			com_engineHz.SetInteger( AdjustOption( com_engineHz.GetInteger(), values, numValues, adjustAmount ) );
			break;
		}
		case SYSTEM_FIELD_VSYNC:
		{
			static const int numValues = 3;
			static const int values[numValues] = { 0, 1, 2 };
			r_swapInterval.SetInteger( AdjustOption( r_swapInterval.GetInteger(), values, numValues, adjustAmount ) );
			break;
		}
		case SYSTEM_FIELD_ANTIALIASING:
		{
#if ID_MSAA
			static const int numValues = 5;
			static const int values[numValues] =
			{
				ANTI_ALIASING_NONE,
				ANTI_ALIASING_TAA,
				ANTI_ALIASING_TAA_SMAA_1X,
				ANTI_ALIASING_MSAA_2X,
				ANTI_ALIASING_MSAA_4X,
			};
#else
			static const int numValues = 3;
			static const int values[numValues] =
			{
				ANTI_ALIASING_NONE,
				ANTI_ALIASING_SMAA_1X,
				ANTI_ALIASING_TAA,
			};
#endif

			r_antiAliasing.SetInteger( AdjustOption( r_antiAliasing.GetInteger(), values, numValues, adjustAmount ) );
			break;
		}
		// RB begin
		case SYSTEM_FIELD_RENDERMODE:
		{
			static const int numValues = 10;
			static const int values[numValues] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
			r_renderMode.SetInteger( AdjustOption( r_renderMode.GetInteger(), values, numValues, adjustAmount ) );
			break;
		}
		case SYSTEM_FIELD_FILMIC_POSTFX:
		{
			static const int numValues = 2;
			static const int values[numValues] = { 0, 1 };
			r_useFilmicPostFX.SetInteger( AdjustOption( r_useFilmicPostFX.GetInteger(), values, numValues, adjustAmount ) );
			break;
		}
		case SYSTEM_FIELD_CRT_POSTFX:
		{
			static const int numValues = 4;
			static const int values[numValues] = { 0, 1, 2, 3 };
			r_useCRTPostFX.SetInteger( AdjustOption( r_useCRTPostFX.GetInteger(), values, numValues, adjustAmount ) );
			break;
		}
		/*
		RB: this should be the texture quality field
		case SYSTEM_FIELD_LODBIAS:
		{
			const float percent = LinearAdjust( r_lodBias.GetFloat(), -1.0f, 1.0f, 0.0f, 100.0f );
			const float adjusted = percent + ( float )adjustAmount * 5.0f;
			const float clamped = idMath::ClampFloat( 0.0f, 100.0f, adjusted );
			r_lodBias.SetFloat( LinearAdjust( clamped, 0.0f, 100.0f, -1.0f, 1.0f ) );
			break;
		}*/
		case SYSTEM_FIELD_SSAO:
		{
			static const int numValues = 2;
			static const int values[numValues] = { 0, 1 };
			r_useSSAO.SetInteger( AdjustOption( r_useSSAO.GetInteger(), values, numValues, adjustAmount ) );
			break;
		}
		case SYSTEM_FIELD_BLOOD_REFLECTIONS:
		{
			static const int numValues = 2;
			static const int values[numValues] = { 0, 1 };
			r_useSSR.SetInteger( AdjustOption( r_useSSR.GetInteger(), values, numValues, adjustAmount ) );
			break;
		}
		case SYSTEM_FIELD_AMBIENT_BRIGHTNESS:
		{
			const float percent = LinearAdjust( r_forceAmbient.GetFloat(), 0.0f, 1.0f, 0.0f, 100.0f );
			const float adjusted = percent + ( float )adjustAmount;
			const float clamped = idMath::ClampFloat( 0.0f, 100.0f, adjusted );

			r_forceAmbient.SetFloat( LinearAdjust( clamped, 0.0f, 100.0f, 0.0f, 1.0f ) );
			break;
		}
		// RB end
		case SYSTEM_FIELD_BRIGHTNESS:
		{
			const float percent = LinearAdjust( r_exposure.GetFloat(), 0.0f, 1.0f, 0.0f, 100.0f );
			const float adjusted = percent + ( float )adjustAmount;
			const float clamped = idMath::ClampFloat( 0.0f, 100.0f, adjusted );

			r_exposure.SetFloat( LinearAdjust( clamped, 0.0f, 100.0f, 0.0f, 1.0f ) ); // RB
			r_lightScale.SetFloat( LinearAdjust( clamped, 0.0f, 100.0f, 2.0f, 4.0f ) );
			break;
		}
		case SYSTEM_FIELD_VOLUME:
		{
			const float percent = 100.0f * Square( 1.0f - ( s_volume_dB.GetFloat() / DB_SILENCE ) );
			const float adjusted = percent + ( float )adjustAmount;
			const float clamped = idMath::ClampFloat( 0.0f, 100.0f, adjusted );
			s_volume_dB.SetFloat( DB_SILENCE - ( idMath::Sqrt( clamped / 100.0f ) * DB_SILENCE ) );
			break;
		}
	}
	cvarSystem->ClearModifiedFlags( CVAR_ARCHIVE );
}

/*
========================
idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::GetField
========================
*/
idSWFScriptVar idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::GetField( const int fieldIndex ) const
{
	switch( fieldIndex )
	{
#ifdef _WIN32
		case SYSTEM_FIELD_RENDERAPI:
		{
			if( !idStr::Icmp( r_graphicsAPI.GetString(), "vulkan" ) )
			{
				return "Vulkan";
			}
			else
			{
				return "DirectX 12";
			}
		}
#endif

		case SYSTEM_FIELD_FULLSCREEN:
		{
			const int fullscreen = r_fullscreen.GetInteger();
			const int vidmode = r_vidMode.GetInteger();
			if( fullscreen == 0 )
			{
				return "#str_swf_disabled";
			}
			// SRS - Added support for displaying borderless modes
			else if( fullscreen == -1 )
			{
				return "Borderless Window";
			}
			else if( fullscreen == -2 )
			{
				return "Borderless Fullscreen";
			}
			// SRS end
			if( fullscreen < 0 || vidmode < 0 || vidmode >= modeList.Num() )
			{
				return "???";
			}
			if( modeList[vidmode].displayHz == 60 )
			{
				return va( "%4i x %4i", modeList[vidmode].width, modeList[vidmode].height );
			}
			else
			{
				return va( "%4i x %4i @ %dhz", modeList[vidmode].width, modeList[vidmode].height, modeList[vidmode].displayHz );
			}
		}

		case SYSTEM_FIELD_FRAMERATE:
			return va( "%d FPS", com_engineHz.GetInteger() );

		case SYSTEM_FIELD_VSYNC:
			if( r_swapInterval.GetInteger() == 1 )
			{
				return "#str_swf_smart";
			}
			else if( r_swapInterval.GetInteger() == 2 )
			{
				return "#str_swf_enabled";
			}
			else
			{
				return "#str_swf_disabled";
			}

		case SYSTEM_FIELD_ANTIALIASING:
		{
			if( r_antiAliasing.GetInteger() == 0 )
			{
				return "#str_swf_disabled";
			}

#if ID_MSAA
			static const int numValues = 5;
			static const char* values[numValues] =
			{
				"None",
				"TAA",
				"TAA + SMAA 1X",
				"MSAA 2X",
				"MSAA 4X",
			};

			compile_time_assert( numValues == ( ANTI_ALIASING_MSAA_4X + 1 ) );
#else
			static const int numValues = 3;
			static const char* values[numValues] =
			{
				"None",
				"SMAA",
				"TAA"
			};

			compile_time_assert( numValues == ( ANTI_ALIASING_TAA + 1 ) );
#endif

			return values[ r_antiAliasing.GetInteger() ];
		}
		case SYSTEM_FIELD_RENDERMODE:
		{
			static const int numValues = 10;
			static const char* values[numValues] =
			{
				"Doom 3",
				"2-bit",
				"2-bit Hi",
				"Commodore 64",
				"Commodore 64 Hi",
				"Amstrad CPC 6128",
				"Amstrad CPC 6128 Hi",
				"Sega Genesis",
				"Sega Genesis Highres",
				"Sony PSX",
			};

			compile_time_assert( numValues == ( RENDERMODE_PSX + 1 ) );

			return values[ r_renderMode.GetInteger() ];
		}
		case SYSTEM_FIELD_FILMIC_POSTFX:
			if( r_useFilmicPostFX.GetInteger() > 0 )
			{
				return "#str_swf_enabled";
			}
			else
			{
				return "#str_swf_disabled";
			}

		case SYSTEM_FIELD_CRT_POSTFX:
		{
			static const int numValues = 4;
			static const char* values[numValues] =
			{
				"#str_swf_disabled",
				"Mattias",
				"Newpixie",
				"Advanced",
			};

			return values[ r_useCRTPostFX.GetInteger() ];
		}

		//case SYSTEM_FIELD_LODBIAS:
		//	return LinearAdjust( r_lodBias.GetFloat(), -1.0f, 1.0f, 0.0f, 100.0f );

		case SYSTEM_FIELD_SSAO:
			if( r_useSSAO.GetInteger() == 1 )
			{
				return "#str_swf_enabled";
			}
			else
			{
				return "#str_swf_disabled";
			}

		case SYSTEM_FIELD_BLOOD_REFLECTIONS:
			if( r_useSSR.GetInteger() == 1 )
			{
				return "Dynamic (SSR)";
			}
			else
			{
				return "Static";
			}

		case SYSTEM_FIELD_AMBIENT_BRIGHTNESS:
			return LinearAdjust( r_forceAmbient.GetFloat(), 0.0f, 1.0f, 0.0f, 100.0f );

		case SYSTEM_FIELD_BRIGHTNESS:
			return LinearAdjust( r_exposure.GetFloat(), 0.0f, 1.0f, 0.0f, 100.0f );

		case SYSTEM_FIELD_VOLUME:
		{
			return 100.0f * Square( 1.0f - ( s_volume_dB.GetFloat() / DB_SILENCE ) );
		}
	}
	return false;
}

/*
========================
idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::IsDataChanged
========================
*/
bool idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::IsDataChanged() const
{
	if( idStr::Icmp( r_graphicsAPI.GetString(), originalRenderAPI ) != 0 )
	{
		return true;
	}

	if( originalFramerate != com_engineHz.GetInteger() )
	{
		return true;
	}

	if( originalAntialias != r_antiAliasing.GetInteger() )
	{
		return true;
	}

	if( originalVsync != r_swapInterval.GetInteger() )
	{
		return true;
	}

	if( originalRenderMode != r_renderMode.GetInteger() )
	{
		return true;
	}

	if( originalAmbientBrightness != r_forceAmbient.GetFloat() )
	{
		return true;
	}

	if( originalSSAO != r_useSSAO.GetInteger() )
	{
		return true;
	}

	if( originalBloodReflections != r_useSSR.GetInteger() )
	{
		return true;
	}

	if( originalPostProcessing != r_useFilmicPostFX.GetInteger() )
	{
		return true;
	}

	if( originalCRTPostFX != r_useCRTPostFX.GetInteger() )
	{
		return true;
	}

	if( originalBrightness != r_exposure.GetFloat() )
	{
		return true;
	}

	if( originalVolume != s_volume_dB.GetFloat() )
	{
		return true;
	}

	return false;
}

// RB begin
void idMenuWidget_SystemOptionsList::Update()
{
	if( GetSWFObject() == NULL )
	{
		return;
	}

	idSWFScriptObject& root = GetSWFObject()->GetRootObject();

	if( !BindSprite( root ) )
	{
		return;
	}

	//idLib::Printf( "SystemOptionsList::Update( offset = %i )\n", GetViewOffset() );

	// clear old sprites and rebuild the options
	for( int childIndex = 0; childIndex < GetTotalNumberOfOptions(); ++childIndex )
	{
		idMenuWidget& child = GetChildByIndex( childIndex );

		child.ClearSprite();
	}

	for( int optionIndex = 0; optionIndex < GetNumVisibleOptions(); ++optionIndex )
	{
		if( optionIndex >= children.Num() )
		{
			// not enough children
			idSWFSpriteInstance* item = GetSprite()->GetScriptObject()->GetNestedSprite( va( "item%d", optionIndex ) );
			if( item != NULL )
			{
				item->SetVisible( false );
				continue;
			}
		}

		// account view offset and total number of options
		const int childIndex = ( GetViewOffset() + optionIndex ) % GetTotalNumberOfOptions();
		idMenuWidget& child = GetChildByIndex( childIndex );

		child.SetSpritePath( GetSpritePath(), va( "item%d", optionIndex ) );
		if( child.BindSprite( root ) )
		{
			if( optionIndex >= GetTotalNumberOfOptions() )
			{
				child.ClearSprite();
				continue;
			}

			child.Update();

			if( optionIndex == focusIndex )
			{
				child.SetState( WIDGET_STATE_SELECTING );
			}
			else
			{
				child.SetState( WIDGET_STATE_NORMAL );
			}
		}
	}

	idSWFSpriteInstance* const upSprite = GetSprite()->GetScriptObject()->GetSprite( "upIndicator" );
	if( upSprite != NULL )
	{
		upSprite->SetVisible( GetViewOffset() > 0 );
	}

	idSWFSpriteInstance* const downSprite = GetSprite()->GetScriptObject()->GetSprite( "downIndicator" );
	if( downSprite != NULL )
	{
		downSprite->SetVisible( GetViewOffset() + GetNumVisibleOptions() < GetTotalNumberOfOptions() );
	}
}

void idMenuWidget_SystemOptionsList::Scroll( const int scrollAmount, const bool wrapAround )
{
	if( GetTotalNumberOfOptions() == 0 )
	{
		return;
	}

	int newIndex, newOffset;

	// RB: always wrap around
	CalculatePositionFromIndexDelta( newIndex, newOffset, GetViewIndex(), GetViewOffset(), GetNumVisibleOptions(), GetTotalNumberOfOptions(), scrollAmount, IsWrappingAllowed(), true ); //wrapAround );

	//int oldViewIndex = GetViewIndex();
	//int oldViewOffset = GetViewOffset();
	int oldFocusIndex = GetFocusIndex();

	if( newOffset != GetViewOffset() )
	{
		SetViewOffset( newOffset );
		if( menuData != NULL )
		{
			menuData->PlaySound( GUI_SOUND_FOCUS );
		}

		// RB: HACK and I don't like it.
		// focusIndex is used here for the visible state and not for event handling.
		focusIndex = newIndex;
		Update();
		focusIndex = oldFocusIndex;
	}

	if( newIndex != GetViewIndex() )
	{
		SetViewIndex( newIndex );

		// trigger focus/unfocus sprite actions
		SetFocusIndex( newIndex );// - newOffset );
	}

	//idLib::Printf( "scroll = %i, index = %i -> %i, offset = %i -> %i, focus = %i -> %i\n", scrollAmount, oldViewIndex, newIndex, oldViewOffset, newOffset, oldFocusIndex, GetFocusIndex() );
}
// RB end

