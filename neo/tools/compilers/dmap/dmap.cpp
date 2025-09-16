/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2015 Robert Beckebans

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "precompiled.h"
#pragma hdrstop

#include "dmap.h"

idCVar dmap_verbose( "dmap_verbose", "0", CVAR_BOOL | CVAR_SYSTEM | CVAR_NEW, "dmap developer mode" );

dmapGlobals_t	dmapGlobals;

/*
============
ProcessModel
============
*/
bool ProcessModel( uEntity_t* e, bool floodFillWorld )
{
	bspFace_t*	faces;

	faces = MakeStructuralBspFaceList( e->primitives );

	// RB: dump input faces for debugging
	if( dmapGlobals.exportDebugVisuals )
	{
		WriteGLViewFacelist( faces, "facelist" );
	}

	// build a bsp tree using all of the sides
	// of all of the structural brushes
	e->tree = FaceBSP( faces );

	// create portals at every leaf intersection
	// to allow flood filling
	MakeTreePortals( e->tree );

	// RB: calculate node numbers for split plane analysis
	int numLeafs = 0;
	int numNodes = NumberNodes_r( e->tree->headnode, 0, numLeafs );
	int depth = log2f( numLeafs + 1 );

	// classify the leafs as opaque or areaportal
	FilterBrushesIntoTree( e );

	// RB: use mapTri_t by MapPolygonMesh primitives in case we don't use brushes
	if( dmapGlobals.entityNum == 0 )
	{
		FilterMeshesIntoTree( e );
	}

	// see if the bsp is completely enclosed
	bool floodFillEntity = ( e->tree->simpleBSP && dmapGlobals.entityNum != 0 );
	if( floodFillEntity )
	{
		// mark center of entity as occupied so FillOutside works
		idVec3 center = e->tree->bounds.GetCenter();
		if( PlaceOccupant( e->tree->headnode, center, e ) )
		{
			bool inside = true;
		}

		//if( inside ) // should always work
		{
			// set the outside leafs to opaque
			FillOutside( e );
		}
	}
	else if( ( floodFillWorld && !dmapGlobals.noFlood ) )
	{
		if( FloodEntities( e->tree ) )
		{
			// set the outside leafs to opaque
			FillOutside( e );
		}
		else
		{
			common->Printf( "**********************\n" );
			common->Warning( "******* leaked *******" );
			common->Printf( "**********************\n" );
			LeakFile( e->tree );
			WriteGLViewBSP( e->tree, "leaked", dmapGlobals.entityNum, dmapGlobals.verboseentities );

			// bail out here.  If someone really wants to
			// process a map that leaks, they should use
			// -noFlood
			if( floodFillWorld && !dmapGlobals.noFlood )
			{
				return false;
			}
		}
	}

	// get minimum convex hulls for each visible side
	// this must be done before creating area portals,
	// because the visible hull is used as the portal
	ClipSidesByTree( e );

	// determine areas before clipping tris into the
	// tree, so tris will never cross area boundaries
	FloodAreas( e );

	// we now have a BSP tree with solid and non-solid leafs marked with areas
	// all primitives will now be clipped into this, throwing away
	// fragments in the solid areas
	PutPrimitivesInAreas( e );

	// now build shadow volumes for the lights and split
	// the optimize lists by the light beam trees
	// so there won't be unneeded overdraw in the static
	// case
	Prelight( e );

	// optimizing is a superset of fixing tjunctions
	if( !dmapGlobals.noOptimize )
	{
		OptimizeEntity( e );
	}
	else if( !dmapGlobals.noTJunc )
	{
		FixEntityTjunctions( e );
	}

	// now fix t junctions across areas
	FixGlobalTjunctions( e );

	return true;
}

