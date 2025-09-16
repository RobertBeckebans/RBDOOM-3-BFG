/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2015-2025 Robert Beckebans

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


struct OBJFace
{
	OBJFace()
	{
		material = nullptr;
	}

	const idMaterial*			material;
	idList<idDrawVert>			verts;
	idList<triIndex_t>			indexes;

	bool						drawLines = false;
};

struct OBJGroup
{
	idStr						name;
	idList<OBJFace>				faces;
	int							area = -1;
};

int PortalVisibleSides( uPortal_t* p )
{
	int		fcon, bcon;

	if( !p->onnode )
	{
		return 0;    // outside
	}

	fcon = p->nodes[0]->opaque;
	bcon = p->nodes[1]->opaque;

	// same contents never create a face
	if( fcon == bcon )
	{
		return 0;
	}

	if( !fcon )
	{
		return 1;
	}
	if( !bcon )
	{
		return 2;
	}

	return 0;
}

idVec4 PickDebugColor( int area )
{
	static const idVec4 colors[] =
	{
		// Mixed sequence for stronger contrast
		colorRed,            colorDodgerBlue,
		colorDarkKhaki,      colorAqua,
		colorCrimson,        colorTeal,
		colorOlive,          colorCoral,
		colorDarkSlateGray,  colorYellow,
		colorBurlyWood,      colorDarkBlue,
		colorGreen,          colorBrown,
		colorLightSeaGreen,  colorGold,
		colorNavy,           colorOrange,
		colorDarkGoldenRod,  colorDarkSalmon,
		colorLightSteelBlue, colorBlack,
		colorGray,           colorDarkKhaki,
	};
	static const int numColors = sizeof( colors ) / sizeof( colors[0] );

	idVec4 color = ( area > -1 ) ? ( colors[area % numColors] ) : colorPink;

	// tint selected color a bit for each surface
	static	int	level = 128;
	float		light;
	level += 5;
	light = ( level & 55 ) / 255.0;

	color.x = idMath::ClampFloat( 0.0f, 1.0f, color.x + light );
	color.y = idMath::ClampFloat( 0.0f, 1.0f, color.y + light );
	color.z = idMath::ClampFloat( 0.0f, 1.0f, color.z + light );

	return color;
}

void OutputWinding( idWinding* w, OBJGroup& group, int area, bool reverse )
{
	idVec4 color = PickDebugColor( area );
	dword dcolor = PackColor( color );

	OBJFace& face = group.faces.Alloc();

	if( reverse )
	{
		for( int i = 0; i < w->GetNumPoints(); i++ )
		{
			idDrawVert& dv = face.verts.Alloc();

			dv.xyz.x = ( *w )[i][0];
			dv.xyz.y = ( *w )[i][1];
			dv.xyz.z = ( *w )[i][2];

			dv.SetColor( dcolor );

			//dv.SetNormal( w->GetPlane() )
		}
	}
	else
	{
		for( int i = w->GetNumPoints() - 1; i >= 0; i-- )
		{
			idDrawVert& dv = face.verts.Alloc();

			dv.xyz.x = ( *w )[i][0];
			dv.xyz.y = ( *w )[i][1];
			dv.xyz.z = ( *w )[i][2];

			dv.SetColor( dcolor );

			//dv.SetNormal( w->GetPlane() )
		}
	}
}

/*
=============
OutputPortal
=============
*/
void OutputPortal( uPortal_t* p, OBJGroup& group, bool touchingVoid )
{
	idWinding*	w;
	int		sides;

	sides = PortalVisibleSides( p );

	if( touchingVoid )
	{
		node_t* n0 = p->nodes[0];
		node_t* n1 = p->nodes[1];
		int a0 = n0->area;
		int a1 = n1->area;

		// only portals between an area (area != -1) and the void (area == -1)
		bool goingVoid = ( a0 != -1 && a1 == -1 ) || ( a0 == -1 && a1 != -1 );

		bool outside = !sides;
		bool passable = !p->nodes[0]->opaque && !p->nodes[1]->opaque;
		if( outside && !passable && goingVoid )
		{
			w = p->winding;
			OutputWinding( w, group, n0->area, false );
		}

		return;
	}

	if( !sides )
	{
		return;
	}

	w = p->winding;

	if( sides == 2 )  		// back side
	{
		w = w->Reverse();
	}

	OutputWinding( w, group, p->nodes[0]->area, false );

	if( sides == 2 )
	{
		delete w;
	}
}

