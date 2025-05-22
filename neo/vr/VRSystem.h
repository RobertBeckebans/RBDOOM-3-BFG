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
#ifndef __VR_SYSTEM_H
#define __VR_SYSTEM_H

/*
=============================================================

VR

=============================================================
*/

enum class VrAPI : uint8_t
{
	OPENVR,
	OPENXR
};

class VRSystem
{
public:
	static void				Init();

	virtual					~VRSystem() {}

	//virtual bool			HasHMD() const = 0;
	virtual bool			IsActive() const = 0;
	virtual bool			IsSeated() const = 0;

	virtual bool			InitHMD() = 0;
	virtual void			ShutdownHMD() = 0;
	virtual void			UpdateHMD() = 0;

	virtual void			ResetPose() = 0;
	virtual void			LogDevices() = 0;

	virtual void			UpdateResolution() = 0;
	//virtual void			UpdateScaling() = 0;
	//virtual void			UpdateControllers() = 0;

	virtual int				PollGameInputEvents() = 0;
	virtual int				ReturnGameInputEvent( const int n, int& action, int& value ) = 0;

	virtual void			SubmitStereoRenders( nvrhi::ICommandList* commandList, idImage* image0, idImage* image1 ) = 0;
	virtual void			PreSwap() = 0;
	virtual void			PostSwap() = 0;

	virtual idVec2i			GetRenderResolution() const = 0;
	virtual float			GetScreenSeparation() const = 0;
	virtual idVec4			GetFOV( int eye ) const = 0;
	virtual float			GetHalfIPD() const = 0;	// in world units instead of centimeters

	virtual bool			GetHead( idVec3& origin, idMat3& axis ) = 0;
	virtual bool			GetLeftController( idVec3& origin, idMat3& axis ) = 0;
	virtual bool			GetRightController( idVec3& origin, idMat3& axis ) = 0;
	//virtual void			MoveDelta( idVec3& delta, float& height ) = 0;
	virtual void			HapticPulse( int leftDuration, int rightDuration ) = 0;

	virtual bool			GetLeftControllerAxis( idVec2& axis ) = 0;
	virtual bool			GetRightControllerAxis( idVec2& axis ) = 0;

	virtual bool			HasLeftTouchpad() const = 0;
	virtual bool			LeftControllerWasPressed() = 0;
	virtual bool			LeftControllerIsPressed() = 0;

	virtual bool			HasRightTouchpad() const = 0;
	virtual bool			RightControllerWasPressed() = 0;
	virtual bool			RightControllerIsPressed() = 0;

	virtual const idVec3&	GetSeatedOrigin() = 0;
	virtual const idMat3&	GetSeatedAxis() = 0;
	virtual const idMat3&	GetSeatedAxisInverse() = 0;

	virtual const sysEvent_t&		UIEventNext() = 0;
};

extern VRSystem* vrSystem;

extern idCVar vr_enable;
extern idCVar vr_resolutionScale;
extern idCVar vr_playerHeightCM;
extern idCVar vr_aimLook;
extern idCVar vr_seated;
extern idCVar vr_forceGamepad;
extern idCVar vr_knockbackScale;
extern idCVar vr_strafing;
extern idCVar vr_forwardOnly;
extern idCVar vr_relativeAxis;
extern idCVar vr_responseCurve;
extern idCVar vr_moveMode;
extern idCVar vr_moveSpeed;

#endif // __VR_SYSTEM_H