/*
============
ProcessModels
============
*/
bool ProcessModels()
{
	bool oldVerbose = dmap_verbose.GetBool();

	common->DmapPacifierCompileProgressTotal( dmapGlobals.numEntities );

	idStrStatic<128> entityInfo;

	for( dmapGlobals.entityNum = 0; dmapGlobals.entityNum < dmapGlobals.numEntities; dmapGlobals.entityNum++, common->DmapPacifierCompileProgressIncrement( 1 ) )
	{
		uEntity_t* entity = &dmapGlobals.uEntities[dmapGlobals.entityNum];
		if( !entity->primitives )
		{
			continue;
		}

		//ImGui::Text( " Source code      :
		if( dmapGlobals.entityNum == 0 )
		{
			common->DmapPacifierInfo( "Current entity   : worldspawn" );
		}
		else
		{
			common->DmapPacifierInfo( "Current entity   : %s", entity->mapEntity->epairs.GetString( "name" ) );
		}

		common->VerbosePrintf( "############### entity %i ###############\n", dmapGlobals.entityNum );

		// if we leaked, stop without any more processing
		if( !ProcessModel( entity, ( bool )( dmapGlobals.entityNum == 0 ) ) )
		{
			return false;
		}

		// RB: dump BSP after nodes being pruned and optimized
		if( dmapGlobals.exportDebugVisuals )
		{
			uEntity_t* world = entity;

			WriteGLViewBSP( world->tree, "unpruned", dmapGlobals.entityNum, dmapGlobals.verboseentities );
		}

		// we usually don't want to see output for submodels unless
		// something strange is going on
		if( !dmapGlobals.verboseentities )
		{
			dmap_verbose.SetBool( false );
		}
	}

	dmap_verbose.SetBool( oldVerbose );

	return true;
}

/*
============
DmapHelp
============
*/
void DmapHelp()
{
	common->Printf(
		"Usage: dmap [options] mapfile\n"
		"Options:\n"
		"noCurves               = don't process curves\n"
		"noCM                   = don't create collision map\n"
		"noAAS                  = don't create AAS files\n"
		"noFlood                = skip area flooding = bad performance\n"
		"blockSize <x> <y> <z>  = cut BSP along these dimensions or disable with 0 0 0\n"
		"obj                    = export BSP render surfaces as .obj file\n"
		"debug                  = export BSP portals and other details as .obj files\n"
		""
	);
}

/*
============
ResetDmapGlobals
============
*/
void ResetDmapGlobals()
{
	dmapGlobals.mapFileBase[0] = '\0';
	dmapGlobals.dmapFile = NULL;
	dmapGlobals.mapPlanes.Clear();
	dmapGlobals.splitPlanesCounter.Clear();
	dmapGlobals.numEntities = 0;
	dmapGlobals.uEntities = NULL;
	dmapGlobals.entityNum = 0;
	dmapGlobals.mapLights.Clear();
	dmapGlobals.exportDebugVisuals = false;
	dmapGlobals.exportObj = false;
	dmapGlobals.asciiTree = false;
	dmapGlobals.noOptimize = false;
	dmapGlobals.verboseentities = false;
	dmapGlobals.noCurves = false;
	dmapGlobals.fullCarve = false;
	dmapGlobals.noModelBrushes = false;
	dmapGlobals.noTJunc = false;
	dmapGlobals.noMerge = false;
	dmapGlobals.noFlood = false;
	dmapGlobals.noClipSides = false;
	dmapGlobals.noLightCarve = false;
	dmapGlobals.drawBounds.Clear();
	dmapGlobals.drawflag = false;
	dmapGlobals.bspAlternateSplitWeights = false;
	dmapGlobals.blockSize = idVec3( 1024.0f, 1024.0f, 1024.0f );	// default block size for splitting
	dmapGlobals.inlineStatics = false;
	dmapGlobals.totalInlinedModels = 0;
}