// RB: this does not work in all maps
void CollectOutsidePortals_r( node_t* node, OBJGroup& group )
{
	uPortal_t*	p, *nextp;

	if( node->planenum != PLANENUM_LEAF )
	{
		CollectOutsidePortals_r( node->children[0], group );
		CollectOutsidePortals_r( node->children[1], group );
		return;
	}

	//if( !touchingOutside && node->opaque )
	//{
	//	return;
	//}

	// write all the portals
	for( p = node->portals; p; p = nextp )
	{
		idWinding* w = p->winding;
		int s = ( p->nodes[1] == node );
		nextp = p->next[s];

		if( w != NULL )
		{
			OutputPortal( p, group, true );
		}
	}
}

// like q3map WritePortalFile_r
// collect all portals within the BSP that you can walk through
void CollectPortals_r( node_t* node, OBJGroup& group )
{
	uPortal_t*	p, *nextp;

	if( node->planenum != PLANENUM_LEAF )
	{
		CollectPortals_r( node->children[0], group );
		CollectPortals_r( node->children[1], group );
		return;
	}

	if( node->opaque )
	{
		return;
	}

	// write all the portals
	for( p = node->portals; p; p = nextp )
	{
		idWinding* w = p->winding;
		int s = ( p->nodes[1] == node );
		nextp = p->next[s];

		if( w != NULL )
		{
			// only write out from first leaf
			if( p->nodes[0] == node )
			{
				if( !Portal_Passable( p ) )
				{
					continue;
				}

				OutputWinding( w, group, node->area, false );
			}
		}
	}
}

// like q3map WriteFaceFile_r
void CollectFaces_r( node_t* node, OBJGroup& group )
{
	if( node->planenum != PLANENUM_LEAF )
	{
		CollectFaces_r( node->children[0], group );
		CollectFaces_r( node->children[1], group );
		return;
	}

	if( node->opaque )
	{
		return;
	}

	// write all the portals
#if 0
	uPortal_t*	p, *nextp;
	for( p = node->portals; p; p = nextp )
	{
		idWinding* w = p->winding;
		int s = ( p->nodes[1] == node );
		nextp = p->next[s];

		if( w != NULL )
		{
			if( Portal_Passable( p ) )
			{
				continue;
			}

			OutputWinding( w, group, node->area, p->nodes[0] != node );
		}
	}
#else
	uBrush_t* brush;
	side_t*	s;
	for( brush = node->brushlist; brush ; brush = brush->next )
	{
		for( int i = 0 ; i < brush->numsides; i++ )
		{
			s = &brush->sides[i];
			if( !s->winding )
			{
				continue;
			}

			OutputWinding( s->winding, group, node->area, false );
		}
	}
#endif
}


void OutputQuad( const idVec3 verts[4], OBJGroup* group, bool reverse )
{
	OBJFace& face = group->faces.Alloc();
	face.drawLines = true;

	idDrawVert& dv0 = face.verts.Alloc();
	idDrawVert& dv1 = face.verts.Alloc();
	idDrawVert& dv2 = face.verts.Alloc();
	idDrawVert& dv3 = face.verts.Alloc();

	idVec4 color = PickDebugColor( group->area );
	dword dcolor = PackColor( color );

	dv0.SetColor( dcolor );
	dv1.SetColor( dcolor );
	dv2.SetColor( dcolor );
	dv3.SetColor( dcolor );

	if( reverse )
	{
		dv0.xyz = verts[3];
		dv1.xyz = verts[2];
		dv2.xyz = verts[1];
		dv3.xyz = verts[0];
	}
	else
	{
		dv0.xyz = verts[0];
		dv1.xyz = verts[1];
		dv2.xyz = verts[2];
		dv3.xyz = verts[3];
	}
}

