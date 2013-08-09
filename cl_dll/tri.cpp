//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

// Triangle rendering, if any
#include <vector>

#include <windows.h>
#include <gl/gl.h>

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
    
    glVertex3f(result[0], result[1], result[2]);
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
	glBegin( GL_QUADS );
	// Overload p->color with index into tracer palette, p->packedColor with brightness
	gEngfuncs.pTriAPI->Color4f( 1.0, 0.0, 0.0, 1.0 );
	// UNDONE: This gouraud shading causes tracers to disappear on some cards (permedia2)
	gEngfuncs.pTriAPI->Brightness( 1 );
	//gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
	glVertex3f( org.x, org.y, org.z );

	gEngfuncs.pTriAPI->Brightness( 1 );
	//gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
	glVertex3f( org.x, org.y , org.z + 50 );

	gEngfuncs.pTriAPI->Brightness( 1 );
	//gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
	glVertex3f( org.x, org.y + 50, org.z + 50 );

	gEngfuncs.pTriAPI->Brightness( 1 );
	//gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
	glVertex3f( org.x, org.y + 50, org.z );

	glEnd();
	gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
}

#endif

/*
DrawCube
Renders a cube with given center and halfsize.
Arguments: vec3_t vecCenter - center of the cube; float flHalfsize - length of an edge / 2; bool wireframe - draw the cube wireframe if set to true.
*/
void DrawCube( vec3_t vecCenter, float flHalfsize, bool wireframe )
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

	if ( !wireframe )
	{

		glBegin( GL_QUADS );

			// Bottom
			glVertex3fv( A );
			glVertex3fv( B );
			glVertex3fv( C );
			glVertex3fv( D );

			// Top
			glVertex3fv( D1 );
			glVertex3fv( C1 );
			glVertex3fv( B1 );
			glVertex3fv( A1 );

			// Front
			glVertex3fv( D );
			glVertex3fv( D1 );
			glVertex3fv( A1 );
			glVertex3fv( A );

			// Back
			glVertex3fv( B );
			glVertex3fv( B1 );
			glVertex3fv( C1 );
			glVertex3fv( C );

			// Right
			glVertex3fv( C );
			glVertex3fv( C1 );
			glVertex3fv( D1 );
			glVertex3fv( D );

			// Left
			glVertex3fv( A );
			glVertex3fv( A1 );
			glVertex3fv( B1 );
			glVertex3fv( B );

		glEnd();

	}
	else
	{

		glBegin( GL_LINES );

			// Bottom
			glVertex3fv( A );
			glVertex3fv( B );
			glVertex3fv( B );
			glVertex3fv( C );
			glVertex3fv( C );
			glVertex3fv( D );
			glVertex3fv( D );
			glVertex3fv( A );

			// Top
			glVertex3fv( D1 );
			glVertex3fv( C1 );
			glVertex3fv( C1 );
			glVertex3fv( B1 );
			glVertex3fv( B1 );
			glVertex3fv( A1 );
			glVertex3fv( A1 );
			glVertex3fv( D1 );

			// Front
			glVertex3fv( D );
			glVertex3fv( D1 );
			glVertex3fv( D1 );
			glVertex3fv( A1 );
			glVertex3fv( A1 );
			glVertex3fv( A );
			glVertex3fv( A );
			glVertex3fv( D );

			// Back
			glVertex3fv( B );
			glVertex3fv( B1 );
			glVertex3fv( B1 );
			glVertex3fv( C1 );
			glVertex3fv( C1 );
			glVertex3fv( C );
			glVertex3fv( C );
			glVertex3fv( B );

			// Right
			glVertex3fv( C );
			glVertex3fv( C1 );
			glVertex3fv( C1 );
			glVertex3fv( D1 );
			glVertex3fv( D1 );
			glVertex3fv( D );
			glVertex3fv( D );
			glVertex3fv( C );

			// Left
			glVertex3fv( A );
			glVertex3fv( A1 );
			glVertex3fv( A1 );
			glVertex3fv( B1 );
			glVertex3fv( B1 );
			glVertex3fv( B );
			glVertex3fv( B );
			glVertex3fv( A );

		glEnd();

	}
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

	glBegin( GL_LINES );

		for ( r = 0; r < ( iRings - 1 ); r++ )
		{
			for ( s = 0; s < ( iSectors - 1 ); s++ )
			{
				glVertex3f( vertices[3 * ( r * iSectors + s )], vertices[3 * ( r * iSectors + s ) + 1], vertices[3 * ( r * iSectors + s ) + 2] );
				glVertex3f( vertices[3 * ( r * iSectors + ( s + 1 ) )], vertices[3 * ( r * iSectors + ( s + 1 ) ) + 1], vertices[3 * ( r * iSectors + ( s + 1 ) ) + 2] );
				glVertex3f( vertices[3 * ( r * iSectors + ( s + 1 ) )], vertices[3 * ( r * iSectors + ( s + 1 ) ) + 1], vertices[3 * ( r * iSectors + ( s + 1 ) ) + 2] );
				glVertex3f( vertices[3 * ( ( r + 1 ) * iSectors + ( s + 1 ) )], vertices[3 * ( ( r + 1 ) * iSectors + ( s + 1 ) ) + 1], vertices[3 * ( ( r + 1 ) * iSectors + ( s + 1 ) ) + 2] );
				glVertex3f( vertices[3 * ( ( r + 1 ) * iSectors + ( s + 1 ) )], vertices[3 * ( ( r + 1 ) * iSectors + ( s + 1 ) ) + 1], vertices[3 * ( ( r + 1 ) * iSectors + ( s + 1 ) ) + 2] );
				glVertex3f( vertices[3 * ( ( r + 1 ) * iSectors + s )], vertices[3 * ( ( r + 1 ) * iSectors + s ) + 1], vertices[3 * ( ( r + 1 ) * iSectors + s ) + 2] );
				glVertex3f( vertices[3 * ( ( r + 1 ) * iSectors + s )], vertices[3 * ( ( r + 1 ) * iSectors + s ) + 1], vertices[3 * ( ( r + 1 ) * iSectors + s ) + 2] );
				glVertex3f( vertices[3 * ( r * iSectors + s )], vertices[3 * ( r * iSectors + s ) + 1], vertices[3 * ( r * iSectors + s ) + 2] );
			}
		}

	glEnd();
}

