/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2024 Robert Beckebans

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

#include "../sys/sys_local.h"
#include "../framework/EventLoop.h"
#include "../framework/DeclManager.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <dirent.h>
#include <fnmatch.h>
#include <unistd.h>

#include "imtui/imtui.h"
#include "imtui/imtui-impl-ncurses.h"
#include "imtui/imtui-demo.h"

idEventLoop* eventLoop;



//-----------------------------------------------------------------------------
// [SECTION] Example App: Debug Log / ShowExampleAppLog()
//-----------------------------------------------------------------------------

struct MyAppLog
{
	ImGuiTextBuffer     Buf;
	ImGuiTextFilter     Filter;
	ImVector<int>       LineOffsets;        // Index to lines offset. We maintain this with AddLog() calls, allowing us to have a random access on lines

	MyAppLog()
	{
		Clear();
	}

	void    Clear()
	{
		Buf.clear();
		LineOffsets.clear();
		LineOffsets.push_back( 0 );
	}

	void    AddLog( const char* fmt, ... ) IM_FMTARGS( 2 )
	{
		int old_size = Buf.size();
		va_list args;
		va_start( args, fmt );
		Buf.appendfv( fmt, args );
		va_end( args );
		for( int new_size = Buf.size(); old_size < new_size; old_size++ )
			if( Buf[old_size] == '\n' )
			{
				LineOffsets.push_back( old_size + 1 );
			}
	}

	void    Draw( const char* title, bool* p_open = NULL )
	{

		{
			auto wSize = ImGui::GetIO().DisplaySize;
			ImGui::SetNextWindowPos( ImVec2( 0, 1 ), ImGuiCond_Always );
			ImGui::SetNextWindowSize( ImVec2( wSize.x, wSize.y - 5 ), ImGuiCond_Always );
		}
		if( !ImGui::Begin( title, p_open, ImGuiWindowFlags_NoDecoration ) )
		{
			ImGui::End();
			return;
		}


		bool copy = ImGui::Button( "Copy to Clipboard" );
		ImGui::SameLine();

		ImGui::Separator();
		ImGui::BeginChild( "scrolling", ImVec2( 0, 0 ), false, ImGuiWindowFlags_HorizontalScrollbar );

		if( copy )
		{
			ImGui::LogToClipboard();
		}

		ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, 0 ) );
		const char* buf = Buf.begin();
		const char* buf_end = Buf.end();
#if 0
		if( Filter.IsActive() )
		{
			// In this example we don't use the clipper when Filter is enabled.
			// This is because we don't have a random access on the result on our filter.
			// A real application processing logs with ten of thousands of entries may want to store the result of search/filter.
			// especially if the filtering function is not trivial (e.g. reg-exp).
			for( int line_no = 0; line_no < LineOffsets.Size; line_no++ )
			{
				const char* line_start = buf + LineOffsets[line_no];
				const char* line_end = ( line_no + 1 < LineOffsets.Size ) ? ( buf + LineOffsets[line_no + 1] - 1 ) : buf_end;
				if( Filter.PassFilter( line_start, line_end ) )
				{
					ImGui::TextUnformatted( line_start, line_end );
				}
			}
		}
		else
#endif
		{
			// The simplest and easy way to display the entire buffer:
			//   ImGui::TextUnformatted(buf_begin, buf_end);
			// And it'll just work. TextUnformatted() has specialization for large blob of text and will fast-forward to skip non-visible lines.
			// Here we instead demonstrate using the clipper to only process lines that are within the visible area.
			// If you have tens of thousands of items and their processing cost is non-negligible, coarse clipping them on your side is recommended.
			// Using ImGuiListClipper requires A) random access into your data, and B) items all being the  same height,
			// both of which we can handle since we an array pointing to the beginning of each line of text.
			// When using the filter (in the block of code above) we don't have random access into the data to display anymore, which is why we don't use the clipper.
			// Storing or skimming through the search result would make it possible (and would be recommended if you want to search through tens of thousands of entries)
			ImGuiListClipper clipper;
			clipper.Begin( LineOffsets.Size );
			while( clipper.Step() )
			{
				for( int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++ )
				{
					const char* line_start = buf + LineOffsets[line_no];
					const char* line_end = ( line_no + 1 < LineOffsets.Size ) ? ( buf + LineOffsets[line_no + 1] - 1 ) : buf_end;
					ImGui::TextUnformatted( line_start, line_end );
				}
			}
			clipper.End();
		}
		ImGui::PopStyleVar();

		//if( ImGui::GetScrollY() >= ImGui::GetScrollMaxY() )
		{
			ImGui::SetScrollHereY( 1.0f );
		}

		ImGui::EndChild();
		ImGui::End();
	}
};

static MyAppLog tuiLog;

#define MAXPRINTMSG 4096

#define STDIO_PRINT( pre, post )	\
	va_list argptr;					\
	va_start( argptr, fmt );		\
	printf( pre );					\
	vprintf( fmt, argptr );			\
	printf( post );					\
	printf(post);		\
	va_end( argptr )			\


idCVar com_developer( "developer", "0", CVAR_BOOL | CVAR_SYSTEM, "developer mode" );
idCVar com_productionMode( "com_productionMode", "0", CVAR_SYSTEM | CVAR_BOOL, "0 - no special behavior, 1 - building a production build, 2 - running a production build" );