void OutputNode( const node_t* node, idList<OBJGroup>& groups )
{
	const idBounds& bounds = node->bounds;

	if( bounds.IsCleared() )
	{
		return;
	}

	OBJGroup* group = NULL;
	bool reverse = false;

	if( node->planenum == PLANENUM_LEAF )
	{
		//if( !node->opaque )
		{
			//if( node->area != -1 )
			{
				group = &groups.Alloc();
				if( node->opaque )
				{
					group->name.Format( "area%i_leaf_opaque.%i", node->area, node->nodeNumber );
				}
				else
				{
					group->name.Format( "area%i_leaf.%i", node->area, node->nodeNumber );
				}
				group->area = node->area;
			}
		}
	}

	if( group )
	{
		idVec3 verts[4];

		verts[0].Set( bounds[0][0], bounds[0][1], bounds[0][2] );
		verts[1].Set( bounds[0][0], bounds[1][1], bounds[0][2] );
		verts[2].Set( bounds[0][0], bounds[1][1], bounds[1][2] );
		verts[3].Set( bounds[0][0], bounds[0][1], bounds[1][2] );
		OutputQuad( verts, group, reverse );

		verts[0].Set( bounds[1][0], bounds[0][1], bounds[1][2] );
		verts[1].Set( bounds[1][0], bounds[1][1], bounds[1][2] );
		verts[2].Set( bounds[1][0], bounds[1][1], bounds[0][2] );
		verts[3].Set( bounds[1][0], bounds[0][1], bounds[0][2] );
		OutputQuad( verts, group, reverse );

		verts[0].Set( bounds[0][0], bounds[0][1], bounds[1][2] );
		verts[1].Set( bounds[0][0], bounds[1][1], bounds[1][2] );
		verts[2].Set( bounds[1][0], bounds[1][1], bounds[1][2] );
		verts[3].Set( bounds[1][0], bounds[0][1], bounds[1][2] );
		OutputQuad( verts, group, reverse );

		verts[0].Set( bounds[1][0], bounds[0][1], bounds[0][2] );
		verts[1].Set( bounds[1][0], bounds[1][1], bounds[0][2] );
		verts[2].Set( bounds[0][0], bounds[1][1], bounds[0][2] );
		verts[3].Set( bounds[0][0], bounds[0][1], bounds[0][2] );
		OutputQuad( verts, group, reverse );

		verts[0].Set( bounds[0][0], bounds[0][1], bounds[0][2] );
		verts[1].Set( bounds[0][0], bounds[0][1], bounds[1][2] );
		verts[2].Set( bounds[1][0], bounds[0][1], bounds[1][2] );
		verts[3].Set( bounds[1][0], bounds[0][1], bounds[0][2] );
		OutputQuad( verts, group, reverse );

		verts[0].Set( bounds[1][0], bounds[1][1], bounds[0][2] );
		verts[1].Set( bounds[1][0], bounds[1][1], bounds[1][2] );
		verts[2].Set( bounds[0][0], bounds[1][1], bounds[1][2] );
		verts[3].Set( bounds[0][0], bounds[1][1], bounds[0][2] );
		OutputQuad( verts, group, reverse );
	}
}

