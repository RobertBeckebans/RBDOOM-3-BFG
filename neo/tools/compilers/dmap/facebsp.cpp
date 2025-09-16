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

int			c_faceLeafs;


extern	int	c_nodes;

void RemovePortalFromNode( uPortal_t* portal, node_t* l );

node_t* NodeForPoint( node_t* node, const idVec3& origin )
{
	while( node->planenum != PLANENUM_LEAF )
	{
		idPlane& plane = dmapGlobals.mapPlanes[node->planenum];

		int side = plane.Side( origin, 0.1F );
		if( side == SIDE_FRONT || side == SIDE_ON )
		{
			node = node->children[0];
		}
		else
		{
			node = node->children[1];
		}
	}

	return node;
}



/*
=============
FreeTreePortals_r
=============
*/
void FreeTreePortals_r( node_t* node )
{
	uPortal_t*	p, *nextp;
	int			s;

	// free children
	if( node->planenum != PLANENUM_LEAF )
	{
		FreeTreePortals_r( node->children[0] );
		FreeTreePortals_r( node->children[1] );
	}

	// free portals
	for( p = node->portals ; p ; p = nextp )
	{
		s = ( p->nodes[1] == node );
		nextp = p->next[s];

		RemovePortalFromNode( p, p->nodes[!s] );
		FreePortal( p );
	}
	node->portals = NULL;
}

/*
=============
FreeTree_r
=============
*/
void FreeTree_r( node_t* node )
{
	// free children
	if( node->planenum != PLANENUM_LEAF )
	{
		FreeTree_r( node->children[0] );
		FreeTree_r( node->children[1] );
	}

	// free brushes
	FreeBrushList( node->brushlist );

	// free the node
	c_nodes--;
	Mem_Free( node );
}


/*
=============
FreeTree
=============
*/
void FreeTree( tree_t* tree )
{
	if( !tree )
	{
		return;
	}
	FreeTreePortals_r( tree->headnode );
	FreeTree_r( tree->headnode );
	Mem_Free( tree );
}

//===============================================================

void PrintTree_r( node_t* node, int depth )
{
	for( int i = 0 ; i < depth; i++ )
	{
		common->Printf( "  " );
	}

	if( node->planenum == PLANENUM_LEAF )
	{
		/*
		if( !node->brushlist )
		{
			common->Printf( "NULL\n" );
		}
		else
		{
			for( uBrush_t* bb = node->brushlist ; bb ; bb = bb->next )
			{
				common->Printf( "%i ", bb->original->brushnum );
			}
			common->Printf( "\n" );
		}
		*/

		common->Printf( "leaf %i", node->nodeNumber );
		if( node->area >= 0 )
		{
			common->Printf( " area %i", node->area );
		}
		if( node->opaque )
		{
			common->Printf( " opaque" );
		}
		if( node->occupied )
		{
			common->Printf( " occupied" );
		}
		common->Printf( "\n" );
		return;
	}

	idPlane& plane = dmapGlobals.mapPlanes[node->planenum];
	common->Printf( "#%i plane = %i (%5.2f %5.2f %5.2f %5.2f)\n", node->nodeNumber, node->planenum,
					plane[0], plane[1], plane[2], plane[3] );

	PrintTree_r( node->children[0], depth + 1 );
	PrintTree_r( node->children[1], depth + 1 );
}

/*
================
AllocBspFace
================
*/
bspFace_t*	AllocBspFace()
{
	bspFace_t*	f;

	f = ( bspFace_t* )Mem_Alloc( sizeof( *f ), TAG_TOOLS );
	memset( f, 0, sizeof( *f ) );

	return f;
}

/*
================
FreeBspFace
================
*/
void	FreeBspFace( bspFace_t* f )
{
	if( f->w )
	{
		delete f->w;
	}
	Mem_Free( f );
}