/*
==============================================================

	idSys

==============================================================
*/

void Sys_Printf( const char* fmt, ... )
{
	va_list argptr;

	//tty_Hide();
	va_start( argptr, fmt );
	vprintf( fmt, argptr );
	va_end( argptr );
	//tty_Show();
}

void Sys_VPrintf( const char* fmt, va_list arg )
{
	//tty_Hide();
	vprintf( fmt, arg );
	//tty_Show();
}

/*
==============
Sys_Mkdir
==============
*/
void Sys_Mkdir( const char* path )
{
	mkdir( path, 0777 );
}


/*
========================
Sys_Rmdir
========================
*/
bool Sys_Rmdir( const char* path )
{
	return ( rmdir( path ) == 0 );
}

/*
==============
Sys_EXEPath
==============
*/
const char* Sys_EXEPath()
{
	static char	buf[ 1024 ];
	idStr		linkpath;
	int			len;

	buf[ 0 ] = '\0';
	sprintf( linkpath, "/proc/%d/exe", getpid() );
	len = readlink( linkpath.c_str(), buf, sizeof( buf ) );
	if( len == -1 )
	{
		Sys_Printf( "couldn't stat exe path link %s\n", linkpath.c_str() );
		// RB: fixed array subscript is below array bounds
		buf[ 0 ] = '\0';
		// RB end
	}
	return buf;
}

/*
================
Posix_Cwd
================
*/
const char* Posix_Cwd()
{
	static char cwd[MAX_OSPATH];

	getcwd( cwd, sizeof( cwd ) - 1 );
	cwd[MAX_OSPATH - 1] = 0;

	return cwd;
}

/*
==============
Sys_ListFiles
==============
*/
int Sys_ListFiles( const char* directory, const char* extension, idStrList& list )
{
	struct dirent* d;
	DIR* fdir;
	bool dironly = false;
	char search[MAX_OSPATH];
	struct stat st;
	bool debug;

	list.Clear();

	debug = cvarSystem->GetCVarBool( "fs_debug" );
	// DG: we use fnmatch for shell-style pattern matching
	// so the pattern should at least contain "*" to match everything,
	// the extension will be added behind that (if !dironly)
	idStr pattern( "*" );

	// passing a slash as extension will find directories
	if( extension[0] == '/' && extension[1] == 0 )
	{
		dironly = true;
	}
	else
	{
		// so we have *<extension>, the same as in the windows code basically
		pattern += extension;
	}
	// DG end

	// NOTE: case sensitivity of directory path can screw us up here
	if( ( fdir = opendir( directory ) ) == NULL )
	{
		if( debug )
		{
			common->Printf( "Sys_ListFiles: opendir %s failed\n", directory );
		}
		return -1;
	}

	// DG: use readdir_r instead of readdir for thread safety
	// the following lines are from the readdir_r manpage.. fscking ugly.
	//int nameMax = pathconf( directory, _PC_NAME_MAX );
	//if( nameMax == -1 )
	//{
	//	nameMax = 255;
	//}
	//int direntLen = offsetof( struct dirent, d_name ) + nameMax + 1;

	//struct dirent* entry = ( struct dirent* )Mem_Alloc( direntLen, TAG_CRAP );

	//if( entry == NULL )
	//{
	//	common->Warning( "Sys_ListFiles: Mem_Alloc for entry failed!" );
	//	closedir( fdir );
	//	return 0;
	//}

	//while( readdir_r( fdir, entry, &d ) == 0 && d != NULL )
	// SRS - readdir_r() is deprecated on linux, readdir() is thread safe with different dir streams
	while( ( d = readdir( fdir ) ) != NULL )
	{
		// DG end
		idStr::snPrintf( search, sizeof( search ), "%s/%s", directory, d->d_name );
		if( stat( search, &st ) == -1 )
		{
			continue;
		}
		if( !dironly )
		{
			// DG: the original code didn't work because d3 bfg abuses the extension
			// to match whole filenames and patterns in the savegame-code, not just file extensions...
			// so just use fnmatch() which supports matching shell wildcard patterns ("*.foo" etc)
			// if we should ever need case insensitivity, use FNM_CASEFOLD as third flag
			if( fnmatch( pattern.c_str(), d->d_name, 0 ) != 0 )
			{
				continue;
			}
			// DG end
		}
		if( ( dironly && !( st.st_mode & S_IFDIR ) ) ||
				( !dironly && ( st.st_mode & S_IFDIR ) ) )
		{
			continue;
		}

		list.Append( d->d_name );
	}

	closedir( fdir );
	//Mem_Free( entry );

	if( debug )
	{
		common->Printf( "Sys_ListFiles: %d entries in %s\n", list.Num(), directory );
	}

	return list.Num();
}



int idEventLoop::JournalLevel() const
{
	return 0;
}

/*
========================
Sys_IsFolder
========================
*/
sysFolder_t Sys_IsFolder( const char* path )
{
	struct stat buffer;

	if( stat( path, &buffer ) < 0 )
	{
		return FOLDER_ERROR;
	}

	return ( buffer.st_mode & S_IFDIR ) != 0 ? FOLDER_YES : FOLDER_NO;
}

