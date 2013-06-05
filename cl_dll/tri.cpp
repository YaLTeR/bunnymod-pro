//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

// Triangle rendering, if any
#include <vector>

#include "hud.h"
#include "cl_util.h"

// Triangle rendering apis are in gEngfuncs.pTriAPI

#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "triangleapi.h"

#include "ruler.h"
#include "com_model.h"
#include "studio_util.h"
#include "event_api.h"

#define DLLEXPORT __declspec( dllexport )

extern "C"
{
	void DLLEXPORT HUD_DrawNormalTriangles( void );
	void DLLEXPORT HUD_DrawTransparentTriangles( void );
};

/*void SetPoint( float x, float y, float z, float (*matrix)[4])
{
    vec3_t point, result;
    point[0] = x;
    point[1] = y;
    point[2] = z;
    
    VectorTransform(point, matrix, result);
    
    gEngfuncs.pTriAPI->Vertex3f(result[0], result[1], result[2]);
}*/

#define TEST_IT
#if defined( TEST_IT )

/*
=================
Draw_Triangles

Example routine.  Draws a sprite offset from the player origin.
=================
*/
void Draw_Triangles( void )
{
	cl_entity_t *player;
	vec3_t org;

	// Load it up with some bogus data
	player = gEngfuncs.GetLocalPlayer();
	if ( !player )
		return;

	org = player->origin;

	org.x += 50;
	org.y += 50;

	if (gHUD.m_hsprCursor == 0)
	{
		char sz[256];
		sprintf( sz, "sprites/hotglow.spr" );
		gHUD.m_hsprCursor = SPR_Load( sz );
	}

	/*if ( !gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *)gEngfuncs.GetSpritePointer( gHUD.m_hsprCursor ), 0 ))
	{
		return;
	}*/
	
	// Create a triangle, sigh
	gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
	gEngfuncs.pTriAPI->CullFace( TRI_NONE );
	gEngfuncs.pTriAPI->Begin( TRI_QUADS );
	// Overload p->color with index into tracer palette, p->packedColor with brightness
	gEngfuncs.pTriAPI->Color4f( 1.0, 0.0, 0.0, 1.0 );
	// UNDONE: This gouraud shading causes tracers to disappear on some cards (permedia2)
	gEngfuncs.pTriAPI->Brightness( 1 );
	//gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
	gEngfuncs.pTriAPI->Vertex3f( org.x, org.y, org.z );

	gEngfuncs.pTriAPI->Brightness( 1 );
	//gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
	gEngfuncs.pTriAPI->Vertex3f( org.x, org.y , org.z + 50 );

	gEngfuncs.pTriAPI->Brightness( 1 );
	//gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
	gEngfuncs.pTriAPI->Vertex3f( org.x, org.y + 50, org.z + 50 );

	gEngfuncs.pTriAPI->Brightness( 1 );
	//gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
	gEngfuncs.pTriAPI->Vertex3f( org.x, org.y + 50, org.z );

	gEngfuncs.pTriAPI->End();
	gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
}

#endif

