/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 2016 Leyland Needham
Copyright (C) 2024 Robert Beckebans (class refactoring)

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

#include <sys/DeviceManager.h>
extern DeviceManager* deviceManager;

#include <openvr.h>

#include "VRSystem.h"

#define MAX_VREVENTS 256

class VRSystem_Valve : public VRSystem
{
	virtual	bool			InitHMD();
	virtual	void			ShutdownHMD() {}
	virtual	void			UpdateHMD() {}

	virtual	void			ResetPose();
	virtual	void			LogDevices();

	virtual void			UpdateResolution();

	virtual int				PollGameInputEvents();
	virtual int				ReturnGameInputEvent( const int n, int& action, int& value );

	virtual void			SubmitStereoRenders( nvrhi::ICommandList* commandList, idImage* image0, idImage* image1 );
	virtual void			PreSwap();
	virtual void			PostSwap();

	virtual idVec2i			GetRenderResolution() const
	{
		return idVec2i( hmdWidth, hmdHeight );
	}

	virtual float			GetScreenSeparation() const
	{
		return openVRScreenSeparation;
	}

	virtual idVec4			GetFOV( int eye ) const
	{
		return openVRfovEye[ eye ];
	}

	virtual float			GetHalfIPD() const
	{
		return openVRHalfIPD;
	}

	virtual bool			GetHead( idVec3& origin, idMat3& axis );
	virtual bool			GetLeftController( idVec3& origin, idMat3& axis );
	virtual bool			GetRightController( idVec3& origin, idMat3& axis );
	virtual void			HapticPulse( int leftDuration, int rightDuration );

	virtual bool			GetLeftControllerAxis( idVec2& axis );
	virtual bool			GetRightControllerAxis( idVec2& axis );

	virtual bool			LeftControllerWasPressed();
	virtual bool			LeftControllerIsPressed();

	virtual bool			RightControllerWasPressed();
	virtual bool			RightControllerIsPressed();

	virtual const idVec3&	GetSeatedOrigin();
	virtual const idMat3&	GetSeatedAxis();
	virtual const idMat3&	GetSeatedAxisInverse();

	const sysEvent_t&		UIEventNext();

	virtual bool			IsActive() const
	{
		return openVREnabled;
	}

	virtual bool			IsSeated() const
	{
		return openVRSeated;
	}

	virtual bool			HasLeftTouchpad() const
	{
		return openVRLeftTouchpad;
	}

	virtual bool			HasRightTouchpad() const
	{
		return openVRRightTouchpad;
	}

private:
	void					ConvertMatrix( const vr::HmdMatrix34_t& poseMat, idVec3& origin, idMat3& axis );
	idStr					GetTrackedDeviceString( vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError* peError = NULL );

	void					UpdateScaling();
	void					UpdateControllers();
	void					MoveDelta( idVec3& delta, float& height );

	// input
	void					ClearEvents();
	void					UIEventQue( sysEventType_t type, int value, int value2 );
	void					GameEventQue( int action, int value );
	int						AxisToDPad( int mode, float x, float y );
	void					GenButtonEvent( uint32_t button, bool left, bool pressed );
	void					GenJoyAxisEvents();
	void					GenMouseEvents();
	bool					ConvertPose( const vr::TrackedDevicePose_t& pose, idVec3& origin, idMat3& axis );

	// unused
	bool					CalculateView( idVec3& origin, idMat3& axis, const idVec3& eyeOffset, bool overridePitch );

	vr::IVRSystem*	hmd = NULL;
	bool			openVREnabled = false;
	bool			openVRSeated;
	bool			openVRLeftTouchpad;
	bool			openVRRightTouchpad;
	idVec4			openVRfovEye[2];
	float			openVRScreenSeparation;
	float			openVRScale;
	float			openVRUnscaledEyeForward;
	float			openVRUnscaledHalfIPD;
	float			openVREyeForward;
	float			openVRHalfIPD;

	int				hmdWidth;
	int				hmdHeight;

	float			m_ScaleX = 1.0f;
	float			m_ScaleY = 1.0f;
	float			m_ScaleZ = 1.0f;
	vr::TrackedDeviceIndex_t m_leftController = vr::k_unTrackedDeviceIndexInvalid;
	vr::TrackedDeviceIndex_t m_rightController = vr::k_unTrackedDeviceIndexInvalid;
	idVec3			m_seatedOrigin;
	idMat3			m_seatedAxis;
	idMat3			m_seatedAxisInverse;

	bool			m_LeftControllerWasPressed = false;
	bool			m_RightControllerWasPressed = false;
	vr::VRControllerState_t m_LeftControllerState;
	vr::VRControllerState_t m_RightControllerState;
	int				m_leftControllerPulseDur;
	int				m_rightControllerPulseDur;

	bool			m_HasHeadPose = false;
	idVec3			m_HeadOrigin;
	idMat3			m_HeadAxis;
	bool			m_HadHead = false;
	idVec3			m_HeadLastOrigin;
	idVec3			m_HeadMoveDelta;

	bool			m_HasLeftControllerPose = false;
	idVec3			m_LeftControllerOrigin;
	idMat3			m_LeftControllerAxis;

	bool			m_HasRightControllerPose = false;
	idVec3			m_RightControllerOrigin;
	idMat3			m_RightControllerAxis;

	bool			g_poseReset = false;

