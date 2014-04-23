//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

// cl.input.c  -- builds an intended movement command to send to the server

//xxxxxx Move bob and pitch drifting code here and other stuff from view if needed

// Quake is a trademark of Id Software, Inc., (c) 1996 Id Software, Inc. All
// rights reserved.
#include "hud.h"
#include "cl_util.h"
#include "camera.h"
extern "C"
{
#include "kbutton.h"
}
#include "cvardef.h"
#include "usercmd.h"
#include "const.h"
#include "camera.h"
#include "in_defs.h"
#include "view.h"
#include <string.h>
#include <ctype.h>

#include "vgui_TeamFortressViewport.h"

#include "event_api.h" // YaLTeR
#include "eventscripts.h" // YaLTeR

//rofi
extern int _mx;

// YaLTeR Start
float yawRotation = 0.0f;
bool firstRotationInTheAir = true;
bool shouldForceJump = false;
bool autostrafe_turningClockwise = true;
float autostrafe_desiredViewangle = 0.0;
bool perfectstrafe_movingBackwards = false;
bool perfectstrafe_needToUpdateDirection = false;
extern bool g_bPaused;
bool g_bPerfectstrafe = false;
bool g_bOldPerfectstrafe = false;
bool g_bAutostrafe = false;
bool g_bOldAutostrafe = false;
bool g_bOldUnpausedAutostrafe = false;
bool g_bInBhop = false;
bool g_bGroundduck = false;
bool g_bOldGroundduck = false;
bool g_bClAutojump = false;
bool g_bOldClAutojump = false;

#define INVALID_ANGLE -361.0f
float setYaw = INVALID_ANGLE;
float setPitch = INVALID_ANGLE;

#define M_PI         3.1415926535897
#define M_RAD2DEG   57.2957795130823
#define M_DEG2RAD    0.0174532925199
#define M_U          0.0054931640625
#define M_INVU     182.0444444444444

extern double demorec_counter_delta;
extern vec3_t g_org, g_vel, g_vecViewAngle;
extern bool g_bOnGroundDemoInaccurate;
extern bool g_bOldOnGroundDemoInaccurate;
extern cvar_t *cl_printpos;
extern cvar_t *tas_log;

extern cvar_t *tas_perfectstrafe_accel;
extern cvar_t *tas_perfectstrafe_airaccel;
extern cvar_t *tas_perfectstrafe_friction;
extern cvar_t *tas_perfectstrafe_maxspeed;
extern cvar_t *tas_perfectstrafe_autojump;
extern cvar_t *tas_perfectstrafe_movetype;

extern cvar_t *tas_autostrafe_desiredviewangle;
extern cvar_t *tas_autostrafe_manualangle;

/*
    0 - W/S (going left)
    1 - WA/WD
    2 - A/D
    3 - SA/SD
    4 - W/S (going right)
    5 - SA/SD (backwards)
    6 - A/D (backwards)
    7 - WA/WD (backwards)
*/
extern cvar_t *tas_autostrafe_airdir;
extern cvar_t *tas_autostrafe_grounddir;

/*
    0 - D
    1 - SD
    2 - S
    3 - SA
    4 - A
    5 - WA
    6 - W
    7 - WD
*/
extern cvar_t *tas_autostrafe_backpedaldir;

#define BOOLSTRING(b) (b)?"true":"false"

double normangle(double angle)
{
	if (angle >= 180)
		angle -= 360;
	else if (angle < -180)
		angle += 360;

	return angle;
}

double normangleengine(double angle)
{
	if (angle >= 360)
		angle -= 360;
	else if (angle < 0)
		angle += 360;

	return angle;
}
// YaLTeR End

extern "C" 
{
	struct kbutton_s DLLEXPORT *KB_Find( const char *name );
	void DLLEXPORT CL_CreateMove ( float frametime, struct usercmd_s *cmd, int active );
	void DLLEXPORT HUD_Shutdown( void );
	int DLLEXPORT HUD_Key_Event( int eventcode, int keynum, const char *pszCurrentBinding );
}

extern int g_iAlive;

extern int g_weaponselect;
extern cl_enginefunc_t gEngfuncs;

// Defined in pm_math.c
extern "C" float anglemod( float a );

void IN_Init (void);
void IN_Move ( float frametime, usercmd_t *cmd);
void IN_Shutdown( void );
void V_Init( void );
void VectorAngles( const float *forward, float *angles );
int CL_ButtonBits( int );

// xxx need client dll function to get and clear impuse
extern cvar_t *in_joystick;

int	in_impulse	= 0;
int	in_cancel	= 0;

cvar_t	*m_pitch;
cvar_t	*m_yaw;
cvar_t	*m_forward;
cvar_t	*m_side;

cvar_t	*lookstrafe;
cvar_t	*lookspring;
cvar_t	*cl_pitchup;
cvar_t	*cl_pitchdown;
cvar_t	*cl_upspeed;
cvar_t	*cl_forwardspeed;
cvar_t	*cl_backspeed;
cvar_t	*cl_sidespeed;
cvar_t	*cl_movespeedkey;
cvar_t	*cl_yawspeed;
cvar_t	*cl_pitchspeed;
cvar_t	*cl_anglespeedkey;
cvar_t	*cl_vsmoothing;
/*
===============================================================================

KEY BUTTONS

Continuous button event tracking is complicated by the fact that two different
input sources (say, mouse button 1 and the control key) can both press the
same button, but the button should only be released when both of the
pressing key have been released.

When a key event issues a button command (+forward, +attack, etc), it appends
its key number as a parameter to the command so it can be matched up with
the release.

state bit 0 is the current state of the key
state bit 1 is edge triggered on the up to down transition
state bit 2 is edge triggered on the down to up transition

===============================================================================
*/


kbutton_t	in_mlook;
kbutton_t	in_klook;
kbutton_t	in_jlook;
kbutton_t	in_left;
kbutton_t	in_right;
kbutton_t	in_forward;
kbutton_t	in_back;
kbutton_t	in_lookup;
kbutton_t	in_lookdown;
kbutton_t	in_moveleft;
kbutton_t	in_moveright;
kbutton_t	in_strafe;
kbutton_t	in_speed;
kbutton_t	in_use;
kbutton_t	in_jump;
kbutton_t	in_attack;
kbutton_t	in_attack2;
kbutton_t	in_up;
kbutton_t	in_down;
kbutton_t	in_duck;
kbutton_t	in_reload;
kbutton_t	in_alt1;
kbutton_t	in_score;
kbutton_t	in_break;
kbutton_t	in_graph;  // Display the netgraph

typedef struct kblist_s
{
	struct kblist_s *next;
	kbutton_t *pkey;
	char name[32];
} kblist_t;

kblist_t *g_kbkeys = NULL;

