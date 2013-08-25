#include <vector>
#include "beamdef.h"

#ifndef __RULER_H__
#define __RULER_H__

#define POINT_SPRITE_HALFSIZE 16

typedef struct cl_rulerPoint
{
	vec3_t origin;
	BEAM *beamToThisPoint;

	cl_rulerPoint *pNext;
	cl_rulerPoint *pPrev;
} cl_rulerPoint_t;

typedef struct cl_sphere
{
	vec3_t vecOrigin;

	int random_stuff_here; // This not-used-at-all variable presence fixes the crash. No, I don't know why.

	int iRadius;
	int iRings;
	int iSectors;
} cl_sphere_t;

void StorePoint( vec3_t vecPoint );
void DeleteLast( void );
void AimPos( void );
void PlayerPos( void );
void ViewPos( void );
void RulerAutoFunc( double flTimeDelta );
void PrintDistance( void );
void ResetRuler( void );
void InitRuler( void );
void DrawRulerPoints( void );
void AddOrigin( void );

void AutostopsaveAddPoint( void );
void AutostopsaveDeletePoint( void );
void AutostopsaveAutoFunc( vec3_t vecOrigin );
void AutostopsavePrintOrigin( void );

void CalcSphereVertices( cl_sphere *sphere );

// void FindSpawnsInMap( void );
int FindEntitiesInMap( char *name, std::vector<vec3_t> &origins );
void CalculateCrossPoints( vec3_t vecCenter, vec3_t *vecOut );

void UTIL_StringToVector_( float *pVector, const char *pString );

#endif