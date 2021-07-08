/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (c) 2016 Mattias Cibien

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
#include "../Game_local.h"

const static int NUM_MOD_OPTIONS = 8;

void idMenuScreen_Shell_Mods::Initialize(idMenuHandler* data)
{
	idMenuScreen::Initialize(data);
	
	if (data != NULL)
	{
		menuGUI = data->GetGUI();
	}
	
	SetSpritePath("menuSettings");
	
	options = new (TAG_SWF) idMenuWidget_DynamicList();
	options->SetNumVisibleOptions(NUM_MOD_OPTIONS);
	options->SetSpritePath(GetSpritePath(), "info", "options");
	options->SetWrappingAllowed(true);
	
	while(options->GetChildren().Num() < NUM_MOD_OPTIONS)
	{
		idMenuWidget_Button* const buttonWidget = new (TAG_SWF) idMenuWidget_Button();
		buttonWidget->AddEventAction(WIDGET_EVENT_PRESS).Set(WIDGET_ACTION_PRESS_FOCUSED, options->GetChildren().Num());
		buttonWidget->Initialize(data);
		options->AddChild(buttonWidget);
	}
	
	options->Initialize(data);
	
	AddChild(options);
	
	btnBack = new(TAG_SWF) idMenuWidget_Button();
	btnBack->Initialize(data);
	btnBack->SetLabel("MAIN MENU");
	btnBack->SetSpritePath(GetSpritePath(), "info", "btnBack");
	btnBack->AddEventAction(WIDGET_EVENT_PRESS).Set(WIDGET_ACTION_GO_BACK);
	
	AddChild(btnBack);
	
	SetupModOptions();
	options->AddEventAction(WIDGET_EVENT_SCROLL_DOWN).Set(new(TAG_SWF) idWidgetActionHandler(options, WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER, WIDGET_EVENT_SCROLL_DOWN));
	options->AddEventAction(WIDGET_EVENT_SCROLL_UP).Set(new(TAG_SWF) idWidgetActionHandler(options, WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER, WIDGET_EVENT_SCROLL_UP));
	options->AddEventAction(WIDGET_EVENT_SCROLL_DOWN_RELEASE).Set(new(TAG_SWF) idWidgetActionHandler(options, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_RELEASE));
	options->AddEventAction(WIDGET_EVENT_SCROLL_UP_RELEASE).Set(new(TAG_SWF) idWidgetActionHandler(options, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_RELEASE));
	options->AddEventAction(WIDGET_EVENT_SCROLL_DOWN_LSTICK).Set(new(TAG_SWF) idWidgetActionHandler(options, WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER, WIDGET_EVENT_SCROLL_DOWN_LSTICK));
	options->AddEventAction(WIDGET_EVENT_SCROLL_UP_LSTICK).Set(new(TAG_SWF) idWidgetActionHandler(options, WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER, WIDGET_EVENT_SCROLL_UP_LSTICK));
	options->AddEventAction(WIDGET_EVENT_SCROLL_DOWN_LSTICK_RELEASE).Set(new(TAG_SWF) idWidgetActionHandler(options, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_LSTICK_RELEASE));
	options->AddEventAction(WIDGET_EVENT_SCROLL_UP_LSTICK_RELEASE).Set(new(TAG_SWF) idWidgetActionHandler(options, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_LSTICK_RELEASE));
}