const char* Sys_DefaultSavePath()
{
	return "";
}

const char* Sys_Lang( int )
{
	return "";
}


/*
=================
Sys_FileTimeStamp
=================
*/
ID_TIME_T Sys_FileTimeStamp( idFileHandle fp )
{
	struct stat st;
	fstat( fileno( fp ), &st );
	return st.st_mtime;
}

/*
===============
Sys_GetClockticks
===============
*/
double Sys_GetClockTicks()
{
#if defined( __x86_64__ )
	uint32_t lo, hi;
	__asm__ __volatile__( "rdtsc" : "=a"( lo ), "=d"( hi ) );
	return ( ( ( uint64_t )hi ) << 32 ) | lo;
#else
	//#error unsupported CPU
	struct timespec now;

	clock_gettime( CLOCK_MONOTONIC, &now );

	return now.tv_sec * 1000000000LL + now.tv_nsec;
#endif
}

void Sys_Sleep( int msec )
{
	if( usleep( msec * 1000 ) == -1 )
	{
		Sys_Printf( "usleep: %s\n", strerror( errno ) );
	}
}

double MeasureClockTicks()
{
	double t0, t1;

	t0 = Sys_GetClockTicks();
	Sys_Sleep( 1000 );
	t1 = Sys_GetClockTicks();

	return t1 - t0;
}

/*
================
Sys_ClockTicksPerSecond
================
*/
double Sys_ClockTicksPerSecond()
{
	static bool		init = false;
	static double	ret;

	if( init )
	{
		return ret;
	}

	ret = MeasureClockTicks();
	init = true;
	common->Printf( "measured CPU frequency: %g MHz\n", ret / 1000000.0 );
	return ret;
}

/*
================
Sys_DefaultBasePath

Get the default base path
- binary image path
- current directory
- macOS app bundle resources directory path			// SRS - added macOS app bundle resources path
- build directory path								// SRS - added build directory path
- hardcoded
Try to be intelligent: if there is no BASE_GAMEDIR, try the next path
================
*/
static idStr basepath;

const char* Sys_DefaultBasePath()
{
	struct stat st;
	idStr testbase, exepath = {};
	basepath = Sys_EXEPath();
	if( basepath.Length() )
	{
		exepath = basepath.StripFilename();
		testbase = basepath;
		testbase += "/";
		testbase += BASE_GAMEDIR;
		if( stat( testbase.c_str(), &st ) != -1 && S_ISDIR( st.st_mode ) )
		{
			return basepath.c_str();
		}
		else
		{
			common->Printf( "no '%s' directory in exe path %s, skipping\n", BASE_GAMEDIR, basepath.c_str() );
		}
	}
	if( basepath != Posix_Cwd() )
	{
		basepath = Posix_Cwd();
		testbase = basepath;
		testbase += "/";
		testbase += BASE_GAMEDIR;
		if( stat( testbase.c_str(), &st ) != -1 && S_ISDIR( st.st_mode ) )
		{
			return basepath.c_str();
		}
		else
		{
			common->Printf( "no '%s' directory in cwd path %s, skipping\n", BASE_GAMEDIR, basepath.c_str() );
		}
	}
	if( exepath.Length() )
	{
#if defined(__APPLE__)
		// SRS - Check for macOS app bundle resources path (up one dir level and down to Resources dir)
		basepath = exepath;
		basepath = basepath.StripFilename() + "/Resources";
		testbase = basepath;
		testbase += "/";
		testbase += BASE_GAMEDIR;
		if( stat( testbase.c_str(), &st ) != -1 && S_ISDIR( st.st_mode ) )
		{
			return basepath.c_str();
		}
		else
		{
			common->Printf( "no '%s' directory in macOS app bundle resources path %s, skipping\n", BASE_GAMEDIR, basepath.c_str() );
		}
#endif
		// SRS - Check for linux/macOS build path (directory structure with build dir and possible config suffix)
		basepath = exepath;
		basepath.StripFilename();						// up 1st dir level for single-config dev builds
#if !defined( NO_MULTI_CONFIG )
		basepath.StripFilename();						// up 2nd dir level for multi-config dev builds with Debug/Release/etc suffix
#endif
		testbase = basepath;
		testbase += "/";
		testbase += BASE_GAMEDIR;
		if( stat( testbase.c_str(), &st ) != -1 && S_ISDIR( st.st_mode ) )
		{
			return basepath.c_str();
		}
		else
		{
			common->Printf( "no '%s' directory in build path %s, skipping\n", BASE_GAMEDIR, basepath.c_str() );
		}
	}
	common->Printf( "WARNING: using hardcoded default base path %s\n", DEFAULT_BASEPATH );
	return DEFAULT_BASEPATH;
}

int Sys_NumLangs()
{
	return 0;
}

/*
================
Sys_Milliseconds
================
*/
/* base time in seconds, that's our origin
   timeval:tv_sec is an int:
   assuming this wraps every 0x7fffffff - ~68 years since the Epoch (1970) - we're safe till 2038
   using unsigned long data type to work right with Sys_XTimeToSysTime */