/*
============
KB_ConvertString

Removes references to +use and replaces them with the keyname in the output string.  If
 a binding is unfound, then the original text is retained.
NOTE:  Only works for text with +word in it.
============
*/
int KB_ConvertString( char *in, char **ppout )
{
	char sz[ 4096 ];
	char binding[ 64 ];
	char *p;
	char *pOut;
	char *pEnd;
	const char *pBinding;

	if ( !ppout )
		return 0;

	*ppout = NULL;
	p = in;
	pOut = sz;
	while ( *p )
	{
		if ( *p == '+' )
		{
			pEnd = binding;
			while ( *p && ( isalnum( *p ) || ( pEnd == binding ) ) && ( ( pEnd - binding ) < 63 ) )
			{
				*pEnd++ = *p++;
			}

			*pEnd =  '\0';

			pBinding = NULL;
			if ( strlen( binding + 1 ) > 0 )
			{
				// See if there is a binding for binding?
				pBinding = gEngfuncs.Key_LookupBinding( binding + 1 );
			}

			if ( pBinding )
			{
				*pOut++ = '[';
				pEnd = (char *)pBinding;
			}
			else
			{
				pEnd = binding;
			}

			while ( *pEnd )
			{
				*pOut++ = *pEnd++;
			}

			if ( pBinding )
			{
				*pOut++ = ']';
			}
		}
		else
		{
			*pOut++ = *p++;
		}
	}

	*pOut = '\0';

	pOut = ( char * )malloc( strlen( sz ) + 1 );
	strcpy( pOut, sz );
	*ppout = pOut;

	return 1;
}

/*
============
KB_Find

Allows the engine to get a kbutton_t directly ( so it can check +mlook state, etc ) for saving out to .cfg files
============
*/
struct kbutton_s DLLEXPORT *KB_Find( const char *name )
{
	kblist_t *p;
	p = g_kbkeys;
	while ( p )
	{
		if ( !stricmp( name, p->name ) )
			return p->pkey;

		p = p->next;
	}
	return NULL;
}

/*
============
KB_Add

Add a kbutton_t * to the list of pointers the engine can retrieve via KB_Find
============
*/
void KB_Add( const char *name, kbutton_t *pkb )
{
	kblist_t *p;	
	kbutton_t *kb;

	kb = KB_Find( name );
	
	if ( kb )
		return;

	p = ( kblist_t * )malloc( sizeof( kblist_t ) );
	memset( p, 0, sizeof( *p ) );

	strcpy( p->name, name );
	p->pkey = pkb;

	p->next = g_kbkeys;
	g_kbkeys = p;
}

/*
============
KB_Init

Add kbutton_t definitions that the engine can query if needed
============
*/
void KB_Init( void )
{
	g_kbkeys = NULL;

	KB_Add( "in_graph", &in_graph );
	KB_Add( "in_mlook", &in_mlook );
	KB_Add( "in_jlook", &in_jlook );
}

/*
============
KB_Shutdown

Clear kblist
============
*/
void KB_Shutdown( void )
{
	kblist_t *p, *n;
	p = g_kbkeys;
	while ( p )
	{
		n = p->next;
		free( p );
		p = n;
	}
	g_kbkeys = NULL;
}

/*
============
KeyDown
============
*/
void KeyDown (kbutton_t *b)
{
	int		k;
	char	*c;

	c = gEngfuncs.Cmd_Argv(1);
	if (c[0])
		k = atoi(c);
	else
		k = -1;		// typed manually at the console for continuous down

	if (k == b->down[0] || k == b->down[1])
		return;		// repeating key
	
	if (!b->down[0])
		b->down[0] = k;
	else if (!b->down[1])
		b->down[1] = k;
	else
	{
		gEngfuncs.Con_DPrintf ("Three keys down for a button '%c' '%c' '%c'!\n", b->down[0], b->down[1], c);
		return;
	}
	
	if (b->state & 1)
		return;		// still down
	b->state |= 1 + 2;	// down + impulse down
}

/*
============
KeyUp
============
*/
void KeyUp (kbutton_t *b)
{
	int		k;
	char	*c;
	
	c = gEngfuncs.Cmd_Argv(1);
	if (c[0])
		k = atoi(c);
	else
	{ // typed manually at the console, assume for unsticking, so clear all
		b->down[0] = b->down[1] = 0;
		b->state = 4;	// impulse up
		return;
	}

	if (b->down[0] == k)
		b->down[0] = 0;
	else if (b->down[1] == k)
		b->down[1] = 0;
	else
		return;		// key up without coresponding down (menu pass through)
	if (b->down[0] || b->down[1])
	{
		//Con_Printf ("Keys down for button: '%c' '%c' '%c' (%d,%d,%d)!\n", b->down[0], b->down[1], c, b->down[0], b->down[1], c);
		return;		// some other key is still holding it down
	}

	if (!(b->state & 1))
		return;		// still up (this should not happen)

	b->state &= ~1;		// now up
	b->state |= 4; 		// impulse up
}

/*
============
HUD_Key_Event

Return 1 to allow engine to process the key, otherwise, act on it as needed
============
*/
int DLLEXPORT HUD_Key_Event( int down, int keynum, const char *pszCurrentBinding )
{
	if (gViewPort)
		return gViewPort->KeyInput(down, keynum, pszCurrentBinding);
	
	return 1;
}

void IN_BreakDown( void ) { KeyDown( &in_break );};
void IN_BreakUp( void ) { KeyUp( &in_break ); };
void IN_KLookDown (void) {KeyDown(&in_klook);}
void IN_KLookUp (void) {KeyUp(&in_klook);}
void IN_JLookDown (void) {KeyDown(&in_jlook);}
void IN_JLookUp (void) {KeyUp(&in_jlook);}
void IN_MLookDown (void) {KeyDown(&in_mlook);}
void IN_UpDown(void) {KeyDown(&in_up);}
void IN_UpUp(void) {KeyUp(&in_up);}
void IN_DownDown(void) {KeyDown(&in_down);}
void IN_DownUp(void) {KeyUp(&in_down);}
void IN_LeftDown(void) {KeyDown(&in_left);}
void IN_LeftUp(void) {KeyUp(&in_left);}
void IN_RightDown(void) {KeyDown(&in_right);}
void IN_RightUp(void) {KeyUp(&in_right);}

void IN_ForwardDown(void)
{
	KeyDown(&in_forward);
	gHUD.m_Spectator.HandleButtonsDown( IN_FORWARD );
}

void IN_ForwardUp(void)
{
	KeyUp(&in_forward);
	gHUD.m_Spectator.HandleButtonsUp( IN_FORWARD );
}

void IN_BackDown(void)
{
	KeyDown(&in_back);
	gHUD.m_Spectator.HandleButtonsDown( IN_BACK );
}

void IN_BackUp(void)
{
	KeyUp(&in_back);
	gHUD.m_Spectator.HandleButtonsUp( IN_BACK );
}
void IN_LookupDown(void) {KeyDown(&in_lookup);}
void IN_LookupUp(void) {KeyUp(&in_lookup);}
void IN_LookdownDown(void) {KeyDown(&in_lookdown);}
void IN_LookdownUp(void) {KeyUp(&in_lookdown);}
void IN_MoveleftDown(void)
{
	KeyDown(&in_moveleft);
	gHUD.m_Spectator.HandleButtonsDown( IN_MOVELEFT );
}

void IN_MoveleftUp(void)
{
	KeyUp(&in_moveleft);
	gHUD.m_Spectator.HandleButtonsUp( IN_MOVELEFT );
}

void IN_MoverightDown(void)
{
	KeyDown(&in_moveright);
	gHUD.m_Spectator.HandleButtonsDown( IN_MOVERIGHT );
}