/*
================
SelectSplitPlaneNum  (revised)
- Two-stage approach: strict (portal + axial + wall) -> relaxed (portal + axial) -> fallback (original-like)
- Prioritizes axial/wall/portal planes, penalizes excessive split winding crosses and highly unbalanced partitions.
================
*/
int SelectSplitPlaneNum( node_t* node, bspFace_t* list )
{
	// ---- Tunable heuristics (adjust as needed, or move into dmapGlobals) ----
	const bool enforceTwoStage = true;              // run strict stage first
	const float axialDotThreshold = 0.90f; // 0.97f // how strict the "axial" definition is
	const float wallZMax = 0.20f;                   // |nz| < wallZMax => almost vertical (wall)
	const float nearEdgePenalty = 50.0f;            // penalty for splits near the node boundary (avoid fine slices)
	const float areaBiasScale = 10.0f;              // scale factor for surface area bonus
	const int splitsPenalty = 12;                   // penalty per split cross
	const int facingPenalty = 6;                    // penalty per facing plane
	const int balancePenaltyScale = 2;              // penalty for front/back imbalance
	// -------------------------------------------------------------------------

	bspFace_t* split;
	bspFace_t* check;
	bspFace_t* bestSplit = NULL;
	int statsSplits = 0, statsFacing = 0, statsFront = 0, statsBack = 0;
	int side;
	idPlane* mapPlane;
	int bestValue;
	idPlane plane;
	int planenum;
	bool havePortals;
	float dist;
	idVec3 halfSize;

	// if it is crossing a 1k block boundary, force a split
	// this prevents epsilon problems from extending an
	// arbitrary distance across the map

	halfSize = ( node->bounds[1] - node->bounds[0] ) * 0.5f;
	for( int axis = 0; axis < 3; axis++ )
	{
		if( dmapGlobals.blockSize[axis] <= 0.0f )
		{
			continue;
		}

		float axisBlockSize = dmapGlobals.blockSize[axis];
		if( halfSize[axis] > axisBlockSize )
		{
			dist = axisBlockSize * ( floor( ( node->bounds[0][axis] + halfSize[axis] ) / axisBlockSize ) + 1.0f );
		}
		else
		{
			dist = axisBlockSize * ( floor( node->bounds[0][axis] / axisBlockSize ) + 1.0f );
		}

		if( dist > node->bounds[0][axis] + 1.0f && dist < node->bounds[1][axis] - 1.0f )
		{
			plane[0] = plane[1] = plane[2] = 0.0f;
			plane[axis] = 1.0f;
			plane[3] = -dist;
			planenum = FindFloatPlane( plane );
			return planenum;
		}
	}

	// Initialize "checked" flags and detect if portals exist
	bestValue = INT_MIN;
	havePortals = false;
	for( split = list ; split ; split = split->next )
	{
		split->checked = false;
		if( split->portal )
		{
			havePortals = true;
		}
	}

#if 0
	// Helper lambda: score a plane (used for candidate evaluation)
	auto scorePlane = [&]( bspFace_t* candidate, bool preferWallsOnly ) -> int
	{
		mapPlane = &dmapGlobals.mapPlanes[ candidate->planenum ];

		// Gather statistics for this plane across the face list
		statsSplits = 0, statsFacing = 0, statsFront = 0, statsBack = 0;

		for( check = list ; check ; check = check->next )
		{
			if( check->planenum == candidate->planenum )
			{
				statsFacing++;
				continue;
			}
			int s = check->w->PlaneSide( *mapPlane );
			if( s == SIDE_CROSS )
			{
				statsSplits++;
			}
			else if( s == SIDE_FRONT )
			{
				statsFront++;
			}
			else if( s == SIDE_BACK )
			{
				statsBack++;
			}
		}

		// Axis/wall detection
		float nx = ( *mapPlane )[0];
		float ny = ( *mapPlane )[1];
		float nz = ( *mapPlane )[2];
		float absNx = idMath::Fabs( nx );
		float absNy = idMath::Fabs( ny );
		float absNz = idMath::Fabs( nz );

		bool isAxial = ( mapPlane->Type() < PLANETYPE_TRUEAXIAL ) ||
					   ( absNx > axialDotThreshold || absNy > axialDotThreshold || absNz > axialDotThreshold );

		// Wall: nearly vertical (small z) and strongly aligned to X or Y
		bool isWall = ( idMath::Fabs( nz ) < wallZMax ) && ( absNx > 0.8f || absNy > 0.8f );

		// If wall-only mode is requested and this plane is not a wall, disqualify
		if( preferWallsOnly && !isWall )
		{
			return INT_MIN;
		}

		// Base score
		int score = 0;

		// disqualify non-portal planes if portals are present
		if( havePortals && !candidate->portal )
		{
			//v += 1200;
			return INT_MIN;
		}

		// Axial and wall bonuses
		if( isAxial )
		{
			score += 200;
		}

		if( isWall )
		{
			score += 350;
		}

		// Area bonus (larger surfaces get higher score)
		score += ( int )( candidate->w->GetArea() * areaBiasScale );

		// Penalties: many splits, many facing planes, unbalanced front/back
		score -= statsSplits * splitsPenalty;
		score -= statsFacing * facingPenalty;
		score -= idMath::Fabs( statsFront - statsBack ) * balancePenaltyScale;

		// Penalty if the plane is very close to a node boundary (produces small leaves)
		// Check only for mostly axial planes (more meaningful there)
#if 1
		if( isAxial )
		{
			// Find dominant axis
			int axis = 0;
			if( absNy > absNx && absNy > absNz )
			{
				axis = 1;
			}
			else if( absNz > absNx && absNz > absNy )
			{
				axis = 2;
			}
			float nodeSpan = node->bounds[1][axis] - node->bounds[0][axis];
			if( nodeSpan > 0.0f )
			{
				float planeDist = mapPlane->Dist();
				float t = ( planeDist - node->bounds[0][axis] ) / nodeSpan; // 0..1 in node space
				if( t < 0.12f || t > 0.88f )
				{
					score -= ( int )nearEdgePenalty;
				}
			}
		}
#endif

		// Return computed score (negative values possible)
		return score;
	};

	if( dmapGlobals.bspAlternateSplitWeights )
	{
		// Two-phase: 1) strict: prefer walls only (if enabled)
		// 2) relaxed axial only
		// 3) fallback original style
		// Phase controlled by preferWallsOnly / preferAxialOnly
		for( int phase = 0; phase < 3; phase++ )
		{
			bool preferWallsOnly = ( phase == 0 ) && enforceTwoStage;	// stricter: wall only
			bool preferAxialOnly = ( phase <= 1 );                      // phases 0 and 1 prefer axial
			bestValue = INT_MIN;
			bestSplit = NULL;

			for( split = list ; split ; split = split->next )
			{
				if( split->checked )
				{
					continue;
				}
				if( havePortals != split->portal )
				{
					continue;    // if portals exist, only consider portal planes
				}

				mapPlane = &dmapGlobals.mapPlanes[ split->planenum ];

				// Quick axial filter if we want axial only in this phase
				if( preferAxialOnly )
				{
					float ax0 = idMath::Fabs( ( *mapPlane )[0] );
					float ax1 = idMath::Fabs( ( *mapPlane )[1] );
					float ax2 = idMath::Fabs( ( *mapPlane )[2] );
					if( !( mapPlane->Type() < PLANETYPE_TRUEAXIAL ||
							ax0 > axialDotThreshold || ax1 > axialDotThreshold || ax2 > axialDotThreshold ) )
					{
						// skip non-axial candidate in axial phases
						continue;
					}
				}

				int score = scorePlane( split, preferWallsOnly );

				// If score is INT_MIN => disqualified
				if( score == INT_MIN )
				{
					continue;
				}

				// Mark same planes as checked (avoid retesting)
				if( score > bestValue )
				{
					bestValue = score;
					bestSplit = split;
				}
			}

			if( bestSplit )
			{
				// Mark all faces with the same planenum as checked (like original behavior)
				for( check = list ; check ; check = check->next )
				{
					if( check->planenum == bestSplit->planenum )
					{
						check->checked = true;
					}
				}
				return bestSplit->planenum;
			}

			// If no candidate found in this phase, go to the next (more relaxed)
		}
	}
#endif

	// Fallback: Old heuristic based on front/back/splits
	// Try to at least return some planenum, scoring without axial/wall restrictions
	bestValue = INT_MIN;
	bestSplit = list;

	int numFaces = 0;
	for( split = list ; split ; split = split->next )
	{
		split->checked = false;
		numFaces++;
	}

	for( split = list ; split ; split = split->next )
	{
		if( split->checked )
		{
			continue;
		}
		if( havePortals != split->portal )
		{
			continue;
		}

		mapPlane = &dmapGlobals.mapPlanes[ split->planenum ];

		statsSplits = 0, statsFacing = 0, statsFront = 0, statsBack = 0;

		for( check = list ; check ; check = check->next )
		{
			if( check->planenum == split->planenum )
			{
				statsFacing++;
				check->checked = true;
				continue;
			}

			side = check->w->PlaneSide( *mapPlane );
			if( side == SIDE_CROSS )
			{
				statsSplits++;
			}
			else if( side == SIDE_FRONT )
			{
				statsFront++;
			}
			else if( side == SIDE_BACK )
			{
				statsBack++;
			}
		}

		int score;

#if 1
		if( dmapGlobals.bspAlternateSplitWeights )
		{
			// original idea by 27 of the Urban Terror team

			float sizeBias = split->w->GetArea();
			int planeCounter = 0;

			int* value;
			if( dmapGlobals.splitPlanesCounter.Get( split->planenum, &value ) && value != NULL )
			{
				planeCounter = *value;
			}

			score = numFaces * 10;
			//score = 20000;								// balanced base value
			score -= ( abs( statsFront - statsBack ) ); 	// prefer centered planes
			score -= planeCounter * 1;						// avoid reusing the same splitting plane
			score -= statsFacing;
			score -= statsSplits * 5;
			score += ( int )( sizeBias * areaBiasScale );
		}
		else
#endif
		{
			// original by id Software used in Quake 3 and Doom 3
			score = 5 * statsFacing;	// the more faces share the same plane, the better
			score -= 5 * statsSplits;	// avoid splits
			//score -= ( abs( statsFront - statsBack ) );
			if( mapPlane->Type() < PLANETYPE_TRUEAXIAL )
			{
				score += 5;
			}
		}

		if( score > bestValue )
		{
			bestValue = score;
			bestSplit = split;
		}
	}

	if( bestValue == INT_MIN )
	{
		return -1;
	}

	return bestSplit->planenum;
}