#ifdef CLOCK_MONOTONIC_RAW
	// use RAW monotonic clock if available (=> not subject to NTP etc)
	#define D3_CLOCK_TO_USE CLOCK_MONOTONIC_RAW
#else
	#define D3_CLOCK_TO_USE CLOCK_MONOTONIC
#endif

// RB: changed long to int
unsigned int sys_timeBase = 0;
// RB end
/* current time in ms, using sys_timeBase as origin
   NOTE: sys_timeBase*1000 + curtime -> ms since the Epoch
     0x7fffffff ms - ~24 days
		 or is it 48 days? the specs say int, but maybe it's casted from unsigned int?
*/
int Sys_Milliseconds()
{
	// DG: use clock_gettime on all platforms
	int curtime;
	struct timespec ts;

	clock_gettime( D3_CLOCK_TO_USE, &ts );

	if( !sys_timeBase )
	{
		sys_timeBase = ts.tv_sec;
		return ts.tv_nsec / 1000000;
	}

	curtime = ( ts.tv_sec - sys_timeBase ) * 1000 + ts.tv_nsec / 1000000;

	return curtime;
	// DG end
}

/*
================
Sys_Microseconds
================
*/
static uint64 sys_microTimeBase = 0;

uint64 Sys_Microseconds()
{
#if 0
	static uint64 ticksPerMicrosecondTimes1024 = 0;

	if( ticksPerMicrosecondTimes1024 == 0 )
	{
		ticksPerMicrosecondTimes1024 = ( ( uint64 )Sys_ClockTicksPerSecond() << 10 ) / 1000000;
		assert( ticksPerMicrosecondTimes1024 > 0 );
	}

	return ( ( uint64 )( ( int64 )Sys_GetClockTicks() << 10 ) ) / ticksPerMicrosecondTimes1024;
#elif 0
	uint64 curtime;
	struct timespec ts;

	clock_gettime( CLOCK_MONOTONIC, &ts );

	curtime = ts.tv_sec * 1000000 + ts.tv_nsec / 1000;

	return curtime;
#else
	uint64 curtime;
	struct timespec ts;

	clock_gettime( D3_CLOCK_TO_USE, &ts );

	if( !sys_microTimeBase )
	{
		sys_microTimeBase = ts.tv_sec;
		return ts.tv_nsec / 1000;
	}

	curtime = ( ts.tv_sec - sys_microTimeBase ) * 1000000 + ts.tv_nsec / 1000;

	return curtime;
#endif
}

/*
========================
Sys_CPUCount

TODO: This is a dummy function;
If required I recommend using SDL_CpuCount();

numLogicalCPUCores      - the number of logical CPU per core
numPhysicalCPUCores     - the total number of cores per package
numCPUPackages          - the total number of packages (physical processors)
========================
*/
void Sys_CPUCount( int& numLogicalCPUCores, int& numPhysicalCPUCores, int& numCPUPackages )
{
	numPhysicalCPUCores = 1;
	numLogicalCPUCores = 1;
	numCPUPackages = 1;
}

class idSysCmdline : public idSys
{
public:
	virtual void			DebugPrintf( VERIFY_FORMAT_STRING const char* fmt, ... )
	{
		va_list argptr;

		va_start( argptr, fmt );
		Sys_VPrintf( fmt, argptr );
		va_end( argptr );
	}

	virtual void			DebugVPrintf( const char* fmt, va_list arg )
	{
		Sys_VPrintf( fmt, arg );
	}

	virtual double			GetClockTicks()
	{
		return Sys_GetClockTicks();
	}

	virtual double			ClockTicksPerSecond()
	{
		return Sys_ClockTicksPerSecond();
	}

	virtual cpuid_t			GetProcessorId()
	{
		return CPUID_NONE;
	}

	virtual const char* 	GetProcessorString()
	{
		return NULL;
	}
	virtual const char* 	FPU_GetState()
	{
		return NULL;
	}
	virtual bool			FPU_StackIsEmpty()
	{
		return false;
	}
	virtual void			FPU_SetFTZ( bool enable ) {}
	virtual void			FPU_SetDAZ( bool enable ) {}

	virtual void			FPU_EnableExceptions( int exceptions ) {}

	virtual bool			LockMemory( void* ptr, int bytes )
	{
		return false;
	}
	virtual bool			UnlockMemory( void* ptr, int bytes )
	{
		return false;
	}

	virtual int				DLL_Load( const char* dllName )
	{
		return 0;
	}
	virtual void* 			DLL_GetProcAddress( int dllHandle, const char* procName )
	{
		return NULL;
	}
	virtual void			DLL_Unload( int dllHandle ) {}
	virtual void			DLL_GetFileName( const char* baseName, char* dllName, int maxLength ) {}

	virtual sysEvent_t		GenerateMouseButtonEvent( int button, bool down )
	{
		sysEvent_t ev;
		ev.evType = SE_NONE;
		return ev;
	}
	virtual sysEvent_t		GenerateMouseMoveEvent( int deltax, int deltay )
	{
		sysEvent_t ev;
		ev.evType = SE_NONE;
		return ev;
	}

	virtual void			OpenURL( const char* url, bool quit ) {}
	virtual void			StartProcess( const char* exeName, bool quit ) {}
};