void IN_MoverightUp(void)
{
	KeyUp(&in_moveright);
	gHUD.m_Spectator.HandleButtonsUp( IN_MOVERIGHT );
}
void IN_SpeedDown(void) {KeyDown(&in_speed);}
void IN_SpeedUp(void) {KeyUp(&in_speed);}
void IN_StrafeDown(void) {KeyDown(&in_strafe);}
void IN_StrafeUp(void) {KeyUp(&in_strafe);}

// needs capture by hud/vgui also
extern void __CmdFunc_InputPlayerSpecial(void);

void IN_Attack2Down(void) 
{
	KeyDown(&in_attack2);

	gHUD.m_Spectator.HandleButtonsDown( IN_ATTACK2 );
}

void IN_Attack2Up(void) {KeyUp(&in_attack2);}
void IN_UseDown (void)
{
	KeyDown(&in_use);
	gHUD.m_Spectator.HandleButtonsDown( IN_USE );
}
void IN_UseUp (void) {KeyUp(&in_use);}
void IN_JumpDown (void)
{
	KeyDown(&in_jump);
	gHUD.m_Spectator.HandleButtonsDown( IN_JUMP );

}
void IN_JumpUp (void) {KeyUp(&in_jump);}
void IN_DuckDown(void)
{
	KeyDown(&in_duck);
	gHUD.m_Spectator.HandleButtonsDown( IN_DUCK );

}
void IN_DuckUp(void) {KeyUp(&in_duck);}
void IN_ReloadDown(void) {KeyDown(&in_reload);}
void IN_ReloadUp(void) {KeyUp(&in_reload);}
void IN_Alt1Down(void) {KeyDown(&in_alt1);}
void IN_Alt1Up(void) {KeyUp(&in_alt1);}
void IN_GraphDown(void) {KeyDown(&in_graph);}
void IN_GraphUp(void) {KeyUp(&in_graph);}

void IN_AttackDown(void)
{
	KeyDown( &in_attack );
	gHUD.m_Spectator.HandleButtonsDown( IN_ATTACK );
}

void IN_AttackUp(void)
{
	KeyUp( &in_attack );
	in_cancel = 0;
}

// Special handling
void IN_Cancel(void)
{
	in_cancel = 1;
}

void IN_Impulse (void)
{
	in_impulse = atoi( gEngfuncs.Cmd_Argv(1) );
}

void IN_ScoreDown(void)
{
	KeyDown(&in_score);
	if ( gViewPort )
	{
		gViewPort->ShowScoreBoard();
	}
}

void IN_ScoreUp(void)
{
	KeyUp(&in_score);
	if ( gViewPort )
	{
		gViewPort->HideScoreBoard();
	}
}

void IN_MLookUp (void)
{
	KeyUp( &in_mlook );
	if ( !( in_mlook.state & 1 ) && lookspring->value )
	{
		V_StartPitchDrift();
	}
}

/*
===============
CL_KeyState

Returns 0.25 if a key was pressed and released during the frame,
0.5 if it was pressed and held
0 if held then released, and
1.0 if held for the entire time
===============
*/
float CL_KeyState (kbutton_t *key)
{
	float		val = 0.0;
	int			impulsedown, impulseup, down;
	
	impulsedown = key->state & 2;
	impulseup	= key->state & 4;
	down		= key->state & 1;
	
	if ( impulsedown && !impulseup )
	{
		// pressed and held this frame?
		val = down ? 0.5 : 0.0;
	}

	if ( impulseup && !impulsedown )
	{
		// released this frame?
		val = down ? 0.0 : 0.0;
	}

	if ( !impulsedown && !impulseup )
	{
		// held the entire frame?
		val = down ? 1.0 : 0.0;
	}

	if ( impulsedown && impulseup )
	{
		if ( down )
		{
			// released and re-pressed this frame
			val = 0.75;	
		}
		else
		{
			// pressed and released this frame
			val = 0.25;	
		}
	}

	// clear impulses
	key->state &= 1;		
	return val;
}

/*
================
CL_AdjustAngles

Moves the local angle positions
================
*/
void CL_AdjustAngles ( float frametime, float *viewangles )
{
	float	speed;
	float	up, down;
	
	if (in_speed.state & 1)
	{
		speed = frametime * cl_anglespeedkey->value;
	}
	else
	{
		speed = frametime;
	}

	if (!(in_strafe.state & 1))
	{
		viewangles[YAW] -= speed*cl_yawspeed->value*CL_KeyState (&in_right);
		viewangles[YAW] += speed*cl_yawspeed->value*CL_KeyState (&in_left);

		// YaLTeR Start
		/*if (!g_bPaused)
			gEngfuncs.Con_Printf("Yaw: %f; yawRot: %f\n", viewangles[YAW], yawRotation);*/

		if (setYaw != INVALID_ANGLE)
		{
			viewangles[YAW] = setYaw;
			setYaw = INVALID_ANGLE;

			float temp = viewangles[YAW] * M_INVU;
			temp = floor( temp + 0.5 );
			viewangles[YAW] = temp * M_U;
		}
		else if (yawRotation != 0.0f)
		{
			viewangles[YAW] += yawRotation;
			//viewangles[YAW] = normangleengine( viewangles[YAW] );

			//gEngfuncs.Con_Printf("Pre-deanglemod: %f; ", viewangles[YAW]);

			float temp = viewangles[YAW] * M_INVU;
			temp = floor(temp + 0.5);
			viewangles[YAW] = temp * M_U;

			//gEngfuncs.Con_Printf("post-deanglemod: %f\n", viewangles[YAW]);
		}
		// YaLTeR End

		if( _mx != -1 )
			viewangles[YAW] = anglemod(viewangles[YAW]);
	}
	if (in_klook.state & 1)
	{
		V_StopPitchDrift ();
		viewangles[PITCH] -= speed*cl_pitchspeed->value * CL_KeyState (&in_forward);
		viewangles[PITCH] += speed*cl_pitchspeed->value * CL_KeyState (&in_back);
	}
	
	up = CL_KeyState (&in_lookup);
	down = CL_KeyState(&in_lookdown);
	
	viewangles[PITCH] -= speed*cl_pitchspeed->value * up;
	viewangles[PITCH] += speed*cl_pitchspeed->value * down;

	// YaLTeR Start
	if (setPitch != INVALID_ANGLE)
	{
		viewangles[PITCH] = setPitch;
		setPitch = INVALID_ANGLE;

		float temp = viewangles[PITCH] * M_INVU;
		temp = floor( temp + 0.5 );
		viewangles[PITCH] = temp * M_U;
	}
	// YaLTeR End

	if (up || down)
		V_StopPitchDrift ();
		
	if (viewangles[PITCH] > cl_pitchdown->value)
		viewangles[PITCH] = cl_pitchdown->value;
	if (viewangles[PITCH] < -cl_pitchup->value)
		viewangles[PITCH] = -cl_pitchup->value;

	if (viewangles[ROLL] > 50)
		viewangles[ROLL] = 50;
	if (viewangles[ROLL] < -50)
		viewangles[ROLL] = -50;
}

/*
================
CL_CreateMove

Send the intended movement message to the server
if active == 1 then we are 1) not playing back demos ( where our commands are ignored ) and
2 ) we have finished signing on to server
================
*/