/*
DrawCube
Renders a cube with given center and halfsize.
Arguments: vec3_t vecCenter - center of the cube; float flHalfsize - length of an edge / 2.
*/
void DrawCube( vec3_t vecCenter, float flHalfsize )
{
	vec3_t A, B, C, D, A1, B1, C1, D1;

	A[0] = vecCenter[0] - flHalfsize;
	B[0] = vecCenter[0] - flHalfsize;
	C[0] = vecCenter[0] + flHalfsize;
	D[0] = vecCenter[0] + flHalfsize;
	A1[0] = vecCenter[0] - flHalfsize;
	B1[0] = vecCenter[0] - flHalfsize;
	C1[0] = vecCenter[0] + flHalfsize;
	D1[0] = vecCenter[0] + flHalfsize;

	A[1] = vecCenter[1] + flHalfsize;
	B[1] = vecCenter[1] - flHalfsize;
	C[1] = vecCenter[1] - flHalfsize;
	D[1] = vecCenter[1] + flHalfsize;
	A1[1] = vecCenter[1] + flHalfsize;
	B1[1] = vecCenter[1] - flHalfsize;
	C1[1] = vecCenter[1] - flHalfsize;
	D1[1] = vecCenter[1] + flHalfsize;

	A[2] = vecCenter[2] - flHalfsize;
	B[2] = vecCenter[2] - flHalfsize;
	C[2] = vecCenter[2] - flHalfsize;
	D[2] = vecCenter[2] - flHalfsize;
	A1[2] = vecCenter[2] + flHalfsize;
	B1[2] = vecCenter[2] + flHalfsize;
	C1[2] = vecCenter[2] + flHalfsize;
	D1[2] = vecCenter[2] + flHalfsize;

	gEngfuncs.pTriAPI->Begin( TRI_QUADS );

		// Bottom
		gEngfuncs.pTriAPI->Vertex3fv( A );
		gEngfuncs.pTriAPI->Vertex3fv( B );
		gEngfuncs.pTriAPI->Vertex3fv( C );
		gEngfuncs.pTriAPI->Vertex3fv( D );

		// Top
		gEngfuncs.pTriAPI->Vertex3fv( D1 );
		gEngfuncs.pTriAPI->Vertex3fv( C1 );
		gEngfuncs.pTriAPI->Vertex3fv( B1 );
		gEngfuncs.pTriAPI->Vertex3fv( A1 );

		// Front
		gEngfuncs.pTriAPI->Vertex3fv( D );
		gEngfuncs.pTriAPI->Vertex3fv( D1 );
		gEngfuncs.pTriAPI->Vertex3fv( A1 );
		gEngfuncs.pTriAPI->Vertex3fv( A );

		// Back
		gEngfuncs.pTriAPI->Vertex3fv( B );
		gEngfuncs.pTriAPI->Vertex3fv( B1 );
		gEngfuncs.pTriAPI->Vertex3fv( C1 );
		gEngfuncs.pTriAPI->Vertex3fv( C );

		// Right
		gEngfuncs.pTriAPI->Vertex3fv( C );
		gEngfuncs.pTriAPI->Vertex3fv( C1 );
		gEngfuncs.pTriAPI->Vertex3fv( D1 );
		gEngfuncs.pTriAPI->Vertex3fv( D );

		// Left
		gEngfuncs.pTriAPI->Vertex3fv( A );
		gEngfuncs.pTriAPI->Vertex3fv( A1 );
		gEngfuncs.pTriAPI->Vertex3fv( B1 );
		gEngfuncs.pTriAPI->Vertex3fv( B );

	gEngfuncs.pTriAPI->End();
}

/*
DrawSphere
Renders a sphere with given vertex array, ring and sector counts.
Arguments: std::vector<float> vertices - the vertex array; int iRings - the amount of rings; int iSectors - the amount of sectors.
*/
void DrawSphere( std::vector<float> vertices, int iRings, int iSectors )
{
	float const R = 1.0 / ( float ) ( iRings - 1 );
	float const S = 1.0 / ( float ) ( iSectors - 1 );
	int r, s;

	gEngfuncs.pTriAPI->Begin( TRI_LINES );

		for ( r = 0; r < ( iRings - 1 ); r++ )
		{
			for ( s = 0; s < ( iSectors - 1 ); s++ )
			{
				gEngfuncs.pTriAPI->Vertex3f( vertices[3 * ( r * iSectors + s )], vertices[3 * ( r * iSectors + s ) + 1], vertices[3 * ( r * iSectors + s ) + 2] );
				gEngfuncs.pTriAPI->Vertex3f( vertices[3 * ( r * iSectors + ( s + 1 ) )], vertices[3 * ( r * iSectors + ( s + 1 ) ) + 1], vertices[3 * ( r * iSectors + ( s + 1 ) ) + 2] );
				gEngfuncs.pTriAPI->Vertex3f( vertices[3 * ( r * iSectors + ( s + 1 ) )], vertices[3 * ( r * iSectors + ( s + 1 ) ) + 1], vertices[3 * ( r * iSectors + ( s + 1 ) ) + 2] );
				gEngfuncs.pTriAPI->Vertex3f( vertices[3 * ( ( r + 1 ) * iSectors + ( s + 1 ) )], vertices[3 * ( ( r + 1 ) * iSectors + ( s + 1 ) ) + 1], vertices[3 * ( ( r + 1 ) * iSectors + ( s + 1 ) ) + 2] );
				gEngfuncs.pTriAPI->Vertex3f( vertices[3 * ( ( r + 1 ) * iSectors + ( s + 1 ) )], vertices[3 * ( ( r + 1 ) * iSectors + ( s + 1 ) ) + 1], vertices[3 * ( ( r + 1 ) * iSectors + ( s + 1 ) ) + 2] );
				gEngfuncs.pTriAPI->Vertex3f( vertices[3 * ( ( r + 1 ) * iSectors + s )], vertices[3 * ( ( r + 1 ) * iSectors + s ) + 1], vertices[3 * ( ( r + 1 ) * iSectors + s ) + 2] );
				gEngfuncs.pTriAPI->Vertex3f( vertices[3 * ( ( r + 1 ) * iSectors + s )], vertices[3 * ( ( r + 1 ) * iSectors + s ) + 1], vertices[3 * ( ( r + 1 ) * iSectors + s ) + 2] );
				gEngfuncs.pTriAPI->Vertex3f( vertices[3 * ( r * iSectors + s )], vertices[3 * ( r * iSectors + s ) + 1], vertices[3 * ( r * iSectors + s ) + 2] );
			}
		}

	gEngfuncs.pTriAPI->End();
}