idSysCmdline		idSysLocal;
idSys* 				sys = &idSysLocal;

/*
==============================================================

	idCommon

==============================================================
*/

namespace UI
{

enum class ColorScheme : int
{
	Default,
	Dark,
	Green,
	COUNT,
};

struct State
{
	int hoveredWindowId = 0;
	int statusWindowHeight = 4;

	ColorScheme colorScheme = ColorScheme::Dark;

	bool showHelpWelcome = false;
	bool showHelpModal = false;
	bool showStatusWindow = true;

#define STATUS_TEXT_SIZE 512
	idStrStatic<STATUS_TEXT_SIZE> statusWindowHeader =  "Initializing Doom Framework";
	idStrStatic<STATUS_TEXT_SIZE> statusActiveTool = "-";

	float progress = 1.0f;

	void ChangeColorScheme( bool inc = true )
	{
		if( inc )
		{
			colorScheme = ( ColorScheme )( ( ( int ) colorScheme + 1 ) % ( ( int )ColorScheme::COUNT ) );
		}

		ImVec4* colors = ImGui::GetStyle().Colors;
		switch( colorScheme )
		{
			case ColorScheme::Default:
			{
				colors[ImGuiCol_Text]                   = ImVec4( 0.00f, 0.00f, 0.00f, 1.00f );
				colors[ImGuiCol_TextDisabled]           = ImVec4( 0.60f, 0.60f, 0.60f, 1.00f );
				colors[ImGuiCol_WindowBg]               = ImVec4( 0.96f, 0.96f, 0.94f, 1.00f );
				colors[ImGuiCol_TitleBg]                = ImVec4( 1.00f, 0.40f, 0.00f, 1.00f );
				colors[ImGuiCol_TitleBgActive]          = ImVec4( 1.00f, 0.40f, 0.00f, 1.00f );
				colors[ImGuiCol_TitleBgCollapsed]       = ImVec4( 0.69f, 0.25f, 0.00f, 1.00f );
				colors[ImGuiCol_ChildBg]                = ImVec4( 0.96f, 0.96f, 0.94f, 1.00f );
				colors[ImGuiCol_PopupBg]                = ImVec4( 0.96f, 0.96f, 0.94f, 1.00f );
				colors[ImGuiCol_ModalWindowDimBg]       = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
				break;
			}

			case ColorScheme::Dark:
			{
				colors[ImGuiCol_Text]                   = ImVec4( 1.00f, 1.00f, 1.00f, 1.00f );
				colors[ImGuiCol_TextDisabled]           = ImVec4( 0.60f, 0.60f, 0.60f, 1.00f );
				colors[ImGuiCol_WindowBg]               = ImVec4( 0.10f, 0.10f, 0.10f, 1.00f );
				colors[ImGuiCol_TitleBg]                = ImVec4( 1.00f, 0.40f, 0.00f, 0.50f );
				colors[ImGuiCol_TitleBgActive]          = ImVec4( 1.00f, 0.40f, 0.00f, 0.50f );
				colors[ImGuiCol_TitleBgCollapsed]       = ImVec4( 0.69f, 0.25f, 0.00f, 0.50f );
				colors[ImGuiCol_ChildBg]                = ImVec4( 0.10f, 0.10f, 0.10f, 1.00f );
				colors[ImGuiCol_PopupBg]                = ImVec4( 0.20f, 0.20f, 0.20f, 1.00f );
				colors[ImGuiCol_ModalWindowDimBg]       = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
				break;
			}

			case ColorScheme::Green:
			{
				colors[ImGuiCol_Text]                   = ImVec4( 0.00f, 1.00f, 0.00f, 1.00f );
				colors[ImGuiCol_TextDisabled]           = ImVec4( 0.60f, 0.60f, 0.60f, 1.00f );
				colors[ImGuiCol_WindowBg]               = ImVec4( 0.10f, 0.10f, 0.10f, 1.00f );
				colors[ImGuiCol_TitleBg]                = ImVec4( 0.25f, 0.25f, 0.25f, 1.00f );
				colors[ImGuiCol_TitleBgActive]          = ImVec4( 0.25f, 0.25f, 0.25f, 1.00f );
				colors[ImGuiCol_TitleBgCollapsed]       = ImVec4( 0.50f, 1.00f, 0.50f, 1.00f );
				colors[ImGuiCol_ChildBg]                = ImVec4( 0.10f, 0.10f, 0.10f, 1.00f );
				colors[ImGuiCol_PopupBg]                = ImVec4( 0.00f, 0.00f, 0.00f, 1.00f );
				colors[ImGuiCol_ModalWindowDimBg]       = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
				break;
			}

			default:
			{
			}
		}
	}
};

}

// UI state
UI::State stateUI;

class idCommonLocal : public idCommon
{
private:
	int							count = 0;
	int							expectedCount = 0;
	size_t						tics = 0;
	size_t						nextTicCount = 0;

public:
	bool						com_refreshOnPrint = true;		// update the screen every print for dmap
	ImTui::TScreen*				screen;


	// Initialize everything.
	// if the OS allows, pass argc/argv directly (without executable name)
	// otherwise pass the command line in a single string (without executable name)
	virtual void				Init( int argc, const char* const* argv, const char* cmdline ) {}