	int				m_UIEventIndex;
	int				m_UIEventCount = 0;
	sysEvent_t		m_UIEvents[MAX_VREVENTS];

	int				m_GameEventCount;
	struct
	{
		int action;
		int value;
	} m_GameEvents[MAX_VREVENTS];
};

void VRSystem::Init()
{
	// TODO API check

	vrSystem = new VRSystem_Valve();
	vrSystem->InitHMD();
}

void VRSystem_Valve::ConvertMatrix( const vr::HmdMatrix34_t& poseMat, idVec3& origin, idMat3& axis )
{
	origin.Set(
		m_ScaleX * poseMat.m[2][3],
		m_ScaleY * poseMat.m[0][3],
		m_ScaleZ * poseMat.m[1][3] );

	axis[0].Set( poseMat.m[2][2], poseMat.m[0][2], -poseMat.m[1][2] );
	axis[1].Set( poseMat.m[2][0], poseMat.m[0][0], -poseMat.m[1][0] );
	axis[2].Set( -poseMat.m[2][1], -poseMat.m[0][1], poseMat.m[1][1] );
}

idStr VRSystem_Valve::GetTrackedDeviceString( vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError* peError )
{
	uint32_t unRequiredBufferLen = vr::VRSystem()->GetStringTrackedDeviceProperty( unDevice, prop, NULL, 0, peError );
	if( unRequiredBufferLen == 0 )
	{
		return "";
	}

	char* pchBuffer = new char[ unRequiredBufferLen ];
	unRequiredBufferLen = vr::VRSystem()->GetStringTrackedDeviceProperty( unDevice, prop, pchBuffer, unRequiredBufferLen, peError );
	idStr sResult( pchBuffer );
	delete [] pchBuffer;
	return sResult;
}

bool VRSystem_Valve::InitHMD()
{
	if( vr_enable.GetInteger() == 0 )
	{
		openVREnabled = false;
		return false;
	}

	vr::EVRInitError error = vr::VRInitError_None;
	hmd = vr::VR_Init( &error, vr::VRApplication_Scene );
	if( error != vr::VRInitError_None )
	{
		common->Printf( "VR initialization failed: %s\n", vr::VR_GetVRInitErrorAsEnglishDescription( error ) );
		openVREnabled = false;

		return false;
	}

	if( !vr::VRCompositor() )
	{
		common->Printf( "VR compositor not present.\n" );
		openVREnabled = false;

		return false;
	}

	//vr::VRCompositor()->ForceInterleavedReprojectionOn( true );

	openVREnabled = true;

	UpdateResolution();

	hmd->GetProjectionRaw( vr::Eye_Right,
						   &openVRfovEye[1][0], &openVRfovEye[1][1],
						   &openVRfovEye[1][2], &openVRfovEye[1][3] );

	hmd->GetProjectionRaw( vr::Eye_Left,
						   &openVRfovEye[0][0], &openVRfovEye[0][1],
						   &openVRfovEye[0][2], &openVRfovEye[0][3] );

	openVRScreenSeparation =
		0.5f * ( openVRfovEye[1][1] + openVRfovEye[1][0] )
		/ ( openVRfovEye[1][1] - openVRfovEye[1][0] )
		- 0.5f * ( openVRfovEye[0][1] + openVRfovEye[0][0] )
		/ ( openVRfovEye[0][1] - openVRfovEye[0][0] );

	vr::HmdMatrix34_t mat;

#if 0
	mat = hmd->GetEyeToHeadTransform( vr::Eye_Left );
	Convert4x3Matrix( &mat, hmdEyeLeft );
	MatrixRTInverse( hmdEyeLeft );
#endif

	mat = hmd->GetEyeToHeadTransform( vr::Eye_Right );
#if 0
	Convert4x3Matrix( &mat, hmdEyeRight );
	MatrixRTInverse( hmdEyeRight );
#endif

	openVRUnscaledHalfIPD = mat.m[0][3];
	openVRUnscaledEyeForward = -mat.m[2][3];
	UpdateScaling();

	openVRSeated = true;
	m_leftController = vr::k_unTrackedDeviceIndexInvalid;
	m_rightController = vr::k_unTrackedDeviceIndexInvalid;
	UpdateControllers();

	vr::VRCompositor()->SetTrackingSpace( vr::TrackingUniverseStanding );
	ConvertMatrix( hmd->GetSeatedZeroPoseToStandingAbsoluteTrackingPose(), m_seatedOrigin, m_seatedAxis );
	m_seatedAxisInverse = m_seatedAxis.Inverse();


	int hmdHz = ( int )( hmd->GetFloatTrackedDeviceProperty( vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_DisplayFrequency_Float ) + 0.5f );
	cvarSystem->SetCVarString( "r_swapInterval", "0" );
	cvarSystem->SetCVarInteger( "com_engineHz", hmdHz );

	// RB: avoid waiting when WaitGetPoses is called
	vr::VRCompositor()->SetExplicitTimingMode( vr::VRCompositorTimingMode_Explicit_RuntimePerformsPostPresentHandoff );

	return true;
}

void VRSystem_Valve::ResetPose()
{
	g_poseReset = true;
	//hmd->ResetSeatedZeroPose();
}

