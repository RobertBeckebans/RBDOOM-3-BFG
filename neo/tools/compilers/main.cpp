/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2024-2025 Robert Beckebans

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

#include <shlwapi.h>  // for PathMatchSpecW

#include "../sys/sys_local.h"
#include "../framework/EventLoop.h"
#include "../framework/DeclManager.h"

#include <direct.h>
#include <io.h>

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
	char msg[MAXPRINTMSG];			\
	va_list argptr;					\
	va_start( argptr, fmt );		\
	idStr::vsnPrintf( msg, MAXPRINTMSG - 1, fmt, argptr ); \
	msg[ sizeof( msg ) - 1 ] = '\0'; \
	va_end( argptr );				\
	Sys_Printf( "%s%s%s", pre, msg, post );			\


idCVar com_developer( "developer", "0", CVAR_BOOL | CVAR_SYSTEM, "developer mode" );
idCVar com_productionMode( "com_productionMode", "0", CVAR_SYSTEM | CVAR_BOOL, "0 - no special behavior, 1 - building a production build, 2 - running a production build" );

/*
==============================================================

	idSys

==============================================================
*/



/*
==============
Sys_Printf
==============
*/

void Sys_Printf( const char* fmt, ... )
{
	char msg[MAXPRINTMSG];

	va_list argptr;
	va_start( argptr, fmt );
	idStr::vsnPrintf( msg, MAXPRINTMSG - 1, fmt, argptr );
	msg[ sizeof( msg ) - 1 ] = '\0';
	va_end( argptr );

	printf( msg );
	OutputDebugString( msg );

	tuiLog.AddLog( "%s", msg );
}

/*
==============
Sys_VPrintf
==============
*/
void Sys_VPrintf( const char* fmt, va_list arg )
{
	char msg[MAXPRINTMSG];

	idStr::vsnPrintf( msg, MAXPRINTMSG - 1, fmt, arg );
	msg[ sizeof( msg ) - 1 ] = '\0';

	printf( msg );
	OutputDebugString( msg );

	tuiLog.AddLog( "%s", msg );
}

// RB: Returns absolute path with \\?\ prefix for long path support
wchar_t* MakeWindowsLongPathW( const char* utf8Path )
{
	if( !utf8Path || !utf8Path[0] )
	{
		return NULL;
	}

	int utf16Len = MultiByteToWideChar( CP_UTF8, 0, utf8Path, -1, NULL, 0 );
	if( utf16Len == 0 )
	{
		return NULL;
	}

	// Allocate temporary buffer for initial conversion
	wchar_t* tempPath = ( wchar_t* )malloc( utf16Len * sizeof( wchar_t ) );
	if( !tempPath )
	{
		return NULL;
	}

	// Perform UTF-8 to UTF-16 conversion
	if( MultiByteToWideChar( CP_UTF8, 0, utf8Path, -1, tempPath, utf16Len ) == 0 )
	{
		free( tempPath );
		return NULL;
	}

	// Replace forward slashes with backslashes
	for( int i = 0; tempPath[i]; i++ )
	{
		if( tempPath[i] == L'/' )
		{
			tempPath[i] = L'\\';
		}
	}

	// Convert to absolute path
	wchar_t fullPath[MAX_OSPATH];
	if( GetFullPathNameW( tempPath, MAX_OSPATH, fullPath, NULL ) == 0 )
	{
		free( tempPath );
		return NULL;
	}
	free( tempPath );

	// +4 for \\?\ prefix +1 for null terminator
	size_t fullPathLen = wcslen( fullPath );
	wchar_t* wPath = ( wchar_t* )malloc( ( fullPathLen + 5 ) * sizeof( wchar_t ) );
	if( !wPath )
	{
		return NULL;
	}

	// Add \\?\ prefix for long path support
	swprintf( wPath, fullPathLen + 5, L"\\\\?\\%s", fullPath );
	//wcscpy( wPath, L"\\\\?\\" );
	//wcscat( wPath, fullPath );

	return wPath;
}
// RB end

/*
==============
Sys_Mkdir
==============
*/
void Sys_Mkdir( const char* path )
{
	// RB: support paths longer than 260 characters

	// ignore pure drive-letter paths like "C:"
	if( strlen( path ) == 2 && path[1] == ':' && isalpha( path[0] ) )
	{
		return;
	}

	wchar_t* wPath = MakeWindowsLongPathW( path );
	if( !wPath )
	{
		common->FatalError( "Failed to convert path to wide string: '%s'", path );
		return;
	}

	if( !CreateDirectoryW( wPath, NULL ) )
	{
		DWORD err = GetLastError();

		// don't fatal if directory already exists
		if( err != ERROR_ALREADY_EXISTS )
		{
			common->FatalError( "CreateDirectoryW failed (error %lu)", err );
		}
	}

	free( wPath );
	// RB end
}