	// Shuts down everything.
	virtual void				Shutdown() {}
	virtual bool				IsShuttingDown() const
	{
		return false;
	};

	virtual	void				CreateMainMenu() {}

	// Shuts down everything.
	virtual void				Quit() {}

	// Returns true if common initialization is complete.
	virtual bool				IsInitialized() const
	{
		return true;
	};

	// Called repeatedly as the foreground thread for rendering and game logic.
	virtual void				Frame() {}

	// Redraws the screen, handling games, guis, console, etc
	// in a modal manner outside the normal frame loop
	virtual void				UpdateScreen( bool captureToImage, bool releaseMouse = true );

	virtual void				UpdateLevelLoadPacifier() {}
	virtual void				LoadPacifierInfo( VERIFY_FORMAT_STRING const char* fmt, ... ) {}
	virtual void				LoadPacifierProgressTotal( int total ) {}
	virtual void				LoadPacifierProgressIncrement( int step ) {}
	virtual bool				LoadPacifierRunning()
	{
		return false;
	}


	// Checks for and removes command line "+set var arg" constructs.
	// If match is NULL, all set commands will be executed, otherwise
	// only a set with the exact name.
	virtual void				StartupVariable( const char* match ) {}

	// Begins redirection of console output to the given buffer.
	virtual void				BeginRedirect( char* buffer, int buffersize, void ( *flush )( const char* ) ) {}

	// Stops redirection of console output.
	virtual void				EndRedirect() {}

	// Update the screen with every message printed.
	virtual void				SetRefreshOnPrint( bool set )
	{
		//com_refreshOnPrint = set;
	}

	virtual void			Printf( const char* fmt, ... )
	{
		STDIO_PRINT( "", "" );

		if( com_refreshOnPrint )
		{
			common->UpdateScreen( false );
		}
	}

	virtual void			VPrintf( const char* fmt, va_list arg )
	{
		Sys_VPrintf( fmt, arg );

		if( com_refreshOnPrint )
		{
			common->UpdateScreen( false );
		}
	}

	virtual void			DPrintf( const char* fmt, ... )
	{
		if( com_developer.GetBool() )
		{
			STDIO_PRINT( "", "" );

			if( com_refreshOnPrint )
			{
				common->UpdateScreen( false );
			}
		}
	}

	virtual void			VerbosePrintf( const char* fmt, ... )
	{
		if( dmap_verbose.GetBool() )
		{
			STDIO_PRINT( "", "" );

			if( com_refreshOnPrint )
			{
				common->UpdateScreen( false );
			}
		}
	}

	virtual void			Warning( const char* fmt, ... )
	{
		STDIO_PRINT( "WARNING: ", "\n" );

		if( com_refreshOnPrint )
		{
			common->UpdateScreen( false );
		}
	}

	virtual void			DWarning( const char* fmt, ... )
	{
		if( com_developer.GetBool() )
		{
			STDIO_PRINT( "WARNING: ", "\n" );

			if( com_refreshOnPrint )
			{
				common->UpdateScreen( false );
			}
		}
	}

	// Prints all queued warnings.
	virtual void				PrintWarnings() {}

	// Removes all queued warnings.
	virtual void				ClearWarnings( const char* reason ) {}

	virtual void			Error( const char* fmt, ... )
	{
		STDIO_PRINT( "ERROR: ", "\n" );

		if( com_refreshOnPrint )
		{
			common->UpdateScreen( false );
		}
		exit( 0 );
	}
	virtual void			FatalError( const char* fmt, ... )
	{
		STDIO_PRINT( "FATAL ERROR: ", "\n" );

		if( com_refreshOnPrint )
		{
			common->UpdateScreen( false );
		}
		exit( 0 );
	}

	// Returns key bound to the command
	virtual const char* KeysFromBinding( const char* bind )
	{
		return NULL;
	};

	// Returns the binding bound to the key
	virtual const char* BindingFromKey( const char* key )
	{
		return NULL;
	};

	// Directly sample a button.
	virtual int					ButtonState( int key )
	{
		return 0;
	};

	// Directly sample a keystate.
	virtual int					KeyState( int key )
	{
		return 0;
	};

	// Returns true if a multiplayer game is running.
	// CVars and commands are checked differently in multiplayer mode.
	virtual bool				IsMultiplayer()
	{
		return false;
	};
	virtual bool				IsServer()
	{
		return false;
	};
	virtual bool				IsClient()
	{
		return false;
	};

	// Returns true if the player has ever enabled the console
	virtual bool				GetConsoleUsed()
	{
		return false;
	};

	// Returns the rate (in ms between snaps) that we want to generate snapshots
	virtual int					GetSnapRate()
	{
		return 0;
	};

	virtual void				NetReceiveReliable( int peer, int type, idBitMsg& msg ) { };
	virtual void				NetReceiveSnapshot( class idSnapShot& ss ) { };
	virtual void				NetReceiveUsercmds( int peer, idBitMsg& msg ) { };

	// Processes the given event.
	virtual	bool				ProcessEvent( const sysEvent_t* event )
	{
		return false;
	};