void VRSystem_Valve::LogDevices()
{
	int axisType;
	const char* axisTypeString;

	idStr modelNumberString = GetTrackedDeviceString( vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_ModelNumber_String );

	common->Printf( "\nhead  model \"%s\"\n", modelNumberString );

	if( m_leftController != vr::k_unTrackedDeviceIndexInvalid )
	{
		modelNumberString = GetTrackedDeviceString( m_leftController, vr::Prop_ModelNumber_String );

		axisType = hmd->GetInt32TrackedDeviceProperty( m_leftController, vr::Prop_Axis0Type_Int32 );
		axisTypeString = hmd->GetControllerAxisTypeNameFromEnum( ( vr::EVRControllerAxisType )axisType );
		common->Printf( "left  model \"%s\" axis %s\n", modelNumberString, axisTypeString );
	}
	else
	{
		common->Printf( "left  not detected\n" );
	}

	if( m_rightController != vr::k_unTrackedDeviceIndexInvalid )
	{
		modelNumberString = GetTrackedDeviceString( m_rightController, vr::Prop_ModelNumber_String );

		axisType = hmd->GetInt32TrackedDeviceProperty( m_rightController, vr::Prop_Axis0Type_Int32 );
		axisTypeString = hmd->GetControllerAxisTypeNameFromEnum( ( vr::EVRControllerAxisType )axisType );
		common->Printf( "right model \"%s\" axis %s\n", modelNumberString, axisTypeString );
	}
	else
	{
		common->Printf( "right not detected\n" );
	}
}



void VRSystem_Valve::ClearEvents()
{
	m_UIEventIndex = 0;
	m_UIEventCount = 0;
	m_GameEventCount = 0;
	m_LeftControllerWasPressed = false;
	m_RightControllerWasPressed = false;
}

void VRSystem_Valve::UIEventQue( sysEventType_t type, int value, int value2 )
{
	assert( m_UIEventCount < MAX_VREVENTS );
	sysEvent_t* ev = &m_UIEvents[m_UIEventCount++];

	ev->evType = type;
	ev->evValue = value;
	ev->evValue2 = value2;
	ev->evPtrLength = 0;
	ev->evPtr = NULL;
	ev->inputDevice = 0;
}

const sysEvent_t& VRSystem_Valve::UIEventNext()
{
	assert( m_UIEventIndex < MAX_VREVENTS );
	if( m_UIEventIndex >= m_UIEventCount )
	{
		sysEvent_t& ev = m_UIEvents[m_UIEventIndex];
		ev.evType = SE_NONE;
		return ev;
	}

	return m_UIEvents[m_UIEventIndex++];
}

int VRSystem_Valve::PollGameInputEvents()
{
	return m_GameEventCount;
}

void VRSystem_Valve::GameEventQue( int action, int value )
{
	assert( m_GameEventCount < MAX_VREVENTS );
	m_GameEvents[m_GameEventCount].action = action;
	m_GameEvents[m_GameEventCount].value = value;
	m_GameEventCount++;
}

int VRSystem_Valve::ReturnGameInputEvent( const int n, int& action, int& value )
{
	if( n < 0 || n > m_GameEventCount )
	{
		return 0;
	}
	action = m_GameEvents[n].action;
	value = m_GameEvents[n].value;
	return 1;
}

idCVar vr_leftAxis( "vr_leftAxis", "0", CVAR_INTEGER | CVAR_ARCHIVE | CVAR_NEW, "left axis mode" );
idCVar vr_rightAxis( "vr_rightAxis", "4", CVAR_INTEGER | CVAR_ARCHIVE | CVAR_NEW, "right axis mode" );

int VRSystem_Valve::AxisToDPad( int mode, float x, float y )
{
	int dir;
	switch( mode )
	{
		case 3:
			if( y >= 0 )
			{
				dir = 1; // up
			}
			else
			{
				dir = 3; // down
			}
			break;
		case 4:
			if( x <= 0 )
			{
				dir = 0; // left
			}
			else
			{
				dir = 2; // right
			}
			break;
		case 5:
			if( x < y )
			{
				if( x > -y )
				{
					dir = 1; // up
				}
				else
				{
					dir = 0; // left
				}
			}
			else
			{
				if( x > -y )
				{
					dir = 2; // right
				}
				else
				{
					dir = 3; // down
				}
			}
			break;
		default:
			dir = -1;
	}
	return dir;
}

