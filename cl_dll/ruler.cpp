#include <vector>
#include <memory.h>
#include "hud.h"
#include "com_model.h"
#include "cl_util.h"
#include "studio_util.h"
#include "const.h"
#include "cdll_int.h"
#include "pm_defs.h"
#include "eventscripts.h"
#include "event_api.h"
#include "r_efx.h"

#include "ruler.h"

extern vec3_t g_org;
extern vec3_t g_vecViewAngle;
extern cvar_t *cl_ruler_enable;
extern cvar_t *cl_ruler_autodelay;
extern cvar_t *cl_ruler_addorigin;
extern cvar_t *cl_autostopsave_radius;
extern cvar_t *cl_autostopsave_cmd;
extern cvar_t *cl_autocmd_enable;
extern cvar_t *cl_autocmd_plane, *cl_autocmd_coord, *cl_autocmd_distance;
extern cvar_t *cl_autocmd_cmd;

extern "C" int g_iAutoJump;

cl_rulerPoint firstRulerPoint;

cl_rulerPoint *autostopsavePoint = NULL;
cl_sphere *autostopsaveSphere = NULL;
std::vector<float> autostopsaveSphereVertices;

bool autostopsaveCmdExecuted = false;
bool autocmdCmdExecuted = false;

double flRulerOldTime = 0.0, flRulerTime, flRulerTimeDelta;

std::vector<vec3_t> spawns;
std::vector<vec3_t*> spawnCrossPoints;

/*
ResetRuler
Resets ruler points and cleans the memory.
*/
void ResetRuler( void )
{
	cl_rulerPoint *delRulerPoint = firstRulerPoint.pNext;
	firstRulerPoint.pNext = NULL;

	while ( delRulerPoint != NULL )
	{
		if ( delRulerPoint->beamToThisPoint != NULL )
		{
			delRulerPoint->beamToThisPoint->flags &= ~FBEAM_FOREVER;
		}

		cl_rulerPoint *nextRulerPoint = delRulerPoint->pNext;
		delete delRulerPoint;
		delRulerPoint = nextRulerPoint;
	}

	InitRuler();
	return;
}

/*
InitRuler
Initializes the variables with zero values.
*/
void InitRuler( void )
{
	firstRulerPoint.origin[0] = 0;
	firstRulerPoint.origin[1] = 0;
	firstRulerPoint.origin[2] = 0;
	firstRulerPoint.beamToThisPoint = NULL;
	firstRulerPoint.pNext = NULL;
	firstRulerPoint.pPrev = NULL;

	return;
}

/*
PrintDistance
Prints the distance between the two points into the console.
*/
void PrintDistance( void )
{
	vec3_t rulerDelta;
	double flDistanceXYZ = 0.0, flDistanceXY = 0.0;
	cl_rulerPoint *curPoint = firstRulerPoint.pNext;

	while ( curPoint != NULL )
	{
		cl_rulerPoint *nextPoint = curPoint->pNext;
		if ( nextPoint == NULL ) break;

		VectorSubtract( nextPoint->origin, curPoint->origin, rulerDelta );
		flDistanceXY += sqrt( rulerDelta[0] * rulerDelta[0] + rulerDelta[1] * rulerDelta[1] );
		flDistanceXYZ += sqrt( rulerDelta[0] * rulerDelta[0] + rulerDelta[1] * rulerDelta[1] + rulerDelta[2] * rulerDelta[2] );

		curPoint = nextPoint;
	}

	gEngfuncs.Con_Printf("Distance (XY): %.15f\nDistance (XYZ): %.15f\n", flDistanceXY, flDistanceXYZ);

	return;
}

/*
AimPos
Stores the position of the point the player is aiming at.
*/
void AimPos( void )
{
	vec3_t origin;
	vec3_t angles;
	vec3_t vecStart, vecEnd;
	vec3_t forward, right, up;

	VectorCopy( g_org, origin );
	VectorCopy( g_vecViewAngle, angles );

	AngleVectors( angles, forward, right, up );

	vec3_t view_ofs;
	VectorClear( view_ofs );
	view_ofs[2] = DEFAULT_VIEWHEIGHT;
	gEngfuncs.pEventAPI->EV_LocalPlayerViewheight( view_ofs );
	VectorAdd( origin, view_ofs, vecStart );

	VectorMA( vecStart, 8192, forward, vecEnd );

	pmtrace_t tr;
	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
	gEngfuncs.pEventAPI->EV_PlayerTrace( vecStart, vecEnd, PM_STUDIO_BOX, -1, &tr );// 1st, this can hit players

	if ( tr.fraction < 1.0 )
	{
		StorePoint( tr.endpos );
	}
}