/*
DrawLine
Renders a line between two points with given origins.
Arguments: vec3_t vecPoint1 - origin of the first point; vec3_t vecPoint2 - origin of the second point.
*/
void DrawLine( vec3_t vecPoint1, vec3_t vecPoint2 )
{
	glBegin( GL_LINES );

		glVertex3fv( vecPoint1 );
		glVertex3fv( vecPoint2 );

	glEnd();
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
		glDisable( GL_TEXTURE_2D );

		glColor4f( 1.0, 1.0, 0.0, 1.0 );

		gEngfuncs.pTriAPI->RenderMode( kRenderTransColor );

		cl_rulerPoint *curPoint = firstRulerPoint.pNext;

		while ( curPoint != NULL )
		{
			DrawCube( curPoint->origin, 8, false );
			curPoint = curPoint->pNext;
		}

		if ( autostopsavePoint != NULL )
		{
			gEngfuncs.pTriAPI->Color4f( 1.0, 0.0, 0.0, 1.0 );

			DrawCube( autostopsavePoint->origin, 16, false );

			if ( autostopsaveSphere != NULL )
			{
				DrawSphere( autostopsaveSphereVertices, autostopsaveSphere->iRings, autostopsaveSphere->iSectors );
			}
		}

		glEnable( GL_TEXTURE_2D );
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

		glDisable( GL_TEXTURE_2D );
		glDisable( GL_CULL_FACE );
		// glColor4f( 1.0, 0.0, 0.0, 0.1 );

		gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );
		gEngfuncs.pTriAPI->Color4f( 1.0, 0.0, 0.0, 0.5 );

		glBegin( GL_QUADS );

		switch ( plane )
		{
			case 'X':
			case 'x':
				glVertex3f( coord, -50000.0, -50000.0 );
				glVertex3f( coord, -50000.0, 50000.0 );
				glVertex3f( coord, 50000.0, 50000.0 );
				glVertex3f( coord, 50000.0, -50000.0 );

				break;

			case 'Y':
			case 'y':
				glVertex3f( -50000.0, coord, -50000.0 );
				glVertex3f( -50000.0, coord, 50000.0 );
				glVertex3f( 50000.0, coord, 50000.0 );
				glVertex3f( 50000.0, coord, -50000.0 );

				break;

			case 'Z':
			case 'z':
				glVertex3f( -50000.0, -50000.0, coord );
				glVertex3f( -50000.0, 50000.0, coord );
				glVertex3f( 50000.0, 50000.0, coord );
				glVertex3f( 50000.0, -50000.0, coord );

				break;

			default:
				;
		}

		glEnd();

		glEnable( GL_CULL_FACE );
		glEnable( GL_TEXTURE_2D );
	}
}

/*
RenderSpawns
Renders spawn points.
*/
extern cvar_t *cl_spawns_render, *cl_spawns_wireframe;
extern cvar_t *cl_spawns_drawcross;
extern cvar_t *cl_spawns_alpha;
extern std::vector<vec3_t> spawns;
extern std::vector<vec3_t*> spawnCrossPoints;

void RenderSpawns( void )
{
	if ( cl_spawns_render->value == 1 && cl_spawns_alpha->value > 0 )
	{
		glDisable( GL_TEXTURE_2D );

		if ( cl_spawns_alpha->value == 1.0 )
		{
			gEngfuncs.pTriAPI->RenderMode( kRenderTransColor );
			glColor3ub( 0, 255, 0 );
		}
		else
		{
			gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );
			gEngfuncs.pTriAPI->Color4f( 0.0, 1.0, 0.0, cl_spawns_alpha->value );
		}

		if ( spawns.size() != 0 )
		{
			for ( std::vector<vec3_t>::iterator it = spawns.begin(); it < spawns.end(); ++it )
			{
				DrawCube( *it, 10, cl_spawns_wireframe->value == 1 );
			}

			if ( cl_spawns_drawcross->value == 1 )
			{
				for ( std::vector<vec3_t*>::iterator it1 = spawnCrossPoints.begin(); it1 < spawnCrossPoints.end(); ++it1 )
				{
					if ( *it1 != NULL )
					{
						DrawLine( ( *it1 )[0], ( *it1 )[3] );
						DrawLine( ( *it1 )[1], ( *it1 )[4] );
						DrawLine( ( *it1 )[2], ( *it1 )[5] );
					}
				}
			}
		}

		glEnable( GL_TEXTURE_2D );
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
	// DrawRulerPoints();
	RenderAutocmdPlane();

	DrawRulerPoints();
	RenderSpawns();

#if defined( TEST_IT )
	// Draw_Triangles();
#endif
}