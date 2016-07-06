
################################################################################

file(GLOB AAS_INCLUDES aas/*.h)
file(GLOB AAS_SOURCES aas/*.cpp)

file(GLOB_RECURSE CM_INCLUDES cm/*.h)
file(GLOB_RECURSE CM_SOURCES cm/*.cpp)

file(GLOB FRAMEWORK_INCLUDES framework/*.h)
file(GLOB FRAMEWORK_SOURCES framework/*.cpp)

file(GLOB FRAMEWORK_ASYNC_INCLUDES framework/async/*.h)
file(GLOB FRAMEWORK_ASYNC_SOURCES framework/async/*.cpp)

file(GLOB RENDERER_INCLUDES renderer/*.h)
file(GLOB RENDERER_SOURCES renderer/*.cpp)

file(GLOB RENDERER_COLOR_INCLUDES renderer/Color/*.h)
file(GLOB RENDERER_COLOR_SOURCES renderer/Color/*.cpp)

file(GLOB RENDERER_DXT_INCLUDES renderer/DXT/*.h)
file(GLOB RENDERER_DXT_SOURCES renderer/DXT/*.cpp)

file(GLOB RENDERER_JOBS_INCLUDES renderer/jobs/*.h)
file(GLOB RENDERER_JOBS_SOURCES renderer/jobs/*.cpp)

file(GLOB RENDERER_JOBS_DYNAMICSHADOWVOLUME_INCLUDES renderer/jobs/dynamicshadowvolume/*.h)
file(GLOB RENDERER_JOBS_DYNAMICSHADOWVOLUME_SOURCES renderer/jobs/dynamicshadowvolume/*.cpp)

file(GLOB RENDERER_JOBS_PRELIGHTSHADOWVOLUME_INCLUDES renderer/jobs/prelightshadowvolume/*.h)
file(GLOB RENDERER_JOBS_PRELIGHTSHADOWVOLUME_SOURCES renderer/jobs/prelightshadowvolume/*.cpp)

file(GLOB RENDERER_JOBS_STATICSHADOWVOLUME_INCLUDES renderer/jobs/staticshadowvolume/*.h)
file(GLOB RENDERER_JOBS_STATICSHADOWVOLUME_SOURCES renderer/jobs/staticshadowvolume/*.cpp)

file(GLOB RENDERER_OPENGL_INCLUDES renderer/OpenGL/*.h)
file(GLOB RENDERER_OPENGL_SOURCES renderer/OpenGL/*.cpp)

file(GLOB RENDERER_SMAA_INCLUDES renderer/SMAA/*.h)

set(SOUND_INCLUDES
  sound/snd_defines.h
  sound/snd_local.h
  sound/sound.h
  sound/SoundVoice.h
  sound/WaveFile.h)

set(SOUND_SOURCES
  #sound/snd_cache.cpp
  #sound/snd_decoder.cpp
  #sound/snd_efxfile.cpp
  sound/snd_emitter.cpp
  sound/snd_shader.cpp
  sound/snd_system.cpp
  sound/snd_world.cpp
  sound/SoundVoice.cpp
  sound/WaveFile.cpp
  )

set(XAUDIO2_INCLUDES
  sound/XAudio2/XA2_SoundHardware.h
  sound/XAudio2/XA2_SoundSample.h
  sound/XAudio2/XA2_SoundVoice.h)

set(XAUDIO2_SOURCES
  sound/XAudio2/XA2_SoundHardware.cpp
  sound/XAudio2/XA2_SoundSample.cpp
  sound/XAudio2/XA2_SoundVoice.cpp)

set(OPENAL_INCLUDES
  sound/OpenAL/AL_SoundHardware.h
  sound/OpenAL/AL_SoundSample.h
  sound/OpenAL/AL_SoundVoice.h)

set(OPENAL_SOURCES
  sound/OpenAL/AL_SoundHardware.cpp
  sound/OpenAL/AL_SoundSample.cpp
  sound/OpenAL/AL_SoundVoice.cpp)

set(STUBAUDIO_INCLUDES
  sound/stub/SoundStub.h)

set(STUBAUDIO_SOURCES
  sound/stub/SoundSample.cpp)

file(GLOB SYS_INCLUDES sys/*.h)
file(GLOB SYS_SOURCES sys/*.cpp)

file(GLOB UI_INCLUDES ui/*.h)
file(GLOB UI_SOURCES ui/*.cpp)

file(GLOB SWF_INCLUDES swf/*.h)
file(GLOB SWF_SOURCES swf/*.cpp)


set(GAMED3XP_INCLUDES
  d3xp/Achievements.h
  d3xp/Actor.h
  d3xp/AF.h
  d3xp/AFEntity.h
  d3xp/AimAssist.h
  d3xp/BrittleFracture.h
  d3xp/Camera.h
  #d3xp/EndLevel.h
  d3xp/Entity.h
  d3xp/Fx.h
  d3xp/Game.h
  d3xp/GameEdit.h
  d3xp/Game_local.h
  d3xp/Grabber.h
  d3xp/IK.h
  d3xp/Item.h
  d3xp/Leaderboards.h
  d3xp/Light.h
  d3xp/Misc.h
  d3xp/Moveable.h
  d3xp/Mover.h
  d3xp/MultiplayerGame.h
  d3xp/Player.h
  d3xp/PlayerIcon.h
  d3xp/PlayerView.h
  d3xp/PredictedValue.h
  d3xp/Projectile.h
  d3xp/Pvs.h
  d3xp/SecurityCamera.h
  d3xp/SmokeParticles.h
  d3xp/Sound.h
  d3xp/Target.h
  d3xp/Trigger.h
  d3xp/Weapon.h
  d3xp/WorldSpawn.h)

set(GAMED3XP_SOURCES
  d3xp/Achievements.cpp
  d3xp/Actor.cpp
  d3xp/AF.cpp
  d3xp/AFEntity.cpp
  d3xp/AimAssist.cpp
  d3xp/BrittleFracture.cpp
  d3xp/Camera.cpp
  #d3xp/EndLevel.cpp
  d3xp/Entity.cpp
  d3xp/Fx.cpp
  d3xp/GameEdit.cpp
  d3xp/Game_local.cpp
  d3xp/Game_network.cpp
  d3xp/Grabber.cpp
  d3xp/IK.cpp
  d3xp/Item.cpp
  d3xp/Leaderboards.cpp
  d3xp/Light.cpp
  d3xp/Misc.cpp
  d3xp/Moveable.cpp
  d3xp/Mover.cpp
  d3xp/MultiplayerGame.cpp
  d3xp/Player.cpp
  d3xp/PlayerIcon.cpp
  d3xp/PlayerView.cpp
  d3xp/precompiled.cpp
  d3xp/Projectile.cpp
  d3xp/Pvs.cpp
  d3xp/SecurityCamera.cpp
  d3xp/SmokeParticles.cpp
  d3xp/Sound.cpp
  d3xp/Target.cpp
  d3xp/Trigger.cpp
  d3xp/Weapon.cpp
  d3xp/WorldSpawn.cpp)

file(GLOB GAMED3XP_AI_INCLUDES d3xp/ai/*.h)
file(GLOB GAMED3XP_AI_SOURCES d3xp/ai/*.cpp)

file(GLOB GAMED3XP_ANIM_INCLUDES d3xp/anim/*.h)
file(GLOB GAMED3XP_ANIM_SOURCES d3xp/anim/*.cpp)

file(GLOB GAMED3XP_GAMESYS_INCLUDES d3xp/gamesys/*.h)
set(GAMED3XP_GAMESYS_SOURCES
  #d3xp/gamesys/Callbacks.cpp
  d3xp/gamesys/Class.cpp
  #d3xp/gamesys/DebugGraph.cpp
  d3xp/gamesys/Event.cpp
  d3xp/gamesys/SaveGame.cpp
  d3xp/gamesys/SysCmds.cpp
  d3xp/gamesys/SysCvar.cpp
  #d3xp/gamesys/TypeInfo.cpp
  )

  
# shared menu includes only for cegui builds
set(GAMED3XP_MENUS_SHARED_INCLUDES
	d3xp/menus/MenuHandler_Interface.h
	d3xp/menus/MenuScreen_Interface.h
	d3xp/menus/MenuHandler.h
	d3xp/menus/MenuScreen.h
	d3xp/menus/MenuState.h
  )

if (CEGUI)
  list(APPEND GAMED3XP_MENUS_INCLUDES
    ${GAMED3XP_MENUS_SHARED_INCLUDES}
  )
else()
  file(GLOB GAMED3XP_MENUS_SWF_INCLUDES d3xp/menus/*.h)
  file(GLOB GAMED3XP_MENUS_SWF_SOURCES d3xp/menus/*.cpp)
  
  list(APPEND GAMED3XP_MENUS_INCLUDES
    ${GAMED3XP_MENUS_SHARED_INCLUDES}
	${GAMED3XP_MENUS_SWF_INCLUDES}
  )
  
  list(APPEND GAMED3XP_MENUS_SOURCES
	${GAMED3XP_MENUS_SWF_SOURCES}
  )
endif()

file(GLOB GAMED3XP_PHYSICS_INCLUDES d3xp/physics/*.h)
file(GLOB GAMED3XP_PHYSICS_SOURCES d3xp/physics/*.cpp)

file(GLOB GAMED3XP_SCRIPT_INCLUDES d3xp/script/*.h)
file(GLOB GAMED3XP_SCRIPT_SOURCES d3xp/script/*.cpp)

file(GLOB_RECURSE WIN32_INCLUDES sys/win32/*.h)
file(GLOB_RECURSE WIN32_SOURCES sys/win32/*.cpp)

#set(WIN32_SOURCES
#	sys/win32/win_glimp.cpp
#	sys/win32/win_input.cpp
#	sys/win32/win_main.cpp
#	sys/win32/win_shared.cpp
#	sys/win32/win_snd.cpp
#	sys/win32/win_syscon.cpp
#	sys/win32/win_taskkeyhook.cpp
#	sys/win32/win_wndproc.cpp
#	sys/win32/win_cpu.cpp # MSVC?
#	)

set(WIN32_RESOURCES
  # sys/win32/rc/res/BEVEL.BMP
  # sys/win32/rc/res/BITMAP2.BMP
  # sys/win32/rc/res/BMP00001.BMP
  # sys/win32/rc/res/BMP0002.BMP
  # sys/win32/rc/res/DEFTEX.WAL
  # sys/win32/rc/res/ENDCAP.BMP
  # sys/win32/rc/res/GetString.htm
  # sys/win32/rc/res/IBEVEL.BMP
  # sys/win32/rc/res/IENDCAP.BMP
  # sys/win32/rc/res/MEFileToolbar.bmp
  # sys/win32/rc/res/MEtoolbar.bmp
  # sys/win32/rc/res/MaterialEditor.ico
  # sys/win32/rc/res/PropTree.rc2
  # sys/win32/rc/res/Q.BMP
  # sys/win32/rc/res/RADIANT3.GIF
  # sys/win32/rc/res/Radiant.ico
  # sys/win32/rc/res/RadiantDoc.ico
  # sys/win32/rc/res/TOOLBAR1.BMP
  # sys/win32/rc/res/TOOLBAR2.BMP
  # sys/win32/rc/res/Toolbar.bmp
  # sys/win32/rc/res/VIEWDEFA.BMP
  # sys/win32/rc/res/VIEWOPPO.BMP
  # sys/win32/rc/res/bmp00002.bmp
  # sys/win32/rc/res/bmp00003.bmp
  # sys/win32/rc/res/bmp00004.bmp
  # sys/win32/rc/res/bmp00005.bmp
  # sys/win32/rc/res/cchsb.bmp
  # sys/win32/rc/res/ccrgb.bmp
  # sys/win32/rc/res/dbg_back.bmp
  # sys/win32/rc/res/dbg_breakpoint.ico
  # sys/win32/rc/res/dbg_current.ico
  # sys/win32/rc/res/dbg_currentline.ico
  # sys/win32/rc/res/dbg_empty.ico
  # sys/win32/rc/res/dbg_open.bmp
  # sys/win32/rc/res/dbg_toolbar.bmp
  sys/win32/rc/res/doom.ico
  # sys/win32/rc/res/fpoint.cur
  # sys/win32/rc/res/fxed_link.ico
  # sys/win32/rc/res/fxed_toolbar.bmp
  # sys/win32/rc/res/fxeditor.ico
  # sys/win32/rc/res/guied.ico
  # sys/win32/rc/res/guied_collapse.ico
  # sys/win32/rc/res/guied_expand.ico
  # sys/win32/rc/res/guied_hand.cur
  # sys/win32/rc/res/guied_nav_visible.ico
  # sys/win32/rc/res/guied_nav_visibledisabled.ico
  # sys/win32/rc/res/guied_scripts.ico
  # sys/win32/rc/res/guied_scripts_white.ico
  # sys/win32/rc/res/guied_viewer_toolbar.bmp
  # sys/win32/rc/res/icon2.ico
  # sys/win32/rc/res/logo_sm3dfx.bmp
  # sys/win32/rc/res/matedtree.bmp
  # sys/win32/rc/res/me_disabled_icon.ico
  # sys/win32/rc/res/me_enabled.ico
  # sys/win32/rc/res/me_off_icon.ico
  # sys/win32/rc/res/me_on_icon.ico
  # sys/win32/rc/res/qe3.ico
  # sys/win32/rc/res/shaderbar.bmp
  # sys/win32/rc/res/shaderdoc.ico
  # sys/win32/rc/res/shaderframe.ico
  # sys/win32/rc/res/spliter.cur
  )

if(WIN32)
  if(USE_MFC_TOOLS)
    list(APPEND WIN32_RESOURCES sys/win32/rc/doom.rc)
    #else()
    #	list(APPEND WIN32_RESOURCES sys/win32/rc/doom_nomfc.rc)
  endif()
endif()


file(GLOB POSIX_INCLUDES sys/posix/*.h)
file(GLOB POSIX_SOURCES sys/posix/*.cpp)

file(GLOB COMMON_INCLUDES sys/common/*.h)
file(GLOB COMMON_SOURCES sys/common/*.cpp)

file(GLOB SDL_INCLUDES sys/sdl/*.h)
file(GLOB SDL_SOURCES sys/sdl/*.cpp)

file(GLOB BFG_IMGUI_INCLUDES imgui/*.h)
file(GLOB BFG_IMGUI_SOURCES imgui/*.cpp)

source_group("aas" FILES ${AAS_INCLUDES})
source_group("aas" FILES ${AAS_SOURCES})

source_group("cm" FILES ${CM_INCLUDES})
source_group("cm" FILES ${CM_SOURCES})

source_group("framework" FILES ${FRAMEWORK_INCLUDES})
source_group("framework" FILES ${FRAMEWORK_SOURCES})

source_group("framework\\async" FILES ${FRAMEWORK_ASYNC_INCLUDES})
source_group("framework\\async" FILES ${FRAMEWORK_ASYNC_SOURCES})

source_group("renderer" FILES ${RENDERER_INCLUDES})
source_group("renderer" FILES ${RENDERER_SOURCES})

source_group("renderer\\Color" FILES ${RENDERER_COLOR_INCLUDES})
source_group("renderer\\Color" FILES ${RENDERER_COLOR_SOURCES})

source_group("renderer\\DXT" FILES ${RENDERER_DXT_INCLUDES})
source_group("renderer\\DXT" FILES ${RENDERER_DXT_SOURCES})

source_group("renderer\\jobs" FILES ${RENDERER_JOBS_INCLUDES})
source_group("renderer\\jobs" FILES ${RENDERER_JOBS_SOURCES})

source_group("renderer\\jobs\\dynamicshadowvolume" FILES ${RENDERER_JOBS_DYNAMICSHADOWVOLUME_INCLUDES})
source_group("renderer\\jobs\\dynamicshadowvolume" FILES ${RENDERER_JOBS_DYNAMICSHADOWVOLUME_SOURCES})

source_group("renderer\\jobs\\prelightshadowvolume" FILES ${RENDERER_JOBS_PRELIGHTSHADOWVOLUME_INCLUDES})
source_group("renderer\\jobs\\prelightshadowvolume" FILES ${RENDERER_JOBS_PRELIGHTSHADOWVOLUME_SOURCES})

source_group("renderer\\jobs\\staticshadowvolume" FILES ${RENDERER_JOBS_STATICSHADOWVOLUME_INCLUDES})
source_group("renderer\\jobs\\staticshadowvolume" FILES ${RENDERER_JOBS_STATICSHADOWVOLUME_SOURCES})

source_group("renderer\\OpenGL" FILES ${RENDERER_OPENGL_INCLUDES})
source_group("renderer\\OpenGL" FILES ${RENDERER_OPENGL_SOURCES})

source_group("renderer\\SMAA" FILES ${RENDERER_SMAA_INCLUDES})

source_group("sound" FILES ${SOUND_INCLUDES})
source_group("sound" FILES ${SOUND_SOURCES})

source_group("sound\\XAudio2" FILES ${XAUDIO2_INCLUDES})
source_group("sound\\XAudio2" FILES ${XAUDIO2_SOURCES})

source_group("sound\\OpenAL" FILES ${OPENAL_INCLUDES})
source_group("sound\\OpenAL" FILES ${OPENAL_SOURCES})

source_group("sound\\stub" FILES ${STUBAUDIO_INCLUDES})
source_group("sound\\stub" FILES ${STUBAUDIO_SOURCES})

source_group("ui" FILES ${UI_INCLUDES})
source_group("ui" FILES ${UI_SOURCES})

source_group("swf" FILES ${SWF_INCLUDES})
source_group("swf" FILES ${SWF_SOURCES})

source_group("sys" FILES ${SYS_INCLUDES})
source_group("sys" FILES ${SYS_SOURCES})

source_group("game-d3xp" FILES ${GAMED3XP_INCLUDES})
source_group("game-d3xp" FILES ${GAMED3XP_SOURCES})

source_group("game-d3xp\\ai" FILES ${GAMED3XP_AI_INCLUDES})
source_group("game-d3xp\\ai" FILES ${GAMED3XP_AI_SOURCES})

source_group("game-d3xp\\anim" FILES ${GAMED3XP_ANIM_INCLUDES})
source_group("game-d3xp\\anim" FILES ${GAMED3XP_ANIM_SOURCES})

source_group("game-d3xp\\gamesys" FILES ${GAMED3XP_GAMESYS_INCLUDES})
source_group("game-d3xp\\gamesys" FILES ${GAMED3XP_GAMESYS_SOURCES})

source_group("game-d3xp\\menus" FILES ${GAMED3XP_MENUS_INCLUDES})
source_group("game-d3xp\\menus" FILES ${GAMED3XP_MENUS_SOURCES})

source_group("game-d3xp\\physics" FILES ${GAMED3XP_PHYSICS_INCLUDES})
source_group("game-d3xp\\physics" FILES ${GAMED3XP_PHYSICS_SOURCES})

source_group("game-d3xp\\script" FILES ${GAMED3XP_SCRIPT_INCLUDES})
source_group("game-d3xp\\script" FILES ${GAMED3XP_SCRIPT_SOURCES})

source_group("sys\\win32" FILES ${WIN32_INCLUDES})
source_group("sys\\win32" FILES ${WIN32_SOURCES})
source_group("sys\\win32\\Resources" FILES ${WIN32_RESOURCES})

source_group("sys\\posix" FILES ${POSIX_INCLUDES})
source_group("sys\\posix" FILES ${POSIX_SOURCES})

source_group("sys\\common" FILES ${COMMON_INCLUDES})
source_group("sys\\common" FILES ${COMMON_SOURCES})

source_group("sys\\sdl" FILES ${SDL_INCLUDES})
source_group("sys\\sdl" FILES ${SDL_SOURCES})

source_group("imgui" FILES ${BFG_IMGUI_INCLUDES})
source_group("imgui" FILES ${BFG_IMGUI_SOURCES})

set(OpenTechBFG_INCLUDES
  ${AAS_INCLUDES}
  ${CM_INCLUDES}
  ${FRAMEWORK_INCLUDES}
  ${FRAMEWORK_ASYNC_INCLUDES}
  ${RENDERER_INCLUDES}
  ${RENDERER_COLOR_INCLUDES}
  ${RENDERER_DXT_INCLUDES}
  ${RENDERER_JOBS_INCLUDES}
  ${RENDERER_JOBS_DYNAMICSHADOWVOLUME_INCLUDES}
  ${RENDERER_JOBS_PRELIGHTSHADOWVOLUME_INCLUDES}
  ${RENDERER_JOBS_STATICSHADOWVOLUME_INCLUDES}
  ${RENDERER_OPENGL_INCLUDES}
  ${RENDERER_SMAA_INCLUDES}
  ${SOUND_INCLUDES}
  ${UI_INCLUDES}
  ${SWF_INCLUDES}
  ${COMMON_INCLUDES}
  ${BFG_IMGUI_INCLUDES}
  )

set(OpenTechBFG_SOURCES
  ${AAS_SOURCES}
  ${CM_SOURCES}
  ${FRAMEWORK_SOURCES}
  ${FRAMEWORK_ASYNC_SOURCES} 
  ${RENDERER_SOURCES}
  ${RENDERER_COLOR_SOURCES}
  ${RENDERER_DXT_SOURCES}
  ${RENDERER_JOBS_SOURCES}
  ${RENDERER_JOBS_DYNAMICSHADOWVOLUME_SOURCES}
  ${RENDERER_JOBS_PRELIGHTSHADOWVOLUME_SOURCES}
  ${RENDERER_JOBS_STATICSHADOWVOLUME_SOURCES}
  ${RENDERER_OPENGL_SOURCES}
  ${SOUND_SOURCES}
  ${UI_SOURCES}
  ${SWF_SOURCES}
  ${COMMON_SOURCES}
  ${BFG_IMGUI_SOURCES}
  )

add_definitions(-D__DOOM__
  #-DBUILD_FREETYPE
  #-DFT2_BUILD_LIBRARY
  )

if(MONOLITH)
  list(APPEND OpenTechBFG_INCLUDES
    ${GAMED3XP_INCLUDES}
    ${GAMED3XP_AI_INCLUDES} 
    ${GAMED3XP_ANIM_INCLUDES} 
    ${GAMED3XP_GAMESYS_INCLUDES} 
    ${GAMED3XP_MENUS_INCLUDES}
    ${GAMED3XP_PHYSICS_INCLUDES}
    ${GAMED3XP_SCRIPT_INCLUDES})
  
  list(APPEND OpenTechBFG_SOURCES
    ${GAMED3XP_SOURCES}
    ${GAMED3XP_AI_SOURCES}
    ${GAMED3XP_ANIM_SOURCES}
    ${GAMED3XP_GAMESYS_SOURCES}
    ${GAMED3XP_MENUS_SOURCES}
    ${GAMED3XP_PHYSICS_SOURCES}
    ${GAMED3XP_SCRIPT_SOURCES})
else()
  add_definitions(-D__DOOM_DLL__)
endif()

# KORTEMIK: FIXME remove idlib from this list
include_directories(
  .
  idlib)

if(IDTOOLS)
  add_definitions(-DUSE_IDTOOLS)
  
  list(APPEND OpenTechBFG_INCLUDES
    ${IDTOOLS_INCLUDES}
    )
  
  list(APPEND OpenTechBFG_SOURCES
    ${IDTOOLS_SOURCES} 
    )
endif()

if(CEGUI)
  file(GLOB CEGUI_INCLUDES cegui/*.h)
  file(GLOB CEGUI_SOURCES cegui/*.cpp)

  file(GLOB CEGUI_CONSOLE_INCLUDES cegui/console/*.h)
  file(GLOB CEGUI_CONSOLE_SOURCES cegui/console/*.cpp)

  file(GLOB CEGUI_MENU_INCLUDES cegui/menu/*.h)
  file(GLOB CEGUI_MENU_SOURCES cegui/menu/*.cpp)

  file(GLOB CEGUI_MENU_MAIN_INCLUDES cegui/menu/MainMenu/*.h)
  file(GLOB CEGUI_MENU_MAIN_SOURCES cegui/menu/MainMenu/*.cpp)

  list(APPEND CEGUI_INCLUDES
    ${CEGUI_CONSOLE_INCLUDES}
    )
  
  list(APPEND CEGUI_SOURCES
    ${CEGUI_CONSOLE_SOURCES}
    )

  list(APPEND CEGUI_INCLUDES
    ${CEGUI_MENU_INCLUDES}
    ${CEGUI_MENU_MAIN_INCLUDES}
    )
  
  list(APPEND CEGUI_SOURCES
    ${CEGUI_MENU_SOURCES}
    ${CEGUI_MENU_MAIN_SOURCES}
    )
	
  source_group("cegui" FILES ${CEGUI_INCLUDES})
  source_group("cegui" FILES ${CEGUI_SOURCES})

  list(APPEND OpenTechBFG_INCLUDES
    ${CEGUI_INCLUDES}
    )
  list(APPEND OpenTechBFG_SOURCES
    ${CEGUI_SOURCES}
    )
endif()