	virtual bool				LoadGame( const char* saveName )
	{
		return false;
	};
	virtual bool				SaveGame( const char* saveName )
	{
		return false;
	};

	virtual idGame* Game()
	{
		return NULL;
	};
	virtual idRenderWorld* RW()
	{
		return NULL;
	};
	virtual idSoundWorld* SW()
	{
		return NULL;
	};
	virtual idSoundWorld* MenuSW()
	{
		return NULL;
	};
	virtual idSession* Session()
	{
		return NULL;
	};
	virtual idCommonDialog& Dialog()
	{
		static idCommonDialog useless;
		return useless;
	};

	virtual void				OnSaveCompleted( idSaveLoadParms& parms ) {}
	virtual void				OnLoadCompleted( idSaveLoadParms& parms ) {}
	virtual void				OnLoadFilesCompleted( idSaveLoadParms& parms ) {}
	virtual void				OnEnumerationCompleted( idSaveLoadParms& parms ) {}
	virtual void				OnDeleteCompleted( idSaveLoadParms& parms ) {}
	virtual void				TriggerScreenWipe( const char* _wipeMaterial, bool hold ) {}

	virtual void				OnStartHosting( idMatchParameters& parms ) {}

	virtual int					GetGameFrame()
	{
		return 0;
	};

	virtual void				LaunchExternalTitle( int titleIndex, int device, const lobbyConnectInfo_t* const connectInfo ) { };

	virtual void				InitializeMPMapsModes() { };
	virtual const idStrList& GetModeList() const
	{
		static idStrList useless;
		return useless;
	};
	virtual const idStrList& GetModeDisplayList() const
	{
		static idStrList useless;
		return useless;
	};
	virtual const idList<mpMap_t>& GetMapList() const
	{
		static idList<mpMap_t> useless;
		return useless;
	};

	virtual void				ResetPlayerInput( int playerIndex ) {}

	virtual bool				JapaneseCensorship() const
	{
		return false;
	};

	virtual void				QueueShowShell() { };		// Will activate the shell on the next frame.
	virtual void				InitTool( const toolFlag_t, const idDict*, idEntity* ) {}

	virtual void				LoadPacifierBinarizeFilename( const char* filename, const char* reason ) {}
	virtual void				LoadPacifierBinarizeInfo( const char* info ) {}
	virtual void				LoadPacifierBinarizeMiplevel( int level, int maxLevel ) {}
	virtual void				LoadPacifierBinarizeProgress( float progress ) {}
	virtual void				LoadPacifierBinarizeEnd() { };
	virtual void				LoadPacifierBinarizeProgressTotal( int total ) {}
	virtual void				LoadPacifierBinarizeProgressIncrement( int step ) {}

	virtual void				DmapPacifierFilename( const char* filename, const char* reason )
	{
		stateUI.statusWindowHeader.Format( "%s | %s", filename, reason );
	}

	virtual void				DmapPacifierInfo( VERIFY_FORMAT_STRING const char* fmt, ... )
	{
		char msg[STATUS_TEXT_SIZE];

		va_list argptr;
		va_start( argptr, fmt );
		idStr::vsnPrintf( msg, STATUS_TEXT_SIZE - 1, fmt, argptr );
		msg[ sizeof( msg ) - 1 ] = '\0';
		va_end( argptr );

		stateUI.statusActiveTool = msg;

		if( com_refreshOnPrint )
		{
			UpdateScreen( false );
		}
	}

	virtual void				DmapPacifierCompileProgressTotal( int total )
	{
		count = 0;
		expectedCount = total;
		tics = 0;
		nextTicCount = 0;

		stateUI.progress = 0;
	}

	virtual void				DmapPacifierCompileProgressIncrement( int step )
	{
		count += step;

		stateUI.progress = float( count ) / expectedCount;

		// don't refresh the UI with every step if there are e.g. 1300 steps
		if( ( count + 1 ) >= nextTicCount )
		{
			size_t ticsNeeded = ( size_t )( ( ( double )( count + 1 ) / expectedCount ) * 50.0 );

			do
			{
				//common->Printf( "*" );
			}
			while( ++tics < ticsNeeded );

			nextTicCount = ( size_t )( ( tics / 50.0 ) * expectedCount );
			/*
			if( count == ( expectedCount - 1 ) )
			{
				//if( tics < 51 )
				//{
				//	common->Printf( "*" );
				//}
				//common->Printf( "\n" );

				//stateUI.progress = 1;
			}
			*/

			if( com_refreshOnPrint )
			{
				common->UpdateScreen( false );
			}
		}
	}
};

idCommonLocal		commonLocal;
idCommon* common = &commonLocal;

int com_editors = 0;

/*
==============================================================

	main

==============================================================
*/