/*
========================
Sys_Rmdir
========================
*/
bool Sys_Rmdir( const char* path )
{
	// RB: support paths longer than 260 characters
	wchar_t* wPath = MakeWindowsLongPathW( path );
	if( !wPath )
	{
		common->FatalError( "Failed to convert path to wide string: '%s'", path );
		return false;
	}

	BOOL success = RemoveDirectoryW( wPath );
	if( success == 0 )
	{
		DWORD err = GetLastError();
		common->FatalError( "RemoveDirectoryW failed (error %lu)", err );
	}

	free( wPath );
	return success != 0;
}

/*
==============
Sys_EXEPath
==============
*/
const char* Sys_EXEPath()
{
	static char exe[MAX_OSPATH];
	GetModuleFileName( NULL, exe, sizeof( exe ) - 1 );
	return exe;
}

/*
==============
Sys_ListFiles
==============
*/
static void ListFilesRecursive( const wchar_t* baseDir, const wchar_t* pattern, idStrList& list, size_t baseLen )
{
	wchar_t searchPath[MAX_OSPATH];
	swprintf( searchPath, MAX_OSPATH, L"%s\\*", baseDir );

	WIN32_FIND_DATAW findData;
	HANDLE hFind = FindFirstFileW( searchPath, &findData );
	if( hFind == INVALID_HANDLE_VALUE )
	{
		return;
	}

	do
	{
		if( wcscmp( findData.cFileName, L"." ) == 0 || wcscmp( findData.cFileName, L".." ) == 0 )
		{
			continue;
		}

		wchar_t fullPath[MAX_OSPATH];
		swprintf( fullPath, MAX_OSPATH, L"%s\\%s", baseDir, findData.cFileName );

		bool isDir = ( findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) != 0;

		if( isDir )
		{
			ListFilesRecursive( fullPath, pattern, list, baseLen );
		}
		else if( PathMatchSpecW( findData.cFileName, pattern ) )
		{
			char utf8Name[MAX_OSPATH];
			int len = WideCharToMultiByte( CP_UTF8, 0, fullPath + baseLen, -1, utf8Name, sizeof( utf8Name ), NULL, NULL );
			if( len > 0 )
			{
				list.Append( utf8Name );
			}
		}
	}
	while( FindNextFileW( hFind, &findData ) );

	FindClose( hFind );
}


int Sys_ListFiles( const char* directory, const char* extension, idStrList& list )
{
#if 0
	idStr		search;
	struct _finddata_t findinfo;
	// RB: 64 bit fixes, changed int to intptr_t
	intptr_t	findhandle;
	// RB end
	int			flag;

	if( !extension )
	{
		extension = "";
	}

	// passing a slash as extension will find directories
	if( extension[0] == '/' && extension[1] == 0 )
	{
		extension = "";
		flag = 0;
	}
	else
	{
		flag = _A_SUBDIR;
	}

	sprintf( search, "%s\\*%s", directory, extension );

	// search
	list.Clear();

	findhandle = _findfirst( search, &findinfo );
	if( findhandle == -1 )
	{
		return -1;
	}

	do
	{
		if( flag ^ ( findinfo.attrib & _A_SUBDIR ) )
		{
			list.Append( findinfo.name );
		}
	}
	while( _findnext( findhandle, &findinfo ) != -1 );

	_findclose( findhandle );

	return list.Num();
#else
	// RB: support paths longer than 260 characters
	wchar_t* wDir = MakeWindowsLongPathW( directory );
	if( !wDir )
	{
		common->FatalError( "Failed to convert path to wide string: '%s'", directory );
		return -1;
	}

	idStr extPattern = "*";
	if( extension && extension[0] )
	{
		extPattern += extension;
	}

	// convert extension pattern to wide string
	int extLen = MultiByteToWideChar( CP_UTF8, 0, extPattern.c_str(), -1, NULL, 0 );
	wchar_t* wPattern = ( wchar_t* )malloc( extLen * sizeof( wchar_t ) );
	if( !wPattern )
	{
		free( wDir );
		return -1;
	}
	MultiByteToWideChar( CP_UTF8, 0, extPattern.c_str(), -1, wPattern, extLen );

	idStrList tempList;
	size_t baseLen = wcslen( wDir );
	ListFilesRecursive( wDir, wPattern, tempList, baseLen + 1 ); // +1 to skip path separator

	// convert to forward slashes to tab completion works
	for( auto& s : tempList )
	{
		s.BackSlashesToSlashes();
		list.Append( s.c_str() );
	}

	free( wDir );
	free( wPattern );
	return list.Num();
	// RB end
#endif
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
	wchar_t* wPath = MakeWindowsLongPathW( path );
	if( !wPath )
	{
		return FOLDER_ERROR;
	}

	struct _stat buffer;
	int result = _wstat( wPath, &buffer );
	free( wPath );

	if( result < 0 )
	{
		return FOLDER_ERROR;
	}
	return ( buffer.st_mode & _S_IFDIR ) != 0 ? FOLDER_YES : FOLDER_NO;
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
	FILETIME writeTime;
	GetFileTime( fp, NULL, NULL, &writeTime );

	/*
		FILETIME = number of 100-nanosecond ticks since midnight
		1 Jan 1601 UTC. time_t = number of 1-second ticks since
		midnight 1 Jan 1970 UTC. To translate, we subtract a
		FILETIME representation of midnight, 1 Jan 1970 from the
		time in question and divide by the number of 100-ns ticks
		in one second.
	*/

	SYSTEMTIME base_st =
	{
		1970,   // wYear
		1,      // wMonth
		0,      // wDayOfWeek
		1,      // wDay
		0,      // wHour
		0,      // wMinute
		0,      // wSecond
		0       // wMilliseconds
	};

	FILETIME base_ft;
	SystemTimeToFileTime( &base_st, &base_ft );

	LARGE_INTEGER itime;
	itime.QuadPart = reinterpret_cast<LARGE_INTEGER&>( writeTime ).QuadPart;
	itime.QuadPart -= reinterpret_cast<LARGE_INTEGER&>( base_ft ).QuadPart;
	itime.QuadPart /= 10000000LL;
	return itime.QuadPart;
}