void VRSystem_Valve::GenButtonEvent( uint32_t button, bool left, bool pressed )
{
	switch( button )
	{
		case vr::k_EButton_ApplicationMenu:
			if( left )
			{
				GameEventQue( K_VR_LEFT_MENU, pressed );
				UIEventQue( SE_KEY, K_JOY10, pressed ); // pda
			}
			else
			{
				GameEventQue( K_VR_RIGHT_MENU, pressed );
				UIEventQue( SE_KEY, K_JOY9, pressed ); // pause menu
			}
			break;

		case vr::k_EButton_Grip:
			if( left )
			{
				GameEventQue( K_VR_LEFT_GRIP, pressed );
				UIEventQue( SE_KEY, K_JOY5, pressed ); // prev pda menu
			}
			else
			{
				GameEventQue( K_VR_RIGHT_GRIP, pressed );
				UIEventQue( SE_KEY, K_JOY6, pressed ); // next pda menu
			}
			break;

		case vr::k_EButton_SteamVR_Trigger:
			if( left )
			{
				GameEventQue( K_VR_LEFT_TRIGGER, pressed );
				UIEventQue( SE_KEY, K_JOY2, pressed ); // menu back
			}
			else
			{
				GameEventQue( K_VR_RIGHT_TRIGGER, pressed );
				UIEventQue( SE_KEY, K_MOUSE1, pressed ); // cursor click
			}
			break;

		case vr::k_EButton_SteamVR_Touchpad:
			if( left )
			{
				//VR_UIEventQue( SE_KEY, K_JOY2, pressed ); // menu back
				static keyNum_t uiLastKey;
				if( pressed )
				{
					if( m_LeftControllerState.rAxis[0].x < m_LeftControllerState.rAxis[0].y )
					{
						if( m_LeftControllerState.rAxis[0].x > -m_LeftControllerState.rAxis[0].y )
						{
							uiLastKey = K_JOY_STICK1_UP;
						}
						else
						{
							uiLastKey = K_JOY_STICK1_LEFT;
						}
					}
					else
					{
						if( m_LeftControllerState.rAxis[0].x > -m_LeftControllerState.rAxis[0].y )
						{
							uiLastKey = K_JOY_STICK1_RIGHT;
						}
						else
						{
							uiLastKey = K_JOY_STICK1_DOWN;
						}
					}

					UIEventQue( SE_KEY, uiLastKey, 1 );
				}
				else
				{
					UIEventQue( SE_KEY, uiLastKey, 0 );
				}

				GameEventQue( K_VR_LEFT_AXIS, pressed );
				if( pressed )
				{
					m_LeftControllerWasPressed = true;
				}

				if( !openVRLeftTouchpad )
				{
					break;
				}

				// dpad modes
				static int gameLeftLastKey;
				if( pressed )
				{
					int dir = AxisToDPad( vr_leftAxis.GetInteger(), m_LeftControllerState.rAxis[0].x, m_LeftControllerState.rAxis[0].y );
					if( dir != -1 )
					{
						gameLeftLastKey = K_VR_LEFT_DPAD_LEFT + dir;
						GameEventQue( gameLeftLastKey, 1 );
					}
					else
					{
						gameLeftLastKey = K_NONE;
					}
				}
				else if( gameLeftLastKey != K_NONE )
				{
					GameEventQue( gameLeftLastKey, 0 );
					gameLeftLastKey = K_NONE;
				}
			}
			else
			{
				UIEventQue( SE_KEY, K_JOY1, pressed ); // menu select
				GameEventQue( K_VR_RIGHT_AXIS, pressed );

				if( pressed )
				{
					m_RightControllerWasPressed = true;
				}

				if( !openVRRightTouchpad )
				{
					break;
				}

				// dpad modes
				static int gameRightLastKey;
				if( pressed )
				{
					int dir = AxisToDPad( vr_rightAxis.GetInteger(), m_RightControllerState.rAxis[0].x, m_RightControllerState.rAxis[0].y );
					if( dir != -1 )
					{
						gameRightLastKey = K_VR_RIGHT_DPAD_LEFT + dir;
						GameEventQue( gameRightLastKey, 1 );
					}
					else
					{
						gameRightLastKey = K_NONE;
					}
				}
				else if( gameRightLastKey != K_NONE )
				{
					GameEventQue( gameRightLastKey, 0 );
					gameRightLastKey = K_NONE;
				}
			}
			break;

		case vr::k_EButton_A:
			if( left )
			{
				GameEventQue( K_VR_LEFT_A, pressed );
			}
			else
			{
				GameEventQue( K_VR_RIGHT_A, pressed );
			}
			break;
		default:
			break;
	}
}

void VRSystem_Valve::GenJoyAxisEvents()
{
	if( m_leftController != vr::k_unTrackedDeviceIndexInvalid )
	{
		vr::VRControllerState_t& state = m_LeftControllerState;
		hmd->GetControllerState( m_leftController, &state, sizeof( state ) );

		// dpad modes
		if( !openVRLeftTouchpad )
		{
			static int gameLeftLastKey;

			if( state.rAxis[0].x * state.rAxis[0].x + state.rAxis[0].y * state.rAxis[0].y > 0.25f )
			{
				int dir = AxisToDPad( vr_leftAxis.GetInteger(), m_LeftControllerState.rAxis[0].x, m_LeftControllerState.rAxis[0].y );
				if( dir != -1 )
				{
					gameLeftLastKey = K_VR_LEFT_DPAD_LEFT + dir;
					GameEventQue( gameLeftLastKey, 1 );
				}
				else
				{
					gameLeftLastKey = K_NONE;
				}
			}
			else if( gameLeftLastKey != K_NONE )
			{
				GameEventQue( gameLeftLastKey, 0 );
				gameLeftLastKey = K_NONE;
			}
		}
	}

	if( m_rightController != vr::k_unTrackedDeviceIndexInvalid )
	{
		vr::VRControllerState_t& state = m_RightControllerState;
		hmd->GetControllerState( m_rightController, &state, sizeof( state ) );

		// dpad modes
		if( !openVRRightTouchpad )
		{
			static int gameRightLastKey;

			if( state.rAxis[0].x * state.rAxis[0].x + state.rAxis[0].y * state.rAxis[0].y > 0.25f )
			{
				int dir = AxisToDPad( vr_rightAxis.GetInteger(), m_RightControllerState.rAxis[0].x, m_RightControllerState.rAxis[0].y );
				if( dir != -1 )
				{
					gameRightLastKey = K_VR_RIGHT_DPAD_LEFT + dir;
					GameEventQue( gameRightLastKey, 1 );
				}
				else
				{
					gameRightLastKey = K_NONE;
				}
			}
			else if( gameRightLastKey != K_NONE )
			{
				GameEventQue( gameRightLastKey, 0 );
				gameRightLastKey = K_NONE;
			}
		}
	}
}