void idMenuScreen_Shell_Mods::Update()
{
	if (menuData != NULL)
	{
		idMenuWidget_CommandBar* cmdBar = menuData->GetCmdBar();
		if (cmdBar != NULL)
		{
			cmdBar->ClearAllButtons();
			idMenuWidget_CommandBar::buttonInfo_t* buttonInfo;
			buttonInfo = cmdBar->GetButton(idMenuWidget_CommandBar::BUTTON_JOY2);
			if (menuData->GetPlatform() != 2)
			{
				buttonInfo->label = "#str_00395";
			}
			buttonInfo->action.Set(WIDGET_ACTION_GO_BACK);
			
			buttonInfo = cmdBar->GetButton(idMenuWidget_CommandBar::BUTTON_JOY1);
			
			if (menuData->GetPlatform() != 2)
			{
				buttonInfo->label = "#str_SWF_SELECT";
			}
			buttonInfo->action.Set(WIDGET_ACTION_PRESS_FOCUSED);
		}
	}
	
	idSWFScriptObject& root = GetSWFObject()->GetRootObject();
	if (BindSprite(root))
	{
		idSWFTextInstance* heading = GetSprite()->GetScriptObject()->GetNestedText("info", "txtHeading");
		if (heading != NULL)
		{
			heading->SetText("MODS");
			heading->SetStrokeInfo(true, 0.75f, 1.75f);
		}
		
		idSWFSpriteInstance* gradient = GetSprite()->GetScriptObject()->GetNestedSprite("info", "gradient");
		if (gradient != NULL && heading != NULL)
		{
			gradient->SetXPos(heading->GetTextLength());
		}
		
		if (btnBack != NULL)
		{
			btnBack->BindSprite(root);
		}
		
		idMenuScreen::Update();
	}
}

void idMenuScreen_Shell_Mods::ShowScreen(const mainMenuTransition_t transitionType)
{
	idMenuScreen::ShowScreen(transitionType);
}

void idMenuScreen_Shell_Mods::HideScreen(const mainMenuTransition_t transitionType)
{
	idMenuScreen::HideScreen(transitionType);
}

bool idMenuScreen_Shell_Mods::HandleAction(idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled)
{
	if (menuData == NULL)
	{
		return true;
	}
	
	if (menuData->ActiveScreen() != SHELL_AREA_MODS)
	{
		return false;
	}
	
	widgetAction_t actionType = action.GetType();
	const idSWFParmList& params = action.GetParms();
	
	switch (actionType)
	{
	case WIDGET_ACTION_GO_BACK:
	{
		menuData->SetNextScreen(SHELL_AREA_ROOT, MENU_TRANSITION_SIMPLE);
		return true;
	}
	case WIDGET_ACTION_PRESS_FOCUSED:
	{
		if (options == NULL)
		{
			return true;
		}
		
		int selectionIndex = options->GetViewIndex();
		if (params.Num() == 1)
		{
			selectionIndex = params[0].ToInteger();
		}
		
		if (options->GetFocusIndex() != selectionIndex - options->GetViewOffset())
		{
			options->SetFocusIndex(selectionIndex);
			options->SetViewIndex(options->GetViewOffset() + selectionIndex);
		}
		
		int modIndex = options->GetViewIndex();
		if ((modIndex < modOptions.Num()) && (modOptions[modIndex].dir != NULL))
		{
			//load mod
			//From original doom3 source
			idStr args = "+set fs_game ";
			args.Append(modOptions[modIndex].dir);
			unsigned int exitCode;
			Sys_Exec(Sys_EXEPath(), "", args.c_str(), NULL, NULL, 0, exitCode);
			common->Quit();
		}
		
		return true;
	}
	}
	
	return idMenuWidget::HandleAction(action, event, widget, forceHandled);
}

void idMenuScreen_Shell_Mods::SetupModOptions()
{	
	modOptions.Append(modOption_t(BASE_GAMEDIR, GAME_NAME));

	idModList *mods = fileSystem->ListMods();
		
	for (int i = 0; i < mods->GetNumMods(); i++)
	{
		modOptions.Append(modOption_t(mods->GetMod(i), mods->GetDescription(i)));
	}
	
	idList< idList< idStr, TAG_IDLIB_LIST_MENU >, TAG_IDLIB_LIST_MENU > menuOptions;

	for (int i = 0; i < modOptions.Num(); ++i)
	{
		idList< idStr > option;
		option.Append(modOptions[i].description);
		menuOptions.Append(option);	
	}
	
	options->SetListData(menuOptions);

	fileSystem->FreeModList(mods);
}