/*
PlayerPos
Stores the player position.
*/
void PlayerPos( void )
{
	StorePoint( g_org );
}

/*
ViewPos
Stores the view position.
*/
void ViewPos( void )
{
	vec3_t origin;
	vec3_t out;

	VectorCopy( g_org, origin );

	vec3_t view_ofs;
	VectorClear( view_ofs );
	view_ofs[2] = DEFAULT_VIEWHEIGHT;
	gEngfuncs.pEventAPI->EV_LocalPlayerViewheight( view_ofs );
	VectorAdd( origin, view_ofs, out );

	StorePoint( out );
}

/*
RulerAutoFunc
This function is called automatically each frame to add player position as a point each <cl_ruler_autodelay> seconds.
Also this function is used to add the point by its origin (cl_ruler_addorigin).
And also updates autosavepointSphere radius if it doesn't match the one set in cl_autostopsave_radius.
Arguments: double flTime - current time.
*/
void RulerAutoFunc( double flTime )
{
	flRulerOldTime = flRulerTime;
	flRulerTime = flTime;
	flRulerTimeDelta = flRulerTime - flRulerOldTime;

	if ( flRulerTimeDelta < 0 )
	{
		flRulerTimeDelta = 0;
		flRulerOldTime = flRulerTime;
	}

	if ( ( cl_ruler_autodelay->value > 0 ) && ( flRulerTimeDelta > cl_ruler_autodelay->value ) )
	{
		PlayerPos();
	}
	else
	{
		flRulerTime = flRulerOldTime;
	}

	/*if ( strlen( cl_ruler_addorigin->string ) > 0 )
	{
		float x = 0.0, y = 0.0, z = 0.0;

		sscanf( cl_ruler_addorigin->string, "%f %f %f", &x, &y, &z );

		vec3_t vecPoint;
		vecPoint[0] = x;
		vecPoint[1] = y;
		vecPoint[2] = z;

		StorePoint( vecPoint );

		gEngfuncs.pfnClientCmd( "cl_ruler_addorigin \"\"\n" );
	}*/

	if ( autostopsaveSphere != NULL )
	{
		int iSphereRadius = cl_autostopsave_radius->value;
		if ( iSphereRadius <= 0 )
		{
			iSphereRadius = 50;
		}

		if ( autostopsaveSphere->iRadius != iSphereRadius )
		{
			autostopsaveSphere->iRadius = iSphereRadius;
			CalcSphereVertices( autostopsaveSphere );
		}
	}

	g_iAutoJump = CVAR_GET_FLOAT( "sv_autojump" );
}

/*
StorePos
Stores the position of the point into memory.
Arguments: vec3_t vecPoint - coordinates of a point.
*/
void StorePoint( vec3_t vecPoint )
{
	if ( !cl_ruler_enable->value ) return;

	cl_rulerPoint *newPoint = new cl_rulerPoint;

	if ( !newPoint )
	{
		gEngfuncs.Con_Printf( "Error: Failed to allocate memory for a new ruler point!\n" );
		return;
	}

	VectorCopy( vecPoint, newPoint->origin );

	cl_rulerPoint *curPoint = firstRulerPoint.pNext;

	newPoint->pNext = curPoint;
	newPoint->pPrev = &firstRulerPoint;

	if ( curPoint != NULL )
	{
		curPoint->pPrev = newPoint;

		int m_iBeam = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/smoke.spr" );
		newPoint->beamToThisPoint = gEngfuncs.pEfxAPI->R_BeamPoints( curPoint->origin, newPoint->origin, m_iBeam, 0, 1.0, 0, 256.0, 0, 0, 0, 0, 255, 0 );
	}

	firstRulerPoint.pNext = newPoint;

	PrintDistance();
}

/*
DeleteLast
Deletes the last added point from memory.
*/
void DeleteLast( void )
{
	cl_rulerPoint *delPoint = firstRulerPoint.pNext;

	if ( delPoint != NULL )
	{
		cl_rulerPoint *nextPoint = delPoint->pNext;

		if ( nextPoint != NULL )
		{
			nextPoint->pPrev = &firstRulerPoint;
		}

		firstRulerPoint.pNext = nextPoint;

		if ( delPoint->beamToThisPoint != NULL )
		{
			delPoint->beamToThisPoint->flags &= ~FBEAM_FOREVER;
		}

		delete delPoint;
	}
}