void VRSystem_Valve::GenMouseEvents()
{
	// virtual head tracking mouse for shell UI
	idVec3 shellOrigin;
	idMat3 shellAxis;

	if( m_HadHead && tr.guiModel->GetVRShell( shellOrigin, shellAxis ) )
	{
		const float virtualWidth = renderSystem->GetVirtualWidth();
		const float virtualHeight = renderSystem->GetVirtualHeight();
		float guiHeight = 12 * 5.3f;
		float guiScale = guiHeight / virtualHeight;
		float guiWidth = virtualWidth * guiScale;
		float guiForward = guiHeight + 12.f;
		idVec3 upperLeft = shellOrigin
						   + shellAxis[0] * guiForward
						   + shellAxis[1] * 0.5f * guiWidth
						   + shellAxis[2] * 0.5f * guiHeight;
		idMat3 invShellAxis = shellAxis.Inverse();
		idVec3 rayStart = ( m_HeadOrigin - upperLeft ) * invShellAxis;
		idVec3 rayDir = m_HeadAxis[0] * invShellAxis;

		if( rayDir.x != 0 )
		{
			static int oldX, oldY;
			float wx = rayStart.y - rayStart.x * rayDir.y / rayDir.x;
			float wy = rayStart.z - rayStart.x * rayDir.z / rayDir.x;
			int x = -wx * renderSystem->GetWidth() / guiWidth;
			int y = -wy * renderSystem->GetHeight() / guiHeight;
			if( x >= 0 && x < renderSystem->GetWidth() &&
					y >= 0 && y < renderSystem->GetHeight() &&
					( x != oldX || y != oldY ) )
			{
				oldX = x;
				oldY = y;

				UIEventQue( SE_MOUSE_ABSOLUTE, x, y );
			}
		}
	}
}



bool VRSystem_Valve::ConvertPose( const vr::TrackedDevicePose_t& pose, idVec3& origin, idMat3& axis )
{
	if( !pose.bPoseIsValid )
	{
		return false;
	}

	ConvertMatrix( pose.mDeviceToAbsoluteTracking, origin, axis );

	return true;
}

void VRSystem_Valve::UpdateResolution()
{
	vr_resolutionScale.ClearModified();

	float scale = vr_resolutionScale.GetFloat();
	uint32_t width, height;
	hmd->GetRecommendedRenderTargetSize( &width, &height );
	width = width * scale;
	height = height * scale;

	if( width < 540 )
	{
		width = 640;
	}
	else if( width > 8000 )
	{
		width = 8000;
	}

	if( height < 540 )
	{
		height = 480;
	}
	else if( height > 8000 )
	{
		height = 8000;
	}

	hmdWidth = width;
	hmdHeight = height;
}

void VRSystem_Valve::UpdateScaling()
{
	const float m2i = 1 / 0.0254f; // meters to inches
	const float cm2i = 1 / 2.54f; // centimeters to inches
	float ratio = 76.5f / ( vr_playerHeightCM.GetFloat() * cm2i ); // converts player height to character height
	openVRScale = m2i * ratio;
	openVRHalfIPD = openVRUnscaledHalfIPD * openVRScale;
	openVREyeForward = openVRUnscaledEyeForward * openVRScale;
	m_ScaleX = -openVRScale;
	m_ScaleY = -openVRScale;
	m_ScaleZ = openVRScale;
}

void VRSystem_Valve::UpdateControllers()
{
	if( vr_forceGamepad.GetBool() )
	{
		m_leftController = vr::k_unTrackedDeviceIndexInvalid;
		m_rightController = vr::k_unTrackedDeviceIndexInvalid;
		return;
	}

	bool hadLeft = m_leftController != vr::k_unTrackedDeviceIndexInvalid;
	bool hadRight = m_rightController != vr::k_unTrackedDeviceIndexInvalid;

	m_leftController = hmd->GetTrackedDeviceIndexForControllerRole( vr::TrackedControllerRole_LeftHand );
	m_rightController = hmd->GetTrackedDeviceIndexForControllerRole( vr::TrackedControllerRole_RightHand );

	if( hadLeft && m_leftController == vr::k_unTrackedDeviceIndexInvalid )
	{
		common->Printf( "left controller lost\n" );
	}
	if( hadRight && m_rightController == vr::k_unTrackedDeviceIndexInvalid )
	{
		common->Printf( "right controller lost\n" );
	}

	if( m_leftController != vr::k_unTrackedDeviceIndexInvalid
			|| m_rightController != vr::k_unTrackedDeviceIndexInvalid )
	{
		if( openVRSeated )
		{
			openVRSeated = false;

			int axisType;

			idStr modelNumberString = GetTrackedDeviceString( m_leftController, vr::Prop_ModelNumber_String );

			if( idStr::Icmp( modelNumberString, "Hydra" ) == 0 )
			{
				openVRLeftTouchpad = 0;
			}
			else
			{
				axisType = hmd->GetInt32TrackedDeviceProperty( m_leftController, vr::Prop_Axis0Type_Int32 );
				openVRLeftTouchpad = ( axisType == vr::k_eControllerAxis_TrackPad ) ? 1 : 0;
			}

			modelNumberString = GetTrackedDeviceString( m_rightController, vr::Prop_ModelNumber_String );

			if( idStr::Icmp( modelNumberString, "Hydra" ) == 0 )
			{
				openVRRightTouchpad = 0;
			}
			else
			{
				axisType = hmd->GetInt32TrackedDeviceProperty( m_rightController, vr::Prop_Axis0Type_Int32 );
				openVRRightTouchpad = ( axisType == vr::k_eControllerAxis_TrackPad ) ? 1 : 0;
			}
		}
	}
	else
	{
		if( !openVRSeated )
		{
			openVRSeated = true;
		}
	}
}