void OutputSplitPlane( const node_t* node, idList<OBJGroup>& groups )
{
	const idBounds& bounds = node->bounds;
	if( bounds.IsCleared() )
	{
		return;
	}

	if( node->planenum != PLANENUM_LEAF )
	{
		int planeCounter = 0;

		int* value;
		if( dmapGlobals.splitPlanesCounter.Get( node->planenum, &value ) && value != NULL )
		{
			planeCounter = *value;
		}

		if( planeCounter < 10 )
		{
			return;
		}

		idPlane		plane;

		idFixedWinding w;

		w.BaseForPlane( dmapGlobals.mapPlanes[node->planenum] );

		OBJGroup& group = groups.Alloc();
		group.name.Format( "splitplane.%i_%i", node->nodeNumber, planeCounter ) ;

		// cut down to AABB size
		for( int i = 0 ; i < 3 ; i++ )
		{
			plane[0] = plane[1] = plane[2] = 0;
			plane[i] = 1;
			plane[3] = -bounds[1][i];

			w.ClipInPlace( -plane );

			plane[i] = -1;
			plane[3] = bounds[0][i];

			w.ClipInPlace( -plane );
		}

		OutputWinding( &w, group, node->area, false );
	}
}

void OutputAreaPortalTriangles( const node_t* node, idList<OBJGroup>& groups )
{
	const idBounds& bounds = node->bounds;

	if( bounds.IsCleared() )
	{
		return;
	}

	if( node->planenum == PLANENUM_LEAF && node->areaPortalTris )
	{
		OBJGroup& group = groups.Alloc();
		group.name.Format( "areaPortalTris.%i", node->nodeNumber ) ;

		for( mapTri_t* tri = node->areaPortalTris; tri; tri = tri->next )
		{
			OBJFace& face = group.faces.Alloc();

			for( int i = 0; i < 3; i++ )
			{
				idDrawVert& dv = face.verts.Alloc();

				dv = tri->v[i];
			}
		}
	}
}

void CollectNodes_r( node_t* node, idList<OBJGroup>& groups )
{
	if( node->planenum != PLANENUM_LEAF )
	{
		//OutputSplitPlane( node, groups );

		CollectNodes_r( node->children[0], groups );
		CollectNodes_r( node->children[1], groups );
		return;
	}

	OutputNode( node, groups );

	//OutputAreaPortalTriangles( node, groups );
}

// RB: slightly changed variant from output.cpp to number both nodes and leafs
int NumberNodes_r( node_t* node, int nextNode, int& nextLeaf )
{
	if( node->planenum == PLANENUM_LEAF )
	{
		node->nodeNumber = nextLeaf++;
		return nextNode;
	}

	node->nodeNumber = nextNode;
	nextNode++;
	nextNode = NumberNodes_r( node->children[0], nextNode, nextLeaf );
	nextNode = NumberNodes_r( node->children[1], nextNode, nextLeaf );

	return nextNode;
}

void CollectAreaPortals( idList<OBJGroup>& groups )
{
	int			i;
	interAreaPortal_t*	iap;
	idWinding*			w;

	for( i = 0; i < interAreaPortals.Num(); i++ )
	{
		iap = &interAreaPortals[i];

		if( iap->side )
		{
			w = iap->side->winding;
		}
		else
		{
			w = & iap->w;
		}

		OBJGroup& group = groups.Alloc();
		group.name.Format( "interAreaPortal.%i", i );
		OutputWinding( w, group, -1, false );
	}
}