/*
AddOrigin
Adds a ruler point by given through cmd arguments origin.
*/
void AddOrigin( void )
{
	if ( gEngfuncs.Cmd_Argc() != 4 )
	{
		gEngfuncs.Con_Printf( "Usage: cl_ruler_addorigin <x> <y> <z>\n" );
		return;
	}

	float x = 0, y = 0, z = 0;
	sscanf( gEngfuncs.Cmd_Argv( 1 ), "%f", &x );
	sscanf( gEngfuncs.Cmd_Argv( 2 ), "%f", &y );
	sscanf( gEngfuncs.Cmd_Argv( 3 ), "%f", &z );

	gEngfuncs.Con_Printf( "x, y, z: %f, %f, %f\n", x, y, z );

	vec3_t vecPoint;
	vecPoint[0] = x;
	vecPoint[1] = y;
	vecPoint[2] = z;

	StorePoint( vecPoint );
}

/*
AutostopsaveAddPoint
Converts the last added point to an autostopsave point.
*/
void AutostopsaveAddPoint( void )
{
	if ( autostopsavePoint == NULL )
	{
		cl_rulerPoint *lastPoint = firstRulerPoint.pNext;

		if ( lastPoint != NULL )
		{
			cl_rulerPoint *nextPoint = lastPoint->pNext;

			if ( nextPoint != NULL )
			{
				nextPoint->pPrev = &firstRulerPoint;
			}

			firstRulerPoint.pNext = nextPoint;

			if ( lastPoint->beamToThisPoint != NULL )
			{
				lastPoint->beamToThisPoint->flags &= ~FBEAM_FOREVER;
			}

			autostopsavePoint = lastPoint;

			int iSphereRadius = cl_autostopsave_radius->value;
			if ( iSphereRadius <= 0 )
			{
				iSphereRadius = 50;
			}

			autostopsaveSphere = new cl_sphere;

			if ( !autostopsaveSphere )
			{
				gEngfuncs.Con_Printf( "cl_autostopsave_addpoint error: Failed to allocate memory for a new ruler point!\n" );
				return;
			}

			gEngfuncs.Con_Printf( "x, y, z: %f, %f, %f\n", autostopsavePoint->origin[0], autostopsavePoint->origin[1], autostopsavePoint->origin[2] );

			VectorCopy( autostopsavePoint->origin, autostopsaveSphere->vecOrigin );
			autostopsaveSphere->iRadius = iSphereRadius;
			autostopsaveSphere->iRings = 20;
			autostopsaveSphere->iSectors = 20;

			CalcSphereVertices( autostopsaveSphere );

			PrintDistance();
		}
		else
		{
			gEngfuncs.Con_Printf( "cl_autostopsave_addpoint error: you need to add a ruler point first!\n" );
		}
	}
	else
	{
		gEngfuncs.Con_Printf( "cl_autostopsave_addpoint error: you can't add more than one autostopsave point, please delete the existing one first!\n" );
	}
}

/*
AutostopsaveDeletePoint
Deletes the autostopsave point.
*/
void AutostopsaveDeletePoint( void )
{
	if ( autostopsavePoint != NULL )
	{
		delete autostopsavePoint;

		autostopsavePoint = NULL;

		if ( autostopsaveSphere != NULL )
		{
			delete autostopsaveSphere;

			autostopsaveSphere = NULL;
		}
	}
	else
	{
		gEngfuncs.Con_Printf( "cl_autostopsave_delpoint error: no points to delete!\n" );
	}
}