// YaLTeR Start
double CL_GetBorderVelocity(double v0, double A, double Agr, float maxspeed, float friction, float frametime)
{
	double ff = friction * frametime;
	double bordervel = -1.0;

	if ((A < 30) && (Agr < maxspeed))
	{
		double temp = fabs(60*A - A*A + Agr*Agr - 2*Agr*maxspeed);
		bordervel = sqrt(temp) / sqrt(2*ff - ff*ff);
	}
	else if ((A >= 30) && (Agr < maxspeed))
	{
		if (v0 < (maxspeed - Agr))
		{
			double temp = fabs(Agr*Agr - 1800*ff + 900*ff*ff);
			bordervel = fabs( (-Agr + Agr*ff - sqrt(temp)) / (-2*ff + ff*ff) );
		}
		else
		{
			bordervel = sqrt( fabs( 900 + Agr*Agr - 2*Agr*maxspeed ) ) / ( sqrt( fabs(ff - 2) ) * sqrt( fabs(ff) ) );
		}
	}
	else if ((A < 30) && (Agr >= maxspeed))
	{
		bordervel = sqrt( fabs( 60*A - A*A - maxspeed*maxspeed ) ) / sqrt( fabs(-2*ff + ff*ff) );
	}
	else if ((A >= 30) && (Agr >= maxspeed))
	{
		bordervel = sqrt( fabs(900 - maxspeed*maxspeed) ) / sqrt( fabs(-2*ff + ff*ff) );
	}

	return bordervel;
}

double CL_GetOptimalAngle(float maxspeed, double A, double v0, bool onground)
{
	if (v0 == 0)
		return 0;

	double alphacos = (maxspeed - A) / v0;
	if (alphacos >= 1) return 0;
	if (alphacos <= 0) return 90;

	return (acos(alphacos) * M_RAD2DEG);
}

double CL_GetMaximumAngle(float maxspeed, double A, double v0, bool onground)
{
	if (v0 == 0)
		return 90;

	double alphacos = (-A) / v0;
	if (alphacos >= 1) return 0;
	if (alphacos <= -1) return 180;

	return (acos(alphacos) * M_RAD2DEG);
}

// How much the player view (not wishvel!) should differentiate from the velocity.
double CL_GetBackpedallingAngle(float maxspeed, double A, double v0, bool onground, float dirOverride = -1.0f)
{
	float dir;
	if (dirOverride != -1.0f)
	{
		dir = dirOverride;
	}
	else
	{
		dir = tas_autostrafe_backpedaldir->value;
	}

	if (dir == 1.0f)
		return 45;
	else if (dir == 2.0f)
		return 0;
	else if (dir == 3.0f)
		return 315;
	else if (dir == 4.0f)
		return 270;
	else if (dir == 5.0f)
		return 225;
	else if (dir == 6.0f)
		return 180;
	else if (dir == 7.0f)
		return 135;
	else
		return 90;
}

double CL_GetWishAngleDiff(bool onground, bool turningClockwise, float dirOverride = -1.0f)
{
	float dir;
	if (dirOverride != -1.0f)
	{
		dir = dirOverride;
	}
	else
	{
		if (onground)
			dir = tas_autostrafe_grounddir->value;
		else
			dir = tas_autostrafe_airdir->value;
	}

	double result;
	if (dir == 1.0f)
		result = 45;
	else if (dir == 2.0f)
		result = 90;
	else if (dir == 3.0f)
		result = 135;
	else if (dir == 4.0f)
		result = 180;
	else if (dir == 5.0f)
		result = 225;
	else if (dir == 6.0f)
		result = 270;
	else if (dir == 7.0f)
		result = 315;
	else
		result = 0;

	if ( turningClockwise )
	{
		result = -result;
	}
	else
	{
		if (result == 0)
			result = 180;
		else if (result == 180)
			result = 0;
	}

	return result;
}

bool CL_TurningClockwise(bool onground, float dirOverride = -1.0f)
{
	float dir;
	if (dirOverride != -1.0f)
	{
		dir = dirOverride;
	}
	else
	{
		if (onground)
			dir = tas_autostrafe_grounddir->value;
		else
			dir = tas_autostrafe_airdir->value;
	}

	if (dir == 1.0f)
		return ((in_forward.state & 1) && (in_moveright.state & 1));
	else if (dir == 2.0f)
		return (in_moveright.state & 1);
	else if (dir == 3.0f)
		return ((in_back.state & 1) && (in_moveright.state & 1));
	else if (dir == 4.0f)
		return (in_back.state & 1);
	else if (dir == 5.0f)
		return ((in_back.state & 1) && (in_moveleft.state & 1));
	else if (dir == 6.0f)
		return (in_moveleft.state & 1);
	else if (dir == 7.0f)
		return ((in_forward.state & 1) && (in_moveleft.state & 1));
	else
		return (in_forward.state & 1);
}

bool CL_IsMovementKeyPressed()
{
	return ((in_forward.state & 1) || (in_back.state & 1) || (in_moveright.state & 1) || (in_moveleft.state & 1));
}

float CL_GetPerfectstrafeDir()
{
	if (!perfectstrafe_movingBackwards)
	{
		if (((in_forward.state & 1) && (in_moveright.state & 1)) || ((in_forward.state & 1) && (in_moveleft.state & 1)))
			return 1;
		else if (((in_back.state & 1) && (in_moveright.state & 1)) || ((in_back.state & 1) && (in_moveleft.state & 1)))
			return 3;
		else if ((in_moveright.state & 1) || (in_moveleft.state & 1))
			return 2;
		else
			return 0;
	}
	else
	{
		if (((in_forward.state & 1) && (in_moveright.state & 1)) || ((in_forward.state & 1) && (in_moveleft.state & 1)))
			return 7;
		else if (((in_back.state & 1) && (in_moveright.state & 1)) || ((in_back.state & 1) && (in_moveleft.state & 1)))
			return 5;
		else if ((in_moveright.state & 1) || (in_moveleft.state & 1))
			return 6;
		else
			return 4;
	}
}
// YaLTeR End