/*
DrawRulerPoints
Draws sprites where the ruler points are.
*/
extern cl_rulerPoint firstRulerPoint;
extern cl_rulerPoint *autostopsavePoint;
extern cl_sphere *autostopsaveSphere;
extern cvar_t *cl_ruler_render;
extern std::vector<float> autostopsaveSphereVertices;

void DrawRulerPoints( void )
{
	if ( cl_ruler_render->value )
	{
		gEngfuncs.pTriAPI->CullFace( TRI_NONE );
		gEngfuncs.pTriAPI->RenderMode( kRenderTransColor );

		gEngfuncs.pTriAPI->Color4f( 1.0, 1.0, 0.0, 1.0 );

		cl_rulerPoint *curPoint = firstRulerPoint.pNext;

		while ( curPoint != NULL )
		{
			DrawCube( curPoint->origin, 8 );
			curPoint = curPoint->pNext;
		}

		if ( autostopsavePoint != NULL )
		{
			gEngfuncs.pTriAPI->Color4f( 1.0, 0.0, 0.0, 1.0 );

			DrawCube( autostopsavePoint->origin, 16 );

			if ( autostopsaveSphere != NULL )
			{
				DrawSphere( autostopsaveSphereVertices, autostopsaveSphere->iRings, autostopsaveSphere->iSectors );
			}
		}
	}
}


/*
RenderAutocmdPlane
Renders the autocmd plane.
*/
extern cvar_t *cl_autocmd_render;
extern cvar_t *cl_autocmd_plane, *cl_autocmd_coord;

void RenderAutocmdPlane( void )
{
	if ( cl_autocmd_render->value && ( strlen( cl_autocmd_plane->string ) != 0 ) && ( strlen( cl_autocmd_coord->string ) != 0 ) )
	{
		float coord = cl_autocmd_coord->value;
		char plane;

		sscanf( cl_autocmd_plane->string, "%c", &plane );

		gEngfuncs.pTriAPI->CullFace( TRI_NONE );
		gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );
		gEngfuncs.pTriAPI->Color4f( 1.0, 0.0, 0.0, 0.5 );
		gEngfuncs.pTriAPI->Begin( TRI_QUADS );

		switch ( plane )
		{
			case 'X':
			case 'x':
				gEngfuncs.pTriAPI->Vertex3f( coord, -50000.0, -50000.0 );
				gEngfuncs.pTriAPI->Vertex3f( coord, -50000.0, 50000.0 );
				gEngfuncs.pTriAPI->Vertex3f( coord, 50000.0, 50000.0 );
				gEngfuncs.pTriAPI->Vertex3f( coord, 50000.0, -50000.0 );

				break;

			case 'Y':
			case 'y':
				gEngfuncs.pTriAPI->Vertex3f( -50000.0, coord, -50000.0 );
				gEngfuncs.pTriAPI->Vertex3f( -50000.0, coord, 50000.0 );
				gEngfuncs.pTriAPI->Vertex3f( 50000.0, coord, 50000.0 );
				gEngfuncs.pTriAPI->Vertex3f( 50000.0, coord, -50000.0 );

				break;

			case 'Z':
			case 'z':
				gEngfuncs.pTriAPI->Vertex3f( -50000.0, -50000.0, coord );
				gEngfuncs.pTriAPI->Vertex3f( -50000.0, 50000.0, coord );
				gEngfuncs.pTriAPI->Vertex3f( 50000.0, 50000.0, coord );
				gEngfuncs.pTriAPI->Vertex3f( 50000.0, -50000.0, coord );

				break;

			default:
				;
		}

		gEngfuncs.pTriAPI->End();
	}
}

/*
=================
HUD_DrawNormalTriangles

Non-transparent triangles-- add them here
=================
*/
void DLLEXPORT HUD_DrawNormalTriangles( void )
{

	gHUD.m_Spectator.DrawOverview();

	DrawRulerPoints();
	
#if defined( TEST_IT )
	// Draw_Triangles();
#endif
}

/*
=================
HUD_DrawTransparentTriangles

Render any triangles with transparent rendermode needs here
=================
*/
void DLLEXPORT HUD_DrawTransparentTriangles( void )
{
	DrawRulerPoints();
	RenderAutocmdPlane();

#if defined( TEST_IT )
	// Draw_Triangles();
#endif
}