/*
================
BuildFaceTree_r
================
*/
void	BuildFaceTree_r( node_t* node, bspFace_t* list )
{
	bspFace_t*	split;
	bspFace_t*	next;
	int			side;
	bspFace_t*	newFace;
	bspFace_t*	childLists[2];
	idWinding*	frontWinding, *backWinding;
	int			i;
	int			splitPlaneNum;

	splitPlaneNum = SelectSplitPlaneNum( node, list );

	// if we don't have any more faces, this is a node
	if( splitPlaneNum == -1 )
	{
		node->planenum = PLANENUM_LEAF;
		c_faceLeafs++;
		return;
	}

	// RB: increase split plane counter
	int* value;
	if( dmapGlobals.splitPlanesCounter.Get( splitPlaneNum, &value ) && value != NULL )
	{
		( *value )++;
	}
	else
	{
		dmapGlobals.splitPlanesCounter.Set( splitPlaneNum, 1 );
	}
	// RB end

	// partition the list
	node->planenum = splitPlaneNum;
	idPlane& plane = dmapGlobals.mapPlanes[ splitPlaneNum ];
	childLists[0] = NULL;
	childLists[1] = NULL;
	for( split = list ; split ; split = next )
	{
		next = split->next;

		if( split->planenum == node->planenum )
		{
			FreeBspFace( split );
			continue;
		}

		side = split->w->PlaneSide( plane );

		if( side == SIDE_CROSS )
		{
			split->w->Split( plane, CLIP_EPSILON * 2, &frontWinding, &backWinding );
			if( frontWinding )
			{
				newFace = AllocBspFace();
				newFace->w = frontWinding;
				newFace->next = childLists[0];
				newFace->planenum = split->planenum;
				childLists[0] = newFace;
			}
			if( backWinding )
			{
				newFace = AllocBspFace();
				newFace->w = backWinding;
				newFace->next = childLists[1];
				newFace->planenum = split->planenum;
				childLists[1] = newFace;
			}
			FreeBspFace( split );
		}
		else if( side == SIDE_FRONT )
		{
			split->next = childLists[0];
			childLists[0] = split;
		}
		else if( side == SIDE_BACK )
		{
			split->next = childLists[1];
			childLists[1] = split;
		}
	}


	// recursively process children
	for( i = 0 ; i < 2 ; i++ )
	{
		node->children[i] = AllocNode();
		node->children[i]->parent = node;
		node->children[i]->bounds = node->bounds;
	}

	// split the bounds if we have a nice axial plane
	for( i = 0 ; i < 3 ; i++ )
	{
		if( idMath::Fabs( plane[i] - 1.0 ) < 0.001 )
		{
			node->children[0]->bounds[0][i] = plane.Dist();
			node->children[1]->bounds[1][i] = plane.Dist();
			break;
		}
	}

	for( i = 0 ; i < 2 ; i++ )
	{
		BuildFaceTree_r( node->children[i], childLists[i] );
	}
}