void DLLEXPORT CL_CreateMove ( float frametime, struct usercmd_s *cmd, int active )
{	
	float spd;
	vec3_t viewangles;
	static vec3_t oldangles;

	if ( active )
	{

		//memset( viewangles, 0, sizeof( vec3_t ) );
		//viewangles[ 0 ] = viewangles[ 1 ] = viewangles[ 2 ] = 0.0;
		gEngfuncs.GetViewAngles( (float *)viewangles );

		// YaLTeR Start
        if (!g_bPaused && cl_printpos->value)
        {
            gEngfuncs.Con_Printf("Origin (x, y, z): %f, %f, %f\n", g_org[0], g_org[1], g_org[2]);
        }

		float maxspeed = tas_perfectstrafe_maxspeed->value;
		float friction = tas_perfectstrafe_friction->value;

		vec3_t viewOfs;
		VectorClear(viewOfs);
		gEngfuncs.pEventAPI->EV_LocalPlayerViewheight( viewOfs );
		if (viewOfs[2] == VEC_DUCK_VIEW)
		{
			maxspeed *= 0.333;
		}

		double A = tas_perfectstrafe_airaccel->value * maxspeed * frametime;
		double Agr = tas_perfectstrafe_accel->value * maxspeed * frametime;
		double v0 = hypot(g_vel[0], g_vel[1]);

		// Air acceleration
		double alpha;
		if (tas_perfectstrafe_movetype->value == 1.0f)
		{
			alpha = CL_GetMaximumAngle(30, A, v0, false);
		}
		else if (tas_perfectstrafe_movetype->value == 2.0f)
		{
			alpha = CL_GetBackpedallingAngle(30, A, v0, false);
		}
		else
		{
			alpha = CL_GetOptimalAngle(30, A, v0, false);
		}
		
		// Ground acceleration
		double frictiondrop = v0 * friction * frametime; // TODO: edgefriction
		double actualspeed = v0 - frictiondrop;
		double alpha_gr;
		
		if (tas_perfectstrafe_movetype->value == 1.0f)
		{
			alpha_gr = CL_GetMaximumAngle(maxspeed, Agr, actualspeed, true);
		}
		else if (tas_perfectstrafe_movetype->value == 2.0f)
		{
			alpha_gr = CL_GetBackpedallingAngle(maxspeed, A, v0, true);
		}
		else
		{
			alpha_gr = CL_GetOptimalAngle(maxspeed, Agr, actualspeed, true);
		}

		if ( tas_autostrafe_manualangle->value != 0.0f )
		{
			autostrafe_desiredViewangle = tas_autostrafe_desiredviewangle->value;
		}

		if ((g_bPerfectstrafe != g_bOldPerfectstrafe) && g_bPerfectstrafe)
		{
			perfectstrafe_needToUpdateDirection = true;
		}

		g_bOldPerfectstrafe = g_bPerfectstrafe;

		if (g_bPerfectstrafe && !CL_IsMovementKeyPressed())
		{
			perfectstrafe_needToUpdateDirection = true;
		}

		if (perfectstrafe_needToUpdateDirection && CL_IsMovementKeyPressed())
		{
			perfectstrafe_needToUpdateDirection = false;
			perfectstrafe_movingBackwards = (in_back.state & 1);
		}

		// Same condition as below, to check if we're groundstrafing
		if ( !g_bPaused && ( g_bAutostrafe || g_bPerfectstrafe ) && ( g_bOnGroundDemoInaccurate && !g_bInBhop ) && ( g_bAutostrafe || CL_IsMovementKeyPressed() ) )
		{
			if ( tas_perfectstrafe_autojump->value )
			{
				double bordervel = CL_GetBorderVelocity(v0, A, Agr, maxspeed, friction, frametime);

				/*if (!g_bPaused)
				{
					gEngfuncs.Con_Printf("Bordervel: %f\n", bordervel);
				}*/

				if ((bordervel != -1.0f) && (v0 >= bordervel))
				{
					shouldForceJump = true;
					g_bInBhop = true;
				}
			}
		}
		
		if ( g_bAutostrafe && !g_bOldAutostrafe )
		{
			if ( tas_autostrafe_manualangle->value )
			{
				autostrafe_desiredViewangle = tas_autostrafe_desiredviewangle->value;
			}
			else
			{
				autostrafe_desiredViewangle = viewangles[YAW];
			}
		}

		g_bOldAutostrafe = g_bAutostrafe;

		if ( !g_bPaused )
		{
			g_bOldUnpausedAutostrafe = g_bAutostrafe;
		}

		if ( ( g_bAutostrafe || g_bPerfectstrafe ) && ( !g_bOnGroundDemoInaccurate || g_bInBhop ) && ( g_bAutostrafe || CL_IsMovementKeyPressed() ) )
		{
			if ( firstRotationInTheAir )
			{
				firstRotationInTheAir = false;
				g_bInBhop = true;
			}
		
			vec3_t velDir, angDir;
			vec3_t velAng;
			VectorCopy(g_vel, velDir);
			velDir[2] = 0;
			VectorAngles( velDir, velAng );

			if ( g_bAutostrafe )
			{
				double v0delta = normangle(autostrafe_desiredViewangle - velAng[YAW]);
				autostrafe_turningClockwise = (v0delta < 0);

				if (!autostrafe_turningClockwise && (tas_perfectstrafe_movetype->value != 2.0f))
				{
					alpha = -alpha;
				}
			}
			else
			{
				if (!CL_TurningClockwise(false, CL_GetPerfectstrafeDir()) && (tas_perfectstrafe_movetype->value != 2.0f))
				{
					alpha = -alpha;
				}
			}

			double wishang;
			if ( tas_perfectstrafe_movetype->value != 2.0f )
			{
				if ( g_bAutostrafe )
				{
					wishang = normangleengine(viewangles[YAW] + CL_GetWishAngleDiff(false, autostrafe_turningClockwise));
				}
				else
				{
					wishang = normangleengine(viewangles[YAW] + CL_GetWishAngleDiff(false, CL_TurningClockwise(false, CL_GetPerfectstrafeDir()), CL_GetPerfectstrafeDir()));
				}
			}
			else
			{
				wishang = normangleengine(viewangles[YAW]);
			}

			double phi = normangle(velAng[YAW] - wishang);
			
			/*if (!g_bPaused)
			{
				gEngfuncs.Con_Printf("alpha: %f\n", alpha);
			}*/

			if (!g_bPaused)
			{
				yawRotation = phi - alpha;
			}
			else
			{
				yawRotation = 0;
			}
		}
		else if ( ( g_bAutostrafe || g_bPerfectstrafe ) && ( g_bOnGroundDemoInaccurate && !g_bInBhop ) && ( g_bAutostrafe || CL_IsMovementKeyPressed() ) )
		{
			vec3_t velAng;
			if (v0 != 0)
			{
				vec3_t velDir, angDir;
				VectorCopy(g_vel, velDir);
				velDir[2] = 0;
				VectorAngles( velDir, velAng );
			}
			else
			{
				velAng[YAW] = viewangles[YAW];
			}
			
			if ( g_bAutostrafe )
			{
				double v0delta = normangle(autostrafe_desiredViewangle - velAng[YAW]);
				autostrafe_turningClockwise = (v0delta < 0);

				if (!autostrafe_turningClockwise && (tas_perfectstrafe_movetype->value != 2.0f))
				{
					alpha_gr = -alpha_gr;
				}
			}
			else
			{
				if (!CL_TurningClockwise(true, CL_GetPerfectstrafeDir()) && (tas_perfectstrafe_movetype->value != 2.0f))
				{
					alpha_gr = -alpha_gr;
				}
			}

			/*if (!g_bPaused)
			{
				gEngfuncs.Con_Printf("alpha: %f\n", alpha);
			}*/
			
			double wishang;
			if (tas_perfectstrafe_movetype->value != 2.0f)
			{
				if ( g_bAutostrafe )
				{
					wishang = normangleengine(viewangles[YAW] + CL_GetWishAngleDiff(true, autostrafe_turningClockwise));
				}
				else
				{
					wishang = normangleengine(viewangles[YAW] + CL_GetWishAngleDiff(true, CL_TurningClockwise(true, CL_GetPerfectstrafeDir()), CL_GetPerfectstrafeDir()));
				}
			}
			else
			{
				wishang = normangleengine(viewangles[YAW]);
			}

			if ( (actualspeed == 0) || ((alpha_gr == 0) && (tas_perfectstrafe_movetype->value != 2.0f)) )
			{
				if (!g_bPaused)
				{
					yawRotation = normangle(autostrafe_desiredViewangle - wishang);
				}
				else
				{
					yawRotation = 0;
				}
			}
			else
			{
				double phi = normangle(velAng[YAW] - wishang);
			
				if (!g_bPaused)
				{
					//gEngfuncs.Con_Printf("Current client speed: %f; predicted client speed: %f\n", v0, sqrt(v0*v0 + A*A + 2*v0*A*cos(deg2rad(alpha))));
				}

				if (!g_bPaused)
				{
					yawRotation = phi - alpha_gr;
					//gEngfuncs.Con_Printf("phi: %f; yawRot: %f; wishang: %f; velang: %f; alpha: %f\n", phi, yawRotation, wishang, velAng[YAW], alpha_gr);
				}
				else
				{
					yawRotation = 0;
				}
			}
		}
		else
		{
			yawRotation = 0;
			firstRotationInTheAir = true;
			g_bInBhop = false;
		}

        if (tas_log->value)
        {
            // Log everything!
            gEngfuncs.Con_Printf("-- LOGSTART -- Time: %f -- Frametime: %f --\n", demorec_counter_delta, frametime);
            gEngfuncs.Con_Printf("Origin (x, y, z): %f, %f, %f\n\
Viewangles (p, y, r): %f, %f, %f\n\
Velocity(x, y, z): %f, %f, %f\n\
v0: %f; frictiondrop: %f; actualspeed: %f\n\
alpha: %f; alpha_gr: %f\n\
g_bAutostrafe: %s\n\
yawRotation: %f\n",
                g_org[0], g_org[1], g_org[2],
                viewangles[PITCH], viewangles[YAW], viewangles[ROLL],
                g_vel[0], g_vel[1], g_vel[2],
                v0, frictiondrop, actualspeed,
                alpha, alpha_gr,
                BOOLSTRING(g_bAutostrafe),
                yawRotation);
            gEngfuncs.Con_Printf("-- LOGEND --\n");

        }
		// YaLTeR End

		CL_AdjustAngles ( frametime, viewangles );

		memset (cmd, 0, sizeof(*cmd));
		
		//vec3_t angpost;
		//vec3_t angpre;
		//gEngfuncs.GetViewAngles( (float *)angpre );
		gEngfuncs.SetViewAngles( (float *)viewangles );
		//if (!g_bPaused)
		//{
			//gEngfuncs.GetViewAngles( (float *)angpost );
			//gEngfuncs.Con_DPrintf("Angles pre: %f; %f; viewangles: %f; %f; angles post: %f; %f;\n", angpre[0], angpre[1], viewangles[0], viewangles[1], angpost[0], angpost[1]);
			//gEngfuncs.Con_DPrintf("Velocity: %f; %f\n", g_vel[0], g_vel[1]);
		//}

		if ( in_strafe.state & 1 )
		{
			cmd->sidemove += cl_sidespeed->value * CL_KeyState (&in_right);
			cmd->sidemove -= cl_sidespeed->value * CL_KeyState (&in_left);
		}

		// YaLTeR Start
		if ( g_bAutostrafe && (tas_perfectstrafe_movetype->value != 2.0f) ) // 2.0f - backpedalling.
		{
			if ( g_bOldUnpausedAutostrafe )
			{
				float dir;
				if (g_bOnGroundDemoInaccurate && !g_bInBhop)
					dir = tas_autostrafe_grounddir->value;
				else
					dir = tas_autostrafe_airdir->value;

				if ((dir == 1.0f) || (dir == 2.0f) || (dir == 3.0f))
				{
					if ( autostrafe_turningClockwise )
					{
						cmd->sidemove += cl_sidespeed->value;
					}
					else
					{
						cmd->sidemove -= cl_sidespeed->value;
					}
				}
				else if ((dir == 5.0f) || (dir == 6.0f) || (dir == 7.0f))
				{
					if ( autostrafe_turningClockwise )
					{
						cmd->sidemove -= cl_sidespeed->value;
					}
					else
					{
						cmd->sidemove += cl_sidespeed->value;
					}
				}
			}
		}
		else if (g_bPerfectstrafe && (tas_perfectstrafe_movetype->value != 2.0f) && CL_IsMovementKeyPressed()) // 2.0f - backpedalling.
		{
			float dir = CL_GetPerfectstrafeDir();

			if ((dir == 1.0f) || (dir == 2.0f) || (dir == 3.0f))
			{
				if ( CL_TurningClockwise(false, dir) )
				{
					cmd->sidemove += cl_sidespeed->value;
				}
				else
				{
					cmd->sidemove -= cl_sidespeed->value;
				}
			}
			else if ((dir == 5.0f) || (dir == 6.0f) || (dir == 7.0f))
			{
				if ( CL_TurningClockwise(false, dir) )
				{
					cmd->sidemove -= cl_sidespeed->value;
				}
				else
				{
					cmd->sidemove += cl_sidespeed->value;
				}
			}
		}
		else if (((g_bPerfectstrafe && CL_IsMovementKeyPressed()) || g_bAutostrafe) && (tas_perfectstrafe_movetype->value == 2.0f))
		{
			float dir = tas_autostrafe_backpedaldir->value;

			if ((dir == 0.0f) || (dir == 1.0f) || (dir == 7.0f))
			{
				cmd->sidemove += cl_sidespeed->value;
			}
			else if ((dir == 3.0f) || (dir == 4.0f) || (dir == 5.0f))
			{
				cmd->sidemove -= cl_sidespeed->value;
			}
		}
		else
		{
			cmd->sidemove += cl_sidespeed->value * CL_KeyState (&in_moveright);
			cmd->sidemove -= cl_sidespeed->value * CL_KeyState (&in_moveleft);
		}
		// YaLTeR End

		cmd->upmove += cl_upspeed->value * CL_KeyState (&in_up);
		cmd->upmove -= cl_upspeed->value * CL_KeyState (&in_down);

		if ( !(in_klook.state & 1 ) )
		{
			/*if ( g_bAutostrafe && g_bOldUnpausedAutostrafe && g_bOnGroundDemoInaccurate && !g_bInBhop && !(in_forward.state & 1) )
			{
				cmd->forwardmove += cl_forwardspeed->value;
			}*/

			if ((!g_bPerfectstrafe || !CL_IsMovementKeyPressed()) && !g_bAutostrafe)
			{
				cmd->forwardmove += cl_forwardspeed->value * CL_KeyState (&in_forward);
				cmd->forwardmove -= cl_backspeed->value * CL_KeyState (&in_back);
			}
			else if (((g_bPerfectstrafe && CL_IsMovementKeyPressed()) || g_bAutostrafe) && (tas_perfectstrafe_movetype->value == 2.0f))
			{
				float dir = tas_autostrafe_backpedaldir->value;

				if ((dir == 5.0f) || (dir == 6.0f) || (dir == 7.0f))
				{
					cmd->forwardmove += cl_forwardspeed->value;
				}
				else if ((dir == 1.0f) || (dir == 2.0f) || (dir == 3.0f))
				{
					cmd->forwardmove -= cl_backspeed->value;
				}
			}
			else if (g_bAutostrafe)
			{
				float dir;
				if (g_bOnGroundDemoInaccurate && !g_bInBhop)
					dir = tas_autostrafe_grounddir->value;
				else
					dir = tas_autostrafe_airdir->value;

				if (dir == 0.0f)
				{
					if ( autostrafe_turningClockwise )
					{
						cmd->forwardmove += cl_forwardspeed->value;
					}
					else
					{
						cmd->forwardmove -= cl_backspeed->value;
					}
				}
				else if (dir == 4.0f)
				{
					if ( autostrafe_turningClockwise )
					{
						cmd->forwardmove -= cl_backspeed->value;
					}
					else
					{
						cmd->forwardmove += cl_forwardspeed->value;
					}
				}
				else if ((dir == 1.0f) || (dir == 7.0f))
				{
					cmd->forwardmove += cl_forwardspeed->value;
				}
				else if ((dir == 3.0f) || (dir == 5.0f))
				{
					cmd->forwardmove -= cl_backspeed->value;
				}
			}
			else if (g_bPerfectstrafe)
			{
				float dir = CL_GetPerfectstrafeDir();

				if (dir == 0.0f)
				{
					if ( CL_TurningClockwise(false, dir) )
					{
						cmd->forwardmove += cl_forwardspeed->value;
					}
					else
					{
						cmd->forwardmove -= cl_backspeed->value;
					}
				}
				else if (dir == 4.0f)
				{
					if ( CL_TurningClockwise(false, dir) )
					{
						cmd->forwardmove -= cl_backspeed->value;
					}
					else
					{
						cmd->forwardmove += cl_forwardspeed->value;
					}
				}
				else if ((dir == 1.0f) || (dir == 7.0f))
				{
					cmd->forwardmove += cl_forwardspeed->value;
				}
				else if ((dir == 3.0f) || (dir == 5.0f))
				{
					cmd->forwardmove -= cl_backspeed->value;
				}
			}
		}

		// adjust for speed key
		if ( in_speed.state & 1 )
		{
			cmd->forwardmove *= cl_movespeedkey->value;
			cmd->sidemove *= cl_movespeedkey->value;
			cmd->upmove *= cl_movespeedkey->value;
		}

		// clip to maxspeed
		spd = gEngfuncs.GetClientMaxspeed();
		if ( spd != 0.0 )
		{
			// scale the 3 speeds so that the total velocity is not > cl.maxspeed
			float fmov = sqrt( (cmd->forwardmove*cmd->forwardmove) + (cmd->sidemove*cmd->sidemove) + (cmd->upmove*cmd->upmove) );

			if ( fmov > spd )
			{
				float fratio = spd / fmov;
				cmd->forwardmove *= fratio;
				cmd->sidemove *= fratio;
				cmd->upmove *= fratio;
			}
		}

		// Allow mice and other controllers to add their inputs
		IN_Move ( frametime, cmd );
	}

	cmd->impulse = in_impulse;
	in_impulse = 0;

	cmd->weaponselect = g_weaponselect;
	g_weaponselect = 0;
	//
	// set button and flag bits
	//
	cmd->buttons = CL_ButtonBits( 1 );

	// YaLTeR Start
	if ( g_bOnGroundDemoInaccurate != g_bOldOnGroundDemoInaccurate )
	{
		if ( g_bOnGroundDemoInaccurate )
		{
			if ( g_bGroundduck )
			{
				cmd->buttons |= IN_DUCK;
			}
			else if ( g_bClAutojump)
			{
				cmd->buttons |= IN_JUMP;
			}
		}
	}

	if ( (g_bGroundduck != g_bOldGroundduck) && g_bGroundduck )
	{
		cmd->buttons |= IN_DUCK;
	}

	if ( (g_bClAutojump != g_bOldClAutojump) && g_bClAutojump )
	{
		cmd->buttons |= IN_JUMP;
	}

	g_bOldGroundduck = g_bGroundduck;
	g_bOldClAutojump = g_bClAutojump;

	if ( shouldForceJump )
	{
		if ( !g_bOnGroundDemoInaccurate )
		{
			shouldForceJump = false;
		}

		cmd->buttons |= IN_JUMP;
	}
	// YaLTeR End

	// If they're in a modal dialog, ignore the attack button.
	if(GetClientVoiceMgr()->IsInSquelchMode())
		cmd->buttons &= ~IN_ATTACK;

	// Using joystick?
	if ( in_joystick->value )
	{
		if ( cmd->forwardmove > 0 )
		{
			cmd->buttons |= IN_FORWARD;
		}
		else if ( cmd->forwardmove < 0 )
		{
			cmd->buttons |= IN_BACK;
		}
	}

	gEngfuncs.GetViewAngles( (float *)viewangles );
	// Set current view angles.

	if ( g_iAlive )
	{
		VectorCopy( viewangles, cmd->viewangles );
		VectorCopy( viewangles, oldangles );
	}
	else
	{
		VectorCopy( oldangles, cmd->viewangles );
	}

}