int Dmap_NoGui( int argc, char** argv )
{
	commonLocal.com_refreshOnPrint = false;

	idLib::common = common;
	idLib::cvarSystem = cvarSystem;
	idLib::fileSystem = fileSystem;
	idLib::sys = sys;

	idLib::Init();
	cmdSystem->Init();
	cvarSystem->Init();
	idCVar::RegisterStaticVars();

	// set cvars before filesystem init to use mod paths
	idCmdArgs args;
	for( int i = 0; i < argc; i++ )
	{
		if( idStr::Icmp( argv[ i ], "+set" ) == 0 )
		{
			if( ( i + 2 ) < argc )
			{
				cvarSystem->SetCVarString( argv[ i + 1 ], argv[ i + 2 ] );
			}

			i += 2;
		}
		else if( idStr::Icmp( argv[ i ], "-t" ) == 0 || idStr::Icmp( argv[ i ], "-nogui" ) == 0 )
		{
		}
		else
		{
			args.AppendArg( argv[i] );
		}
	}

	fileSystem->Init();
	declManager->InitTool();

	Dmap_f( args );

	return 0;
}

#if 1

void idCommonLocal::UpdateScreen( bool captureToImage, bool releaseMouse )
{
}

int main( int argc, char** argv )
{
	return Dmap_NoGui( argc, argv );
}

#elif 1


void idCommonLocal::UpdateScreen( bool captureToImage, bool releaseMouse )
{
	bool demo = true;
	bool conOpen = true;

	ImTui_ImplNcurses_NewFrame();
	ImTui_ImplText_NewFrame();

	ImGui::NewFrame();

	{
		auto wSize = ImGui::GetIO().DisplaySize;
		if( stateUI.showStatusWindow )
		{
			wSize.y -= stateUI.statusWindowHeight;
		}
		wSize.x = int( wSize.x );
		ImGui::SetNextWindowPos( ImVec2( 0, 0 ), ImGuiCond_Always );

		ImGui::SetNextWindowSize( wSize, ImGuiCond_Always );
	}

	//idStr title = va( "RBDMAP version %s %s", ENGINE_VERSION, BUILD_STRING );
	idStr title = va( "RBDMAP version %s %s %s %s", ENGINE_VERSION, BUILD_STRING, ID__DATE__, ID__TIME__ );
	ImGui::Begin( title.c_str(), nullptr,
				  ImGuiWindowFlags_NoCollapse |
				  ImGuiWindowFlags_NoResize |
				  ImGuiWindowFlags_NoMove |
				  ImGuiWindowFlags_NoScrollbar );

	tuiLog.Draw( "Current Log:", &conOpen );

	//ShowExampleAppConsole( &conOpen );

	//ImTui::ShowDemoWindow( &demo );

	ImGui::End();

	{
		auto wSize = ImGui::GetIO().DisplaySize;
		ImGui::SetNextWindowPos( ImVec2( 0, wSize.y - stateUI.statusWindowHeight ), ImGuiCond_Always );
		ImGui::SetNextWindowSize( ImVec2( wSize.x, stateUI.statusWindowHeight ), ImGuiCond_Always );
	}

	ImGui::Begin( stateUI.statusWindowHeader, nullptr,
				  ImGuiWindowFlags_NoCollapse |
				  ImGuiWindowFlags_NoResize |
				  ImGuiWindowFlags_NoMove );

	auto wSize = ImGui::GetIO().DisplaySize;
	if( stateUI.progress < 1.0f )
	{
		ImGui::ProgressBar( stateUI.progress, ImVec2( wSize.x, 0.0f ) );
	}
	else
	{
		ImGui::Text( " " );
	}

	ImGui::Text( " %s", stateUI.statusActiveTool.c_str() );
	ImGui::Text( " Source code      : https://github.com/RobertBeckebans/RBDOOM-3-BFG" );
	ImGui::End();

	ImGui::Render();

	ImTui_ImplText_RenderDrawData( ImGui::GetDrawData(), screen );
	ImTui_ImplNcurses_DrawScreen();
}

int main( int argc, char** argv )
{
	for( int i = 0; i < argc; i++ )
	{
		if( idStr::Icmp( argv[ i ], "-t" ) == 0 || idStr::Icmp( argv[ i ], "-nogui" ) == 0 )
		{
			return Dmap_NoGui( argc, argv );
		}
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	commonLocal.screen = ImTui_ImplNcurses_Init( true );
	ImTui_ImplText_Init();

	stateUI.ChangeColorScheme( false );

	idLib::common = common;
	idLib::cvarSystem = cvarSystem;
	idLib::fileSystem = fileSystem;
	idLib::sys = sys;

	idLib::Init();
	cmdSystem->Init();
	cvarSystem->Init();
	idCVar::RegisterStaticVars();

	// set cvars before filesystem init to use mod paths
	idCmdArgs args;
	for( int i = 0; i < argc; i++ )
	{
		if( idStr::Icmp( argv[ i ], "+set" ) == 0 )
		{
			if( ( i + 2 ) < argc )
			{
				cvarSystem->SetCVarString( argv[ i + 1 ], argv[ i + 2 ] );
			}

			i += 2;
		}
		else if( idStr::Icmp( argv[ i ], "-t" ) == 0 || idStr::Icmp( argv[ i ], "-nogui" ) == 0 )
		{
		}
		else
		{
			args.AppendArg( argv[i] );
		}
	}

	fileSystem->Init();
	declManager->InitTool();

	Dmap_f( args );

#if 0
	while( true )
	{
		bool captureToImage = false;
		common->UpdateScreen( captureToImage );
	}
#endif

	ImTui_ImplText_Shutdown();
	ImTui_ImplNcurses_Shutdown();

	return 0;
}

#endif