/*
================
Sys_GetClockTicks
================
*/
double Sys_GetClockTicks()
{
	LARGE_INTEGER li;

	QueryPerformanceCounter( &li );
	return ( double ) li.LowPart + ( double ) 0xFFFFFFFF * li.HighPart;
}

/*
================
Sys_ClockTicksPerSecond
================
*/
double Sys_ClockTicksPerSecond()
{
	static double ticks = 0;

	if( !ticks )
	{
		LARGE_INTEGER li;
		QueryPerformanceFrequency( &li );
		ticks = li.QuadPart;
	}

	return ticks;
}

/*
==============
Sys_Cwd
==============
*/
const char* Sys_Cwd()
{
	static char cwd[MAX_OSPATH];

	_getcwd( cwd, sizeof( cwd ) - 1 );
	cwd[MAX_OSPATH - 1] = 0;

	return cwd;
}

/*
==============
Sys_DefaultBasePath
==============
*/
const char* Sys_DefaultBasePath()
{
	return Sys_Cwd();
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
int Sys_Milliseconds()
{
	static DWORD sys_timeBase = timeGetTime();
	return timeGetTime() - sys_timeBase;
}


/*
================
Sys_Microseconds
================
*/
uint64 Sys_Microseconds()
{
	static uint64 ticksPerMicrosecondTimes1024 = 0;

	if( ticksPerMicrosecondTimes1024 == 0 )
	{
		ticksPerMicrosecondTimes1024 = ( ( uint64 )Sys_ClockTicksPerSecond() << 10 ) / 1000000;
		assert( ticksPerMicrosecondTimes1024 > 0 );
	}

	return ( ( uint64 )( ( int64 )Sys_GetClockTicks() << 10 ) ) / ticksPerMicrosecondTimes1024;
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

#if 0

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

#if 1
	// maybe only do this if dmap has a leaked BSP
	while( true )
	{
		common->UpdateScreen( false );
	}
#endif

	ImTui_ImplText_Shutdown();
	ImTui_ImplNcurses_Shutdown();

	return 0;
}
#else

#include "imtui/imtui.h"
#include "imtui/imtui-impl-ncurses.h"
#include "imtui/imtui-demo.h"

void idCommonLocal::UpdateScreen( bool captureToImage, bool releaseMouse )
{
}

int main()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	auto screen = ImTui_ImplNcurses_Init( true );
	ImTui_ImplText_Init();

	stateUI.ChangeColorScheme( false );

	bool demo = true;
	int nframes = 0;
	float fval = 1.23f;

	while( true )
	{
		ImTui_ImplNcurses_NewFrame();
		ImTui_ImplText_NewFrame();

		ImGui::NewFrame();

		ImGui::SetNextWindowPos( ImVec2( 4, 27 ), ImGuiCond_Once );
		ImGui::SetNextWindowSize( ImVec2( 50.0, 10.0 ), ImGuiCond_Once );
		ImGui::Begin( "Hello, world!" );
		ImGui::Text( "NFrames = %d", nframes++ );
		ImGui::Text( "Mouse Pos : x = %g, y = %g", ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y );
		ImGui::Text( "Time per frame %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate );
		ImGui::Text( "Float:" );
		ImGui::SameLine();
		ImGui::SliderFloat( "##float", &fval, 0.0f, 10.0f );

#ifndef __EMSCRIPTEN__
		ImGui::Text( "%s", "" );
		if( ImGui::Button( "Exit program", { ImGui::GetContentRegionAvail().x, 2 } ) )
		{
			break;
		}
#endif

		ImGui::End();

		ImTui::ShowDemoWindow( &demo );

		ImGui::Render();

		ImTui_ImplText_RenderDrawData( ImGui::GetDrawData(), screen );
		ImTui_ImplNcurses_DrawScreen();
	}

	ImTui_ImplText_Shutdown();
	ImTui_ImplNcurses_Shutdown();

	return 0;
}

#endif