/*
============
CL_IsDead

Returns 1 if health is <= 0
============
*/
int	CL_IsDead( void )
{
	return ( gHUD.m_Health.m_iHealth <= 0 ) ? 1 : 0;
}

/*
============
CL_ButtonBits

Returns appropriate button info for keyboard and mouse state
Set bResetState to 1 to clear old state info
============
*/
int CL_ButtonBits( int bResetState )
{
	int bits = 0;

	if ( in_attack.state & 3 )
	{
		bits |= IN_ATTACK;
	}
	
	if (in_duck.state & 3)
	{
		bits |= IN_DUCK;
	}
 
	if (in_jump.state & 3)
	{
		bits |= IN_JUMP;
	}

	if ( in_forward.state & 3 )
	{
		bits |= IN_FORWARD;
	}
	
	if (in_back.state & 3)
	{
		bits |= IN_BACK;
	}

	if (in_use.state & 3)
	{
		bits |= IN_USE;
	}

	if (in_cancel)
	{
		bits |= IN_CANCEL;
	}

	if ( in_left.state & 3 )
	{
		bits |= IN_LEFT;
	}
	
	if (in_right.state & 3)
	{
		bits |= IN_RIGHT;
	}
	
	if ( in_moveleft.state & 3 )
	{
		bits |= IN_MOVELEFT;
	}
	
	if (in_moveright.state & 3)
	{
		bits |= IN_MOVERIGHT;
	}

	if (in_attack2.state & 3)
	{
		bits |= IN_ATTACK2;
	}

	if (in_reload.state & 3)
	{
		bits |= IN_RELOAD;
	}

	if (in_alt1.state & 3)
	{
		bits |= IN_ALT1;
	}

	if ( in_score.state & 3 )
	{
		bits |= IN_SCORE;
	}

	// Dead or in intermission? Shore scoreboard, too
	if ( CL_IsDead() || gHUD.m_iIntermission )
	{
		bits |= IN_SCORE;
	}

	if ( bResetState )
	{
		in_attack.state &= ~2;
		in_duck.state &= ~2;
		in_jump.state &= ~2;
		in_forward.state &= ~2;
		in_back.state &= ~2;
		in_use.state &= ~2;
		in_left.state &= ~2;
		in_right.state &= ~2;
		in_moveleft.state &= ~2;
		in_moveright.state &= ~2;
		in_attack2.state &= ~2;
		in_reload.state &= ~2;
		in_alt1.state &= ~2;
		in_score.state &= ~2;
	}

	return bits;
}