void VRSystem_Valve::SubmitStereoRenders( nvrhi::ICommandList* commandList, idImage* image0, idImage* image1 )
{
#if 0
	GL_ViewportAndScissor( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight() );

	vr::Texture_t leftEyeTexture = {( void* )left, vr::API_OpenGL, vr::ColorSpace_Gamma };
	vr::VRCompositor()->Submit( vr::Eye_Left, &leftEyeTexture );
	vr::Texture_t rightEyeTexture = {( void* )right, vr::API_OpenGL, vr::ColorSpace_Gamma };
	vr::VRCompositor()->Submit( vr::Eye_Right, &rightEyeTexture );
#endif

	if( deviceManager->GetGraphicsAPI() == nvrhi::GraphicsAPI::VULKAN )
	{
		nvrhi::IDevice* device = deviceManager->GetDevice();

		vr::VRVulkanTextureData_t vulkanData;
		nvrhi::ITexture* nativeTexture = image0->GetTextureHandle();

		vulkanData.m_nImage = ( uint64_t )( void* )nativeTexture->getNativeObject( nvrhi::ObjectTypes::VK_Image );
		vulkanData.m_pDevice = ( VkDevice_T* ) device->getNativeObject( nvrhi::ObjectTypes::VK_Device );
		vulkanData.m_pPhysicalDevice = ( VkPhysicalDevice_T* ) device->getNativeObject( nvrhi::ObjectTypes::VK_PhysicalDevice );
		vulkanData.m_pInstance = ( VkInstance_T* ) device->getNativeObject( nvrhi::ObjectTypes::VK_Instance );
		vulkanData.m_pQueue = ( VkQueue_T* ) device->getNativeQueue( nvrhi::ObjectTypes::VK_Queue, nvrhi::CommandQueue::Graphics );
		vulkanData.m_nQueueFamilyIndex = deviceManager->GetGraphicsFamilyIndex();

		vulkanData.m_nWidth = image0->GetUploadWidth();
		vulkanData.m_nHeight = image0->GetUploadHeight();
		vulkanData.m_nFormat = VK_FORMAT_R8G8B8A8_UNORM;
		vulkanData.m_nSampleCount = 1;

		vr::Texture_t leftEyeTexture = { ( void* )& vulkanData, vr::TextureType_Vulkan, vr::ColorSpace_Auto };
		vr::VRCompositor()->Submit( vr::Eye_Left, &leftEyeTexture );

		nativeTexture = image1->GetTextureHandle();
		vulkanData.m_nImage = ( uint64_t )( void* )nativeTexture->getNativeObject( nvrhi::ObjectTypes::VK_Image );
		vulkanData.m_nWidth = image1->GetUploadWidth();
		vulkanData.m_nHeight = image1->GetUploadHeight();
		vulkanData.m_nFormat = VK_FORMAT_R8G8B8A8_UNORM;
		vulkanData.m_nSampleCount = 1;

		vr::Texture_t rightEyeTexture = { ( void* )& vulkanData, vr::TextureType_Vulkan, vr::ColorSpace_Auto };
		vr::VRCompositor()->Submit( vr::Eye_Right, &rightEyeTexture );
	}
	else
	{
		vr::D3D12TextureData_t d3d12LeftEyeTexture;

		nvrhi::ITexture* nativeTexture = image0->GetTextureHandle();
		d3d12LeftEyeTexture.m_pResource = nativeTexture->getNativeObject( nvrhi::ObjectTypes::D3D12_Resource );
		d3d12LeftEyeTexture.m_pCommandQueue = commandList->getNativeObject( nvrhi::ObjectTypes::D3D12_GraphicsCommandList );
		d3d12LeftEyeTexture.m_nNodeMask = 0;

		vr::Texture_t leftEyeTexture = { ( void* )& d3d12LeftEyeTexture, vr::TextureType_DirectX12, vr::ColorSpace_Auto };
		vr::VRCompositor()->Submit( vr::Eye_Left, &leftEyeTexture );


		vr::D3D12TextureData_t d3d12RightEyeTexture;

		nativeTexture = image1->GetTextureHandle();
		d3d12RightEyeTexture.m_pResource = nativeTexture->getNativeObject( nvrhi::ObjectTypes::D3D12_Resource );
		d3d12RightEyeTexture.m_pCommandQueue = commandList->getNativeObject( nvrhi::ObjectTypes::D3D12_GraphicsCommandList );
		d3d12RightEyeTexture.m_nNodeMask = 0;

		vr::Texture_t rightEyeTexture = { ( void* )& d3d12RightEyeTexture, vr::TextureType_DirectX12, vr::ColorSpace_Auto };

		vr::VRCompositor()->Submit( vr::Eye_Right, &rightEyeTexture );
	}
}