/*
AutostopsaveAutoFunc
This function is called automatically each time the position is updated. This function checks if
the player is in the radius for the autostopsave point and executes console commands if so. Also
checks if the player is close enough to the autocmd plane and executes console commands if so.
Arguments: vec3_t vecOrigin - player position.
*/
void AutostopsaveAutoFunc( vec3_t vecOrigin )
{
	if ( autostopsavePoint != NULL )
	{
		int iSphereRadius = cl_autostopsave_radius->value;
		if ( iSphereRadius <= 0 )
		{
			iSphereRadius = 50;
		}

		vec3_t vecDelta;

		VectorSubtract( autostopsavePoint->origin, vecOrigin, vecDelta );

		float flDistance = sqrt( vecDelta[0] * vecDelta[0] + vecDelta[1] * vecDelta[1] + vecDelta[2] * vecDelta[2] );

		if ( flDistance <= iSphereRadius )
		{
			if ( !autostopsaveCmdExecuted )
			{
				autostopsaveCmdExecuted = true;
				gEngfuncs.pfnClientCmd( cl_autostopsave_cmd->string );
			}
		}
		else
		{
			autostopsaveCmdExecuted = false;
		}
	}

	if ( cl_autocmd_enable->value
			&& ( strlen( cl_autocmd_plane->string ) != 0 )
			&& ( strlen( cl_autocmd_coord->string ) != 0 ) 
			&& ( strlen( cl_autocmd_distance->string) != 0 ) )
	{
		float coord = cl_autocmd_coord->value;
		float distance = cl_autocmd_distance->value;
		char plane;

		sscanf( cl_autocmd_plane->string, "%c", &plane );

		switch ( plane )
		{
			case 'X':
			case 'x':
				if ( abs( coord - g_org[0] ) <= distance )
				{
					if ( !autocmdCmdExecuted )
					{
						autocmdCmdExecuted = true;
						gEngfuncs.pfnClientCmd( cl_autocmd_cmd->string );
					}
				}
				else
				{
					autocmdCmdExecuted = false;
				}

				break;

			case 'Y':
			case 'y':
				if ( abs( coord - g_org[1] ) <= distance )
				{
					if ( !autocmdCmdExecuted )
					{
						autocmdCmdExecuted = true;
						gEngfuncs.pfnClientCmd( cl_autocmd_cmd->string );
					}
				}
				else
				{
					autocmdCmdExecuted = false;
				}

				break;

			case 'Z':
			case 'z':
				if ( abs( coord - g_org[2] ) <= distance )
				{
					if ( !autocmdCmdExecuted )
					{
						autocmdCmdExecuted = true;
						gEngfuncs.pfnClientCmd( cl_autocmd_cmd->string );
					}
				}
				else
				{
					autocmdCmdExecuted = false;
				}

				break;

			default:
				;
		}
	}
}

/*
AutostopsavePrintOrigin
Prints the origin of the autostopsave point into the console.
*/
void AutostopsavePrintOrigin( void )
{
	if ( autostopsavePoint != NULL )
	{
		vec3_t vecOrigin;
		VectorCopy( autostopsavePoint->origin, vecOrigin );

		gEngfuncs.Con_Printf( "autostopsave point origin (x, y, z): %f, %f, %f\n", vecOrigin[0], vecOrigin[1], vecOrigin[2] );
	}
	else
	{
		gEngfuncs.Con_Printf( "cl_autostopsave_printorigin error: no autostopsave point found!\n" );
	}
}

/*
CalcSphereVertices
Calculates vertices for a given sphere and stores them so that they can be rendered.
Arguments: cl_sphere *sphere - pointer to the sphere.
*/
void CalcSphereVertices( cl_sphere *sphere )
{
	float const R = 1.0 / ( float ) ( sphere->iRings - 1 );
	float const S = 1.0 / ( float ) ( sphere->iSectors - 1 );
	int r, s;

	autostopsaveSphereVertices.resize( sphere->iRings * sphere->iSectors * 3 );

	std::vector<float>::iterator v = autostopsaveSphereVertices.begin();

	for ( r = 0; r < sphere->iRings; r++ )
	{
		for ( s = 0; s < sphere->iSectors; s++ )
		{
			float const y = sin( -( M_PI / 2 ) + M_PI * r * R );
			float const x = cos( 2 * M_PI * s * S ) * sin( M_PI * r * R );
			float const z = sin( 2 * M_PI * s * S ) * sin( M_PI * r * R );

			*v++ = ( x * sphere->iRadius ) + sphere->vecOrigin[0];
			*v++ = ( y * sphere->iRadius ) + sphere->vecOrigin[1];
			*v++ = ( z * sphere->iRadius ) + sphere->vecOrigin[2];
		}
	}
}

/*
FindSpawnsInMap
Finds every info_player_deathmatch on the map and stores its coordinates so that they can be rendered.
*/
void FindSpawnsInMap( void )
{
	if ( FindEntitiesInMap( "info_player_deathmatch", spawns ) == 0 )
	{
		gEngfuncs.Con_Printf( "Something went wrong in FindEntitiesInMap.\n" );
	}
	else
	{
		gEngfuncs.Con_Printf( "Found %d info_player_deathmatch entities!\n", spawns.size() );

		for ( std::vector<vec3_t*>::iterator it = spawnCrossPoints.begin(); it < spawnCrossPoints.end(); ++it )
		{
			delete [] *it;
		}

		spawnCrossPoints.clear();

		for ( std::vector<vec3_t>::iterator it1 = spawns.begin(); it1 < spawns.end(); ++it1 )
		{
			vec3_t *crossPoints = new vec3_t[5];
			CalculateCrossPoints( *it1, crossPoints );
			spawnCrossPoints.push_back( crossPoints );
		}
	}
}