/*
================
FaceBSP

List will be freed before returning
================
*/
tree_t* FaceBSP( bspFace_t* list )
{
	tree_t*		tree;
	bspFace_t*	face;
	int			i;
	int			count;
	int			start, end;

	start = Sys_Milliseconds();

	common->VerbosePrintf( "--- FaceBSP ---\n" );

	// RB: every model gets its own split plane usage counter
	dmapGlobals.splitPlanesCounter.Clear();

	tree = AllocTree();

	count = 0;
	tree->bounds.Clear();
	for( face = list ; face ; face = face->next )
	{
		count++;
		for( i = 0 ; i < face->w->GetNumPoints() ; i++ )
		{
			tree->bounds.AddPoint( ( *face->w )[i].ToVec3() );
		}

		if( face->simpleBSP )
		{
			tree->simpleBSP = true;
		}
	}
	common->VerbosePrintf( "%5i faces\n", count );

	tree->headnode = AllocNode();
	tree->headnode->bounds = tree->bounds;
	c_faceLeafs = 0;

	BuildFaceTree_r( tree->headnode, list );

	end = Sys_Milliseconds();

	common->VerbosePrintf( "%5.1f seconds faceBsp\n", ( end - start ) / 1000.0 );

	if( dmapGlobals.entityNum == 0 )
	{
		int depth = log2f( c_faceLeafs + 1 );
		common->Printf( "BSP depth = %i and %5i leafs\n", depth, c_faceLeafs );
		common->Printf( "%5i split planes\n", dmapGlobals.splitPlanesCounter.Num() );

		if( dmapGlobals.bspAlternateSplitWeights && dmapGlobals.entityNum == 0 )
		{
			for( int i = 0; i < dmapGlobals.splitPlanesCounter.Num(); i++ )
			{
				int key;
				dmapGlobals.splitPlanesCounter.GetIndexKey( i, key );
				int* value = dmapGlobals.splitPlanesCounter.GetIndex( i );

				idLib::Printf( "%d\t%d\n", key, *value );
			}
		}

		/*
		if( dmapGlobals.entityNum == 2 )
		{
			int numLeafs = 0;
			int numNodes = NumberNodes_r( tree->headnode, 0, numLeafs );

			PrintTree_r(tree->headnode, depth );
		}
		*/
	}

	return tree;
}