void VRSystem_Valve::PreSwap()
{
	// https://github.com/ValveSoftware/openvr/wiki/Vulkan#explicit-timing

	// Explicit Timing: sync the frame data with the compositor
	vr::VRCompositor()->SubmitExplicitTimingData();
}

void VRSystem_Valve::PostSwap()
{
	// tell the compositor that we are done rendering
	vr::VRCompositor()->PostPresentHandoff();

	vr::TrackedDevicePose_t rTrackedDevicePose[ vr::k_unMaxTrackedDeviceCount ];

	{
		OPTICK_CATEGORY( "VR_WaitGetPoses", Optick::Category::Wait );

		vr::VRCompositor()->WaitGetPoses( rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0 );
	}

	UpdateControllers();

	if( vr_playerHeightCM.IsModified() )
	{
		vr_playerHeightCM.ClearModified();
		UpdateScaling();
	}

	if( vr_seated.IsModified() )
	{
		vr_seated.ClearModified();
		tr.guiModel->UpdateVRShell();
	}

	vr::TrackedDevicePose_t& hmdPose = rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd];
	m_HasHeadPose = hmdPose.bPoseIsValid;

	if( hmdPose.bPoseIsValid )
	{
		ConvertPose( hmdPose, m_HeadOrigin, m_HeadAxis );
		m_HeadOrigin += openVREyeForward * m_HeadAxis[0];

		if( m_HadHead )
		{
			m_HeadMoveDelta = m_HeadOrigin - m_HeadLastOrigin;
			m_HeadLastOrigin = m_HeadOrigin;
		}
		else
		{
			m_HadHead = true;
			m_HeadMoveDelta.Zero();
		}
	}
	else
	{
		m_HadHead = false;
	}

	if( vr_forceGamepad.GetBool() )
	{
		m_HasLeftControllerPose = false;
		m_HasRightControllerPose = false;
	}
	else
	{
		if( m_leftController != vr::k_unTrackedDeviceIndexInvalid )
		{
			if( m_leftControllerPulseDur > 500 )
			{
				hmd->TriggerHapticPulse( m_leftController, 0, m_leftControllerPulseDur );
			}
			m_leftControllerPulseDur = 0;

			static bool hadLeftPose;
			vr::TrackedDevicePose_t& handPose = rTrackedDevicePose[m_leftController];
			if( handPose.bPoseIsValid )
			{
				m_HasLeftControllerPose = true;
				ConvertPose( handPose, m_LeftControllerOrigin, m_LeftControllerAxis );
				hadLeftPose = true;
			}
			else if( hadLeftPose )
			{
				hadLeftPose = false;
				common->Printf( "left controller had no pose\n" );
			}
		}

		if( m_rightController != vr::k_unTrackedDeviceIndexInvalid )
		{
			if( m_rightControllerPulseDur > 500 )
			{
				hmd->TriggerHapticPulse( m_rightController, 0, m_rightControllerPulseDur );
			}
			m_rightControllerPulseDur = 0;

			static bool hadRightPose;
			vr::TrackedDevicePose_t& handPose = rTrackedDevicePose[m_rightController];

			if( handPose.bPoseIsValid )
			{
				m_HasRightControllerPose = true;
				ConvertPose( handPose, m_RightControllerOrigin, m_RightControllerAxis );
				hadRightPose = true;
			}
			else if( hadRightPose )
			{
				hadRightPose = false;
				common->Printf( "right controller had no pose\n" );
			}
		}
	}

	ClearEvents();

	GenJoyAxisEvents();

	vr::VREvent_t e;
	while( hmd->PollNextEvent( &e, sizeof( e ) ) )
	{
		//vr::ETrackedControllerRole role;

		switch( e.eventType )
		{
			/*case vr::VREvent_TrackedDeviceActivated:
				role = hmd->GetControllerRoleForTrackedDeviceIndex(e.trackedDeviceIndex);
				switch(role)
				{
				case vr::TrackedControllerRole_LeftHand:
					m_leftController = e.trackedDeviceIndex;
					break;
				case vr::TrackedControllerRole_RightHand:
					m_rightController = e.trackedDeviceIndex;
					break;
				}
				break;
			case vr::VREvent_TrackedDeviceDeactivated:
				if (e.trackedDeviceIndex == m_leftController)
				{
					m_leftController = vr::k_unTrackedDeviceIndexInvalid;
				}
				else if (e.trackedDeviceIndex == m_rightController)
				{
					m_rightController = vr::k_unTrackedDeviceIndexInvalid;
				}
				break;*/
			case vr::VREvent_ButtonPress:
				if( e.trackedDeviceIndex == m_leftController || e.trackedDeviceIndex == m_rightController )
				{
					GenButtonEvent( e.data.controller.button, e.trackedDeviceIndex == m_leftController, true );
				}
				break;
			case vr::VREvent_ButtonUnpress:
				if( e.trackedDeviceIndex == m_leftController || e.trackedDeviceIndex == m_rightController )
				{
					GenButtonEvent( e.data.controller.button, e.trackedDeviceIndex == m_leftController, false );
				}
				break;
		}
	}

	if( !openVRSeated )
	{
		GenMouseEvents();
	}

	if( g_poseReset )
	{
		g_poseReset = false;
		ConvertMatrix( hmd->GetSeatedZeroPoseToStandingAbsoluteTrackingPose(), m_seatedOrigin, m_seatedAxis );
		m_seatedAxisInverse = m_seatedAxis.Inverse();
		tr.guiModel->UpdateVRShell();
	}
}