/*
=============
WriteGLView
=============
*/
void WriteGLViewBSP( tree_t* tree, const char* source, int entityNum, bool force )
{
	//c_glfaces = 0;
	//common->Printf( "Writing %s\n", source );

	if( entityNum != 0 && !force )
	{
		return;
	}

	idStrStatic< MAX_OSPATH > path;
	path.Format( "%s_BSP_%s_%i.obj", dmapGlobals.mapFileBase, source, entityNum );
	idFileLocal objFile( fileSystem->OpenFileWrite( path, "fs_basepath" ) );

	idList<OBJGroup> groups;

	// like in q3map WritePortalFile_r
	OBJGroup& portals = groups.Alloc();
	portals.name = "portals";
	CollectPortals_r( tree->headnode, portals );

	common->Printf( "%5i c_glfaces\n", portals.faces.Num() );

#if 0
	OBJGroup& portals2 = groups.Alloc();
	portals2.name = "void_portals";
	CollectPortals_r( tree->headnode, portals2, true );

	// like in q3map WriteFaceFile_r
	OBJGroup& faces = groups.Alloc();
	faces.name = "faces";
	CollectFaces_r( tree->headnode, faces );
#endif

	int numLeafs = 0;
	int numNodes = NumberNodes_r( tree->headnode, 0, numLeafs );

	CollectNodes_r( tree->headnode, groups );

#if 0
	if( entityNum == 0 )
	{
		CollectAreaPortals( groups );
	}
#endif

	int numVerts = 0;

	for( int g = 0; g < groups.Num(); g++ )
	{
		const OBJGroup& group = groups[g];

		objFile->Printf( "g %s\n", group.name.c_str() );

		for( int i = 0; i < group.faces.Num(); i++ )
		{
			const OBJFace& face = group.faces[i];

			for( int j = 0; j < face.verts.Num(); j++ )
			{
				const idVec3& v = face.verts[j].xyz;

				idVec3 c;
				UnpackColor( *reinterpret_cast<const dword*>( face.verts[j].color ), c );
				objFile->Printf( "v %1.6f %1.6f %1.6f %1.6f %1.6f %1.6f\n",
								 v.x, v.y, v.z, c.x, c.y, c.z );
			}

			if( face.drawLines )
			{
				// write lines connecting consecutive vertices (and close the loop)
				for( int j = 0; j < face.verts.Num(); j++ )
				{
					int v1 = numVerts + 1 + j;
					int v2 = numVerts + 1 + ( j + 1 ) % face.verts.Num(); // connect to next vertex, or first if last
					objFile->Printf( "l %i %i\n", v1, v2 );
				}
			}
			else
			{
				objFile->Printf( "f " );
				for( int j = 0; j < face.verts.Num(); j++ )
				{
					objFile->Printf( "%i// ", numVerts + 1 + j );
				}
			}

			numVerts += face.verts.Num();

			objFile->Printf( "\n\n" );
		}
	}
}


void WriteGLViewFacelist( bspFace_t* list, const char* source, bool force )
{
	if( dmapGlobals.entityNum != 0 && !force )
	{
		return;
	}

	idStrStatic< MAX_OSPATH > path;
	path.Format( "%s_BSP_%s_%i.obj", dmapGlobals.mapFileBase, source, dmapGlobals.entityNum );
	idFileLocal objFile( fileSystem->OpenFileWrite( path, "fs_basepath" ) );

	idList<OBJFace> faces;

	for( bspFace_t*	face = list ; face ; face = face->next )
	{
		if( face->portal )
		{
			continue;
		}

		OBJFace& objFace = faces.Alloc();

		//for( int i = 0; i < face->w->GetNumPoints(); i++ )
		for( int i = face->w->GetNumPoints() - 1; i >= 0; i-- )
		{
			idDrawVert& dv = objFace.verts.Alloc();

			dv.xyz.x = ( *face->w )[i][0];
			dv.xyz.y = ( *face->w )[i][1];
			dv.xyz.z = ( *face->w )[i][2];

			//dv.SetColor( level & 255 );
		}
	}

	int numVerts = 0;

	// give every surface a different color
	static idVec4 colors[] = { colorRed, colorGreen, colorBlue, colorYellow, colorMagenta, colorCyan, colorWhite, colorPurple };

	for( int i = 0; i < faces.Num(); i++ )
	{
		OBJFace& face = faces[i];

		for( int j = 0; j < face.verts.Num(); j++ )
		{
			const idVec3& v = face.verts[j].xyz;
			idVec4 c = colors[i & 7];

			objFile->Printf( "v %1.6f %1.6f %1.6f %1.6f %1.6f %1.6f\n",
							 v.x, v.y, v.z,
							 c[0], c[1], c[2] );
		}

		objFile->Printf( "f " );
		for( int j = 0; j < face.verts.Num(); j++ )
		{
			objFile->Printf( "%i// ", numVerts + 1 + j );
		}

		numVerts += face.verts.Num();

		objFile->Printf( "\n\n" );
	}
}