//==========================================================================

/*
=================
MakeStructuralBspFaceList
=================
*/
bspFace_t*	MakeStructuralBspFaceList( primitive_t* list )
{
	uBrush_t*	b;
	int			i;
	side_t*		s;
	idWinding*	w;
	bspFace_t*	f, *flist;
	mapTri_t*	tri;
	primitive_t* prims;

	prims = list;
	flist = NULL;
	if( dmapGlobals.entityNum != 0 )
	{
		idBounds bounds;
		bounds.Clear();

		for( ; list; list = list->next )
		{
			tri = list->polyTris;
			if( tri )
			{
				for( ; tri; tri = tri->next )
				{
					bounds.AddPoint( tri->v[0].xyz );
					bounds.AddPoint( tri->v[1].xyz );
					bounds.AddPoint( tri->v[2].xyz );
				}

				continue;
			}
		}

		if( !bounds.IsCleared() )
		{
			b = BrushFromBounds( bounds );
			//b->substractive = true;
			b->simpleBSP = true;
			b->opaque = true;
			b->entitynum = dmapGlobals.entityNum;
			b->contentShader = declManager->FindMaterial( "textures/common/caulk", false );
			b->contents = b->contentShader->GetContentFlags();

			for( i = 0; i < b->numsides; i++ )
			{
				s = &b->sides[i];
				s->material = b->contentShader;
			}

			primitive_t* prim = ( primitive_t* )Mem_Alloc( sizeof( *prim ), TAG_TOOLS );
			memset( prim, 0, sizeof( *prim ) );
			prim->next = prims;
			prims = prim;

			prim->brush = b;

			// TODO tell ProcessModel() we are using the simple structural BSP
		}

	}

	for( list = prims; list; list = list->next )
	{
		// RB: support structural polygons instead of brushes but only for the worldspawn.
		// Building a full BSP tree for complex models made in Blender leads to visible cracks
		// so we only feed the triangles later into the empty BSP tree of the entity
		if( dmapGlobals.entityNum == 0 )
		{
			tri = list->polyTris;
			if( tri )
			{
				for( ; tri; tri = tri->next )
				{
					MapPolygonMesh* mapMesh = ( MapPolygonMesh* ) tri->originalMapMesh;

					// don't create BSP faces for the nodraw helpers touching the area portals
					if( mapMesh->IsAreaportal() && !( tri->material->GetContentFlags() & CONTENTS_AREAPORTAL ) )
					{
						continue;
					}

					f = AllocBspFace();

					if( tri->material->GetContentFlags() & CONTENTS_AREAPORTAL )
					{
						f->portal = true;
					}

					w = WindingForTri( tri );
					f->w = w;
					f->planenum = tri->planeNum & ~1;

					f->next = flist;
					flist = f;
				}

				continue;
			}
		}
		// RB end

		b = list->brush;
		if( !b )
		{
			continue;
		}

		if( !b->opaque && !( b->contents & CONTENTS_AREAPORTAL ) && !b->substractive )
		{
			continue;
		}

		for( i = 0; i < b->numsides; i++ )
		{
			s = &b->sides[i];
			w = s->winding;

			if( !w )
			{
				continue;
			}

			if( ( b->contents & CONTENTS_AREAPORTAL ) && !( s->material->GetContentFlags() & CONTENTS_AREAPORTAL ) )
			{
				continue;
			}

			f = AllocBspFace();

			if( s->material->GetContentFlags() & CONTENTS_AREAPORTAL )
			{
				f->portal = true;
			}

			f->simpleBSP = b->simpleBSP;

			if( b->substractive )
			{
				f->w = w->Reverse();
				f->planenum = ( s->planenum ^ 1 ) & ~1;

				//idPlane plane;
				//f->w->GetPlane( plane );
				//f->planenum = FindFloatPlane( plane );
			}
			else
			{
				f->w = w->Copy();
				f->planenum = s->planenum & ~1;
			}

			f->next = flist;
			flist = f;
		}
	}

	return flist;
}