bool VRSystem_Valve::CalculateView( idVec3& origin, idMat3& axis, const idVec3& eyeOffset, bool overridePitch )
{
	if( !m_HasHeadPose )
	{
		return false;
	}

	if( overridePitch )
	{
		float pitch = idMath::M_RAD2DEG * asin( axis[0][2] );
		idAngles angles( pitch, 0, 0 );
		axis = angles.ToMat3() * axis;
	}

	if( !vr_seated.GetBool() )
	{
		origin.z -= eyeOffset.z;
		// ignore x and y
		origin += axis[2] * m_HeadOrigin.z;
	}
	else
	{
		origin += axis * m_HeadOrigin;
	}

	axis = m_HeadAxis * axis;

	return true;
}

bool VRSystem_Valve::GetHead( idVec3& origin, idMat3& axis )
{
	if( !m_HasHeadPose )
	{
		return false;
	}

	origin = m_HeadOrigin;
	axis = m_HeadAxis;

	return true;
}

// returns left controller position relative to the head
bool VRSystem_Valve::GetLeftController( idVec3& origin, idMat3& axis )
{
	if( !m_HasLeftControllerPose || !m_HasHeadPose )
	{
		return false;
	}

	origin = m_LeftControllerOrigin;
	axis = m_LeftControllerAxis;

	return true;
}

// returns right controller position relative to the head
bool VRSystem_Valve::GetRightController( idVec3& origin, idMat3& axis )
{
	if( !m_HasRightControllerPose || !m_HasHeadPose )
	{
		return false;
	}

	origin = m_RightControllerOrigin;
	axis = m_RightControllerAxis;

	return true;
}

void VRSystem_Valve::MoveDelta( idVec3& delta, float& height )
{
	if( !m_HasHeadPose )
	{
		height = 0.f;
		delta.Set( 0, 0, 0 );
		return;
	}

	height = m_HeadOrigin.z;

	delta.x = m_HeadMoveDelta.x;
	delta.y = m_HeadMoveDelta.y;
	delta.z = 0.f;

	m_HeadMoveDelta.Zero();
}

void VRSystem_Valve::HapticPulse( int leftDuration, int rightDuration )
{
	if( leftDuration > m_leftControllerPulseDur )
	{
		m_leftControllerPulseDur = leftDuration;
	}

	if( rightDuration > m_rightControllerPulseDur )
	{
		m_rightControllerPulseDur = rightDuration;
	}
}

extern idCVar joy_deadZone;

bool VRSystem_Valve::GetLeftControllerAxis( idVec2& axis )
{
	if( m_leftController == vr::k_unTrackedDeviceIndexInvalid )
	{
		return false;
	}

	uint64_t mask = vr::ButtonMaskFromId( vr::k_EButton_SteamVR_Touchpad );

	if( openVRLeftTouchpad )
	{
		if( !( m_LeftControllerState.ulButtonTouched & mask ) )
		{
			return false;
		}
	}
	else
	{
		const float threshold =			joy_deadZone.GetFloat();
		if( fabs( m_LeftControllerState.rAxis[0].x ) < threshold &&
				fabs( m_LeftControllerState.rAxis[0].y ) < threshold )
		{
			return false;
		}
	}

	axis.x = m_LeftControllerState.rAxis[0].x;
	axis.y = m_LeftControllerState.rAxis[0].y;

	return true;
}

bool VRSystem_Valve::GetRightControllerAxis( idVec2& axis )
{
	if( m_rightController == vr::k_unTrackedDeviceIndexInvalid )
	{
		return false;
	}

	uint64_t mask = vr::ButtonMaskFromId( vr::k_EButton_SteamVR_Touchpad );

	if( openVRRightTouchpad )
	{
		if( !( m_RightControllerState.ulButtonTouched & mask ) )
		{
			return false;
		}
	}
	else
	{
		const float threshold =			joy_deadZone.GetFloat();
		if( fabs( m_RightControllerState.rAxis[0].x ) < threshold &&
				fabs( m_RightControllerState.rAxis[0].y ) < threshold )
		{
			return false;
		}
	}

	axis.x = m_RightControllerState.rAxis[0].x;
	axis.y = m_RightControllerState.rAxis[0].y;

	return true;
}

bool VRSystem_Valve::LeftControllerWasPressed()
{
	return m_LeftControllerWasPressed;
}

bool VRSystem_Valve::LeftControllerIsPressed()
{
	static uint64_t mask = vr::ButtonMaskFromId( vr::k_EButton_SteamVR_Touchpad );
	return ( m_LeftControllerState.ulButtonPressed & mask ) != 0;
}

bool VRSystem_Valve::RightControllerWasPressed()
{
	return m_RightControllerWasPressed;
}

bool VRSystem_Valve::RightControllerIsPressed()
{
	static uint64_t mask = vr::ButtonMaskFromId( vr::k_EButton_SteamVR_Touchpad );
	return ( m_RightControllerState.ulButtonPressed & mask ) != 0;
}

const idVec3& VRSystem_Valve::GetSeatedOrigin()
{
	return m_seatedOrigin;
}

const idMat3& VRSystem_Valve::GetSeatedAxis()
{
	return m_seatedAxis;
}

const idMat3& VRSystem_Valve::GetSeatedAxisInverse()
{
	return m_seatedAxisInverse;
}