/*
============
Dmap
============
*/
void Dmap( const idCmdArgs& args )
{
	int			i;
	int			start, end;
	char		path[1024];
	idStr		passedName;
	bool		leaked = false;
	bool		noCM = false;
	bool		noAAS = false;

	ResetDmapGlobals();

	if( args.Argc() < 2 )
	{
		DmapHelp();
		return;
	}

	common->Printf( "---- dmap ----\n" );

	dmapGlobals.fullCarve = true;
	dmapGlobals.noLightCarve = true;

	for( i = 1 ; i < args.Argc() ; i++ )
	{
		const char* s;

		s = args.Argv( i );
		if( s[0] == '-' )
		{
			s++;
			if( s[0] == '\0' )
			{
				continue;
			}
		}

		if( !idStr::Icmp( s, "glview" ) || !idStr::Icmp( s, "debug" ) )
		{
			dmapGlobals.exportDebugVisuals = true;
		}
		else if( !idStr::Icmp( s, "obj" ) )
		{
			dmapGlobals.exportObj = true;
		}
		else if( !idStr::Icmp( s, "asciiTree" ) )
		{
			dmapGlobals.asciiTree = true;
		}
		else if( !idStr::Icmp( s, "v" ) || !idStr::Icmp( s, "verbose" ) )
		{
			common->Printf( "verbose = true\n" );
			dmap_verbose.SetBool( true );
		}
		else if( !idStr::Icmp( s, "draw" ) )
		{
			common->Printf( "draw = true\n" );
			dmapGlobals.drawflag = true;
		}
		else if( !idStr::Icmp( s, "altsplit" ) )
		{
			common->Printf( "bspAlternateSplitWeights = true\n" );
			dmapGlobals.bspAlternateSplitWeights = true;
		}
		else if( !idStr::Icmp( s, "blockSize" ) )
		{
			if( i + 3 >= args.Argc() )
			{
				common->Error( "usage: dmap blockSize <x> <y> <z>" );
			}
			dmapGlobals.blockSize[0] = atof( args.Argv( i + 1 ) );
			dmapGlobals.blockSize[1] = atof( args.Argv( i + 2 ) );
			dmapGlobals.blockSize[2] = atof( args.Argv( i + 3 ) );
			common->Printf( "blockSize = %f %f %f\n", dmapGlobals.blockSize[0], dmapGlobals.blockSize[1], dmapGlobals.blockSize[2] );
			i += 3;
		}
		else if( !idStr::Icmp( s, "inlineAll" ) )
		{
			common->Printf( "inlineAll = true\n" );
			dmapGlobals.inlineStatics = true;
		}
		else if( !idStr::Icmp( s, "noMerge" ) )
		{
			common->Printf( "noMerge = true\n" );
			dmapGlobals.noMerge = true;
		}
		else if( !idStr::Icmp( s, "noFlood" ) )
		{
			common->Printf( "noFlood = true\n" );
			dmapGlobals.noFlood = true;
		}
		else if( !idStr::Icmp( s, "noLightCarve" ) )
		{
			common->Printf( "noLightCarve = true\n" );
			dmapGlobals.noLightCarve = true;
		}
		else if( !idStr::Icmp( s, "lightCarve" ) )
		{
			common->Printf( "noLightCarve = false\n" );
			dmapGlobals.noLightCarve = false;
		}
		else if( !idStr::Icmp( s, "noOpt" ) )
		{
			common->Printf( "noOptimize = true\n" );
			dmapGlobals.noOptimize = true;
		}
		else if( !idStr::Icmp( s, "verboseentities" ) )
		{
			common->Printf( "verboseentities = true\n" );
			dmapGlobals.verboseentities = true;
		}
		else if( !idStr::Icmp( s, "noCurves" ) )
		{
			common->Printf( "noCurves = true\n" );
			dmapGlobals.noCurves = true;
		}
		else if( !idStr::Icmp( s, "noModels" ) )
		{
			common->Printf( "noModels = true\n" );
			dmapGlobals.noModelBrushes = true;
		}
		else if( !idStr::Icmp( s, "noClipSides" ) )
		{
			common->Printf( "noClipSides = true\n" );
			dmapGlobals.noClipSides = true;
		}
		else if( !idStr::Icmp( s, "noCarve" ) )
		{
			common->Printf( "noCarve = true\n" );
			dmapGlobals.fullCarve = false;
		}
		else if( !idStr::Icmp( s, "noTjunc" ) )
		{
			// triangle optimization won't work properly without tjunction fixing
			common->Printf( "noTJunc = true\n" );
			dmapGlobals.noTJunc = true;
			dmapGlobals.noOptimize = true;
			common->Printf( "forcing noOptimize = true\n" );
		}
		else if( !idStr::Icmp( s, "noCM" ) )
		{
			noCM = true;
			common->Printf( "noCM = true\n" );
		}
		else if( !idStr::Icmp( s, "noAAS" ) )
		{
			noAAS = true;
			common->Printf( "noAAS = true\n" );
		}
		else
		{
			break;
		}
	}

	if( i >= args.Argc() )
	{
		common->Error( "usage: dmap [options] mapfile" );
	}

	passedName = args.Argv( i );		// may have an extension
	passedName.BackSlashesToSlashes();
	if( passedName.Icmpn( "maps/", 4 ) != 0 )
	{
		passedName = "maps/" + passedName;
	}

	common->DmapPacifierFilename( passedName, "Compiling BSP .proc" );

	idStr stripped = passedName;
	stripped.StripFileExtension();
	idStr::Copynz( dmapGlobals.mapFileBase, stripped, sizeof( dmapGlobals.mapFileBase ) );

	bool region = false;
	// if this isn't a regioned map, delete the last saved region map
	if( passedName.Right( 4 ) != ".reg" )
	{
		idStr::snPrintf( path, sizeof( path ), "%s.reg", dmapGlobals.mapFileBase );
		fileSystem->RemoveFile( path );
	}
	else
	{
		region = true;
	}


	passedName = stripped;

	// delete any old line leak files
	idStr::snPrintf( path, sizeof( path ), "%s.lin", dmapGlobals.mapFileBase );
	fileSystem->RemoveFile( path );

	// delete any old generated binary proc files
	idStr generated = va( "generated/%s.bproc", dmapGlobals.mapFileBase );
	fileSystem->RemoveFile( generated.c_str() );

	// delete any old generated binary cm files
	generated = va( "generated/%s.bcm", dmapGlobals.mapFileBase );
	fileSystem->RemoveFile( generated.c_str() );

	// delete any old ASCII collision files
	idStr::snPrintf( path, sizeof( path ), "%s.cm", dmapGlobals.mapFileBase );
	fileSystem->RemoveFile( path );

	//
	// start from scratch
	//
	start = Sys_Milliseconds();

	if( !LoadDMapFile( passedName ) )
	{
		return;
	}

	if( ProcessModels() )
	{
		WriteOutputFile();
	}
	else
	{
		leaked = true;
	}

	FreeDMapFile();

	common->Printf( "%i static models merged\n", dmapGlobals.totalInlinedModels );

	end = Sys_Milliseconds();
	common->Printf( "-----------------------\n" );
	common->Printf( "%5.0f seconds for dmap\n", ( end - start ) * 0.001f );
	common->DmapPacifierInfo( "%5.0f seconds for dmap\n", ( end - start ) * 0.001f );

	if( !leaked )
	{
		if( !noCM )
		{
#if !defined( DMAP )
			// make sure the collision model manager is not used by the game
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, "disconnect" );
#endif

			common->DmapPacifierFilename( passedName, "Generating .cm collision map" );

			// create the collision map
			start = Sys_Milliseconds();

			// write always a fresh .cm file
			collisionModelManager->LoadMap( dmapGlobals.dmapFile, true );
			collisionModelManager->FreeMap();

			end = Sys_Milliseconds();
			common->Printf( "-------------------------------------\n" );
			common->Printf( "%5.0f seconds to create collision map\n", ( end - start ) * 0.001f );
			common->DmapPacifierInfo( "%5.0f seconds to create collision map\n", ( end - start ) * 0.001f );
		}

		if( !noAAS && !region )
		{
			// create AAS files
			RunAAS_f( args );
		}

		common->DmapPacifierFilename( passedName, "Done" );
	}
	else
	{
		common->DmapPacifierFilename( passedName, "Failed due to errors. Quit program." );
	}


	// free the common .map representation
	delete dmapGlobals.dmapFile;

	// clear the map plane list
	dmapGlobals.mapPlanes.Clear();
}

/*
============
Dmap_f
============
*/
void Dmap_f( const idCmdArgs& args )
{
	common->ClearWarnings( "running dmap" );

	// refresh the screen each time we print so it doesn't look
	// like it is hung
	common->SetRefreshOnPrint( true );
	Dmap( args );
	common->SetRefreshOnPrint( false );

	common->PrintWarnings();
}