/*
FindEntitiesInMap
Finds the origin of each entity in map of the specified classname.
Arguments: char *name - classname; std::vector<vec3_t> &origins - output vector.
*/
int FindEntitiesInMap( char *name, std::vector<vec3_t> &origins )
{
	int				n,found = 0;
	char			keyname[256];
	char			token[1024];
	vec3_t			origin;
	bool			gotorigin = false;

	cl_entity_t *	pEnt = gEngfuncs.GetEntityByIndex( 0 );	// get world model

	if ( !pEnt ) return 0;

	if ( !pEnt->model )	return 0;

	char * data = pEnt->model->entities;

	origins.clear();

	while (data)
	{
		data = gEngfuncs.COM_ParseFile(data, token);
		
		if ( (token[0] == '}') ||  (token[0]==0) )
			break;

		if (!data)
		{
			gEngfuncs.Con_DPrintf("FindEntitiesInMap: EOF without closing brace\n");
			return 0;
		}

		if (token[0] != '{')
		{
			gEngfuncs.Con_DPrintf("FindEntitiesInMap: Expected {\n");
			return 0;
		}

		// we parse the first { now parse entities properties
		
		while ( 1 )
		{	
			// parse key
			data = gEngfuncs.COM_ParseFile(data, token);
			if (token[0] == '}')
				break; // finish parsing this entity

			if (!data)
			{	
				gEngfuncs.Con_DPrintf("FindEntitiesInMap: EOF without closing brace\n");
				return 0;
			};
			
			strcpy (keyname, token);

			// another hack to fix keynames with trailing spaces
			n = strlen(keyname);
			while (n && keyname[n-1] == ' ')
			{
				keyname[n-1] = 0;
				n--;
			}
			
			// parse value	
			data = gEngfuncs.COM_ParseFile(data, token);
			if (!data)
			{	
				gEngfuncs.Con_DPrintf("FindEntitiesInMap: EOF without closing brace\n");
				return 0;
			};
	
			if (token[0] == '}')
			{
				gEngfuncs.Con_DPrintf("FindEntitiesInMap: Closing brace without data");
				return 0;
			}

			if (!strcmp(keyname,"classname"))
			{
				if (!strcmp(token, name ))
				{
					found = 1;	// thats our entity
				}
			}
			
			if (!strcmp(keyname,"origin"))
			{
				UTIL_StringToVector_(origin, token);
				gotorigin = true;
			}
				
		} // while (1)

		if ( gotorigin )
		{
			if ( found )
			{
				origins.push_back( origin );
				found = 0;
			}

			gotorigin = false;
		}

	}

	if (origins.size() == 0)
	{
		gEngfuncs.Con_Printf( "FindEntitiesInMap: There are no entities with such classname on the map!\n" );
		return 0;	// we search all entities, but didn't found the correct
	}
	else
	{
		return 1;
	}

}

/*
CalculateCrossPoints
Calculates 6 end points for a cross that is centered in the point with a given origin.
Arguments: vec3_t vecCenter - the origin of the cross center; vec3_t *vecOut - a pointer to an array of 6 origins
*/
void CalculateCrossPoints( vec3_t vecCenter, vec3_t *vecOut )
{
	vec3_t vecEnd;	// End point for the trace
	pmtrace_t tr;	// Trace result

	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );

	for ( int i = 0; i < 3; ++i )
	{
		VectorCopy( vecCenter, vecEnd );

		vecEnd[i] = vecEnd[i] - 5000; // 5000 units is enough for most cases
		gEngfuncs.pEventAPI->EV_PlayerTrace( vecCenter, vecEnd, PM_WORLD_ONLY, -1, &tr ); // Trace
		VectorCopy( tr.endpos, vecOut[i] ); // Store the point

		vecEnd[i] = vecEnd[i] + 10000; // 5000 units to the opposite direction
		gEngfuncs.pEventAPI->EV_PlayerTrace( vecCenter, vecEnd, PM_WORLD_ONLY, -1, &tr ); // Trace
		VectorCopy( tr.endpos, vecOut[i + 3] ); // Store the point
	}
}

void UTIL_StringToVector_( float * pVector, const char *pString )
{
	char *pstr, *pfront, tempString[128];
	int	j;

	strcpy( tempString, pString );
	pstr = pfront = tempString;
	
	for ( j = 0; j < 3; j++ )		
	{
		pVector[j] = atof( pfront );
		
		while ( *pstr && *pstr != ' ' )
			pstr++;
		if (!*pstr)
			break;
		pstr++;
		pfront = pstr;
	}

	if (j < 2)
	{
		for (j = j+1;j < 3; j++)
			pVector[j] = 0;
	}
}