/*
============
CL_ResetButtonBits

============
*/
void CL_ResetButtonBits( int bits )
{
	int bitsNew = CL_ButtonBits( 0 ) ^ bits;

	// Has the attack button been changed
	if ( bitsNew & IN_ATTACK )
	{
		// Was it pressed? or let go?
		if ( bits & IN_ATTACK )
		{
			KeyDown( &in_attack );
		}
		else
		{
			// totally clear state
			in_attack.state &= ~7;
		}
	}
}

/*
============
InitInput
============
*/
void InitInput (void)
{
	gEngfuncs.pfnAddCommand ("+moveup",IN_UpDown);
	gEngfuncs.pfnAddCommand ("-moveup",IN_UpUp);
	gEngfuncs.pfnAddCommand ("+movedown",IN_DownDown);
	gEngfuncs.pfnAddCommand ("-movedown",IN_DownUp);
	gEngfuncs.pfnAddCommand ("+left",IN_LeftDown);
	gEngfuncs.pfnAddCommand ("-left",IN_LeftUp);
	gEngfuncs.pfnAddCommand ("+right",IN_RightDown);
	gEngfuncs.pfnAddCommand ("-right",IN_RightUp);
	gEngfuncs.pfnAddCommand ("+forward",IN_ForwardDown);
	gEngfuncs.pfnAddCommand ("-forward",IN_ForwardUp);
	gEngfuncs.pfnAddCommand ("+back",IN_BackDown);
	gEngfuncs.pfnAddCommand ("-back",IN_BackUp);
	gEngfuncs.pfnAddCommand ("+lookup", IN_LookupDown);
	gEngfuncs.pfnAddCommand ("-lookup", IN_LookupUp);
	gEngfuncs.pfnAddCommand ("+lookdown", IN_LookdownDown);
	gEngfuncs.pfnAddCommand ("-lookdown", IN_LookdownUp);
	gEngfuncs.pfnAddCommand ("+strafe", IN_StrafeDown);
	gEngfuncs.pfnAddCommand ("-strafe", IN_StrafeUp);
	gEngfuncs.pfnAddCommand ("+moveleft", IN_MoveleftDown);
	gEngfuncs.pfnAddCommand ("-moveleft", IN_MoveleftUp);
	gEngfuncs.pfnAddCommand ("+moveright", IN_MoverightDown);
	gEngfuncs.pfnAddCommand ("-moveright", IN_MoverightUp);
	gEngfuncs.pfnAddCommand ("+speed", IN_SpeedDown);
	gEngfuncs.pfnAddCommand ("-speed", IN_SpeedUp);
	gEngfuncs.pfnAddCommand ("+attack", IN_AttackDown);
	gEngfuncs.pfnAddCommand ("-attack", IN_AttackUp);
	gEngfuncs.pfnAddCommand ("+attack2", IN_Attack2Down);
	gEngfuncs.pfnAddCommand ("-attack2", IN_Attack2Up);
	gEngfuncs.pfnAddCommand ("+use", IN_UseDown);
	gEngfuncs.pfnAddCommand ("-use", IN_UseUp);
	gEngfuncs.pfnAddCommand ("+jump", IN_JumpDown);
	gEngfuncs.pfnAddCommand ("-jump", IN_JumpUp);
	gEngfuncs.pfnAddCommand ("impulse", IN_Impulse);
	gEngfuncs.pfnAddCommand ("+klook", IN_KLookDown);
	gEngfuncs.pfnAddCommand ("-klook", IN_KLookUp);
	gEngfuncs.pfnAddCommand ("+mlook", IN_MLookDown);
	gEngfuncs.pfnAddCommand ("-mlook", IN_MLookUp);
	gEngfuncs.pfnAddCommand ("+jlook", IN_JLookDown);
	gEngfuncs.pfnAddCommand ("-jlook", IN_JLookUp);
	gEngfuncs.pfnAddCommand ("+duck", IN_DuckDown);
	gEngfuncs.pfnAddCommand ("-duck", IN_DuckUp);
	gEngfuncs.pfnAddCommand ("+reload", IN_ReloadDown);
	gEngfuncs.pfnAddCommand ("-reload", IN_ReloadUp);
	gEngfuncs.pfnAddCommand ("+alt1", IN_Alt1Down);
	gEngfuncs.pfnAddCommand ("-alt1", IN_Alt1Up);
	gEngfuncs.pfnAddCommand ("+score", IN_ScoreDown);
	gEngfuncs.pfnAddCommand ("-score", IN_ScoreUp);
	gEngfuncs.pfnAddCommand ("+showscores", IN_ScoreDown);
	gEngfuncs.pfnAddCommand ("-showscores", IN_ScoreUp);
	gEngfuncs.pfnAddCommand ("+graph", IN_GraphDown);
	gEngfuncs.pfnAddCommand ("-graph", IN_GraphUp);
	gEngfuncs.pfnAddCommand ("+break",IN_BreakDown);
	gEngfuncs.pfnAddCommand ("-break",IN_BreakUp);

	lookstrafe			= gEngfuncs.pfnRegisterVariable ( "lookstrafe", "0", FCVAR_ARCHIVE );
	lookspring			= gEngfuncs.pfnRegisterVariable ( "lookspring", "0", FCVAR_ARCHIVE );
	cl_anglespeedkey	= gEngfuncs.pfnRegisterVariable ( "cl_anglespeedkey", "0.67", 0 );
	cl_yawspeed			= gEngfuncs.pfnRegisterVariable ( "cl_yawspeed", "210", 0 );
	cl_pitchspeed		= gEngfuncs.pfnRegisterVariable ( "cl_pitchspeed", "225", 0 );
	cl_upspeed			= gEngfuncs.pfnRegisterVariable ( "cl_upspeed", "320", 0 );
	cl_forwardspeed		= gEngfuncs.pfnRegisterVariable ( "cl_forwardspeed", "400", FCVAR_ARCHIVE );
	cl_backspeed		= gEngfuncs.pfnRegisterVariable ( "cl_backspeed", "400", FCVAR_ARCHIVE );
	cl_sidespeed		= gEngfuncs.pfnRegisterVariable ( "cl_sidespeed", "400", 0 );
	cl_movespeedkey		= gEngfuncs.pfnRegisterVariable ( "cl_movespeedkey", "0.3", 0 );
	cl_pitchup			= gEngfuncs.pfnRegisterVariable ( "cl_pitchup", "89", 0 );
	cl_pitchdown		= gEngfuncs.pfnRegisterVariable ( "cl_pitchdown", "89", 0 );

	cl_vsmoothing		= gEngfuncs.pfnRegisterVariable ( "cl_vsmoothing", "0.05", FCVAR_ARCHIVE );

	m_pitch			    = gEngfuncs.pfnRegisterVariable ( "m_pitch","0.022", FCVAR_ARCHIVE );
	m_yaw				= gEngfuncs.pfnRegisterVariable ( "m_yaw","0.022", FCVAR_ARCHIVE );
	m_forward			= gEngfuncs.pfnRegisterVariable ( "m_forward","1", FCVAR_ARCHIVE );
	m_side				= gEngfuncs.pfnRegisterVariable ( "m_side","0.8", FCVAR_ARCHIVE );

	// Initialize third person camera controls.
	CAM_Init();
	// Initialize inputs
	IN_Init();
	// Initialize keyboard
	KB_Init();
	// Initialize view system
	V_Init();
}

/*
============
ShutdownInput
============
*/
void ShutdownInput (void)
{
	IN_Shutdown();
	KB_Shutdown();
}

void DLLEXPORT HUD_Shutdown( void )
{
	ShutdownInput();
}
