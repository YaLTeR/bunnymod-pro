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

// YaLTeR Start
const double M_PI = 3.14159265358979323846;  // matches value in gcc v2 math.h
const double M_RAD2DEG = 180 / M_PI;
const double M_DEG2RAD = M_PI / 180;

const double M_U = 360.0 / 65536;

// To be changed - just so it compiles.
bool g_bPerfectstrafe = false;
bool g_bAutostrafe = false;
bool g_bInBhop = false;
bool g_bGroundduck = false;
bool g_bClAutojump = false;
float setYaw = 0;
float setPitch = 0;
extern vec3_t g_vel, g_org;
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

// YaLTeR Start - TAS functions

inline double copysign(double value, double sign)
{
	if (sign >= 0)
		return abs(value);
	else
		return -abs(value);
}

double normangle(double angle)
{
	while (angle >= 180)
		angle -= 360;
	while (angle < -180)
		angle += 360;

	return angle;
}

double normangleengine(double angle)
{
	while (angle >= 360)
		angle -= 360;
	while (angle < 0)
		angle += 360;

	return angle;
}

void PM_Math_AngleVectors(const vec3_t &angles, vec3_t &forward, vec3_t &right, vec3_t &up)
{
	float angle;
	float sr, sp, sy, cr, cp, cy;

	angle = angles[YAW] * (M_PI*2 / 360);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[PITCH] * (M_PI*2 / 360);
	sp = sin(angle);
	cp = cos(angle);
	angle = angles[ROLL] * (M_PI*2 / 360);
	sr = sin(angle);
	cr = cos(angle);

	if (forward)
	{
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;
	}
	if (right)
	{
		right[0] = (-1*sr*sp*cy+-1*cr*-sy);
		right[1] = (-1*sr*sp*sy+-1*cr*cy);
		right[2] = -1*sr*cp;
	}
	if (up)
	{
		up[0] = (cr*sp*cy+-sr*-sy);
		up[1] = (cr*sp*sy+-sr*cy);
		up[2] = cr*cp;
	}
}

void TAS_ConstructWishvel(const vec3_t &angles, float forwardmove, float sidemove, float upmove, float maxspeed, vec3_t *wishvel)
{
	vec3_t properangles;
	VectorCopy(angles, properangles);
	properangles[0] = normangle(anglemod(properangles[0]));
	properangles[1] = normangle(anglemod(properangles[1]));
	properangles[2] = normangle(anglemod(properangles[2]));

	vec3_t forward, right, up;
	PM_Math_AngleVectors(properangles, forward, right, up);

	forward[2] = 0;
	right[2] = 0;
	VectorNormalize(forward);
	VectorNormalize(right);

	// CheckParamters
	float spd = (forwardmove * forwardmove) + (sidemove * sidemove) + (upmove * upmove);
	spd = sqrt(spd);
	if ((spd != 0) && (spd > maxspeed))
	{
		float fRatio = maxspeed / spd;
		forwardmove *= fRatio;
		sidemove *= fRatio;
		upmove *= fRatio;
	}

	if (wishvel)
	{
		(*wishvel)[0] = forwardmove * forward[0] + sidemove * right[0];
		(*wishvel)[1] = forwardmove * forward[1] + sidemove * right[1];
		(*wishvel)[2] = 0; // TBD when needed.
	}

	if (CVAR_GET_FLOAT("tas_log") != 0)
	{
		gEngfuncs.Con_Printf("-- TAS_ConstructWishvel Start --\n");
		gEngfuncs.Con_Printf("Angles: %.8f; %.8f; %.8f\n", angles[0], angles[1], angles[2]);
		gEngfuncs.Con_Printf("Properangles: %.8f; %.8f; %.8f\n", properangles[0], properangles[1], properangles[2]);
		gEngfuncs.Con_Printf("Forward: %.8g; %.8g; %.8g; right: %.8g; %.8g; %.8g\n", forward[0], forward[1], forward[2], right[0], right[1], right[2]);
		gEngfuncs.Con_Printf("-- TAS_ConstructWishvel End --\n");
	}
}

// Predicts the next origin and velocity as if we were in an empty world.
// Alpha is the angle between current velocity and acceleration (wishspeed).
// Again, we pass double-pointers, which is unnecessary, for the sake of code cleanness.
void TAS_SimplePredict(const vec3_t &wishvelocity, const vec3_t &velocity, const vec3_t &origin,
	double maxspeed, double accel, double wishspeed_cap, double frametime, double pmove_friction,
	double gravity, double pmove_gravity,
	vec3_t *new_velocity, vec3_t *new_origin)
{
	int i;

	vec3_t newvel, newpos;
	VectorCopy(velocity, newvel);
	VectorCopy(origin, newpos);

	vec3_t wishvel, wishdir;
	VectorCopy(wishvelocity, wishvel); // So that we can modify it without messing up anything outside.
	VectorCopy(wishvel, wishdir);
	float wishspeed = VectorNormalize(wishdir);

	if (CVAR_GET_FLOAT("tas_log") != 0)
	{
		double vel_angle = atan2(velocity[1], velocity[0]) * M_RAD2DEG;
		double wishangle = atan2(wishdir[1], wishdir[0]) * M_RAD2DEG;
		double speed = hypot(velocity[0], velocity[1]);
		double alpha = normangle(wishangle - vel_angle);

		if (speed == 0 || wishspeed == 0)
			alpha = 0;

		gEngfuncs.Con_Printf("-- TAS_SimplePredict Start --\n");
		gEngfuncs.Con_Printf("Velocity: %.8f; %.8f; %.8f; origin: %.8f; %.8f; %.8f\n", velocity[0], velocity[1], velocity[2], origin[0], origin[1], origin[2]);
		gEngfuncs.Con_Printf("Velocity angle: %.8g; wishangle: %.8g; Alpha: %.8g\n", vel_angle, wishangle, alpha);
		gEngfuncs.Con_Printf("Wishvel[0]: %.8f; wishvel[1]: %.8f; wishdir[0]: %.8f; wishdir[1]: %.8f\n", wishvel[0], wishvel[1], wishdir[0], wishdir[1]);
		gEngfuncs.Con_Printf("frametime: %f; maxspeed: %f; accel: %f; wishspeed: %.8g; wishspeed_cap: %f; pmove_friction: %f\n", frametime, maxspeed, accel, wishspeed, wishspeed_cap, pmove_friction);
		gEngfuncs.Con_Printf("Gravity: %f; pmove_gravity: %f\n", gravity, pmove_gravity);
	}

	// AddCorrectGravity
	float ent_gravity;
	if (pmove_gravity != 0)
		ent_gravity = pmove_gravity;
	else
		ent_gravity = 1;

	newvel[2] -= ent_gravity * gravity * 0.5 * frametime;

	// Move
	if (wishspeed > maxspeed)
	{
		VectorScale(wishvel, (maxspeed / wishspeed), wishvel); // Not really used anywhere besides PM_NoClip, but let's just have it here.
		wishspeed = maxspeed;
	}

	// Accelerate
	float wishspd = wishspeed;
	if (wishspd > wishspeed_cap)
		wishspd = wishspeed_cap;

	float currentspeed = DotProduct(velocity, wishdir);
	float addspeed = wishspd - currentspeed;
	if (addspeed > 0)
	{
		float A = accel * wishspeed * frametime * pmove_friction;
		if (A > addspeed)
			A = addspeed;

		for (i = 0; i < 3; i++)
		{
			newvel[i] += A * wishdir[i];
		}
	}

	// FlyMove
	for (i = 0; i < 3; i++)
	{
		newpos[i] += frametime * newvel[i];
	}

	// FixupGravityVelocity
	newvel[2] -= ent_gravity * gravity * 0.5 * frametime;

	// Return values if needed.
	if (new_velocity)
		VectorCopy(newvel, *new_velocity);

	if (new_origin)
		VectorCopy(newpos, *new_origin);

	if (CVAR_GET_FLOAT("tas_log") != 0)
	{
		gEngfuncs.Con_Printf("New velocity: %.8f; %.8f; %.8f; new origin: %.8f; %.8f; %.8f\n", newvel[0], newvel[1], newvel[2], newpos[0], newpos[1], newpos[2]);
		gEngfuncs.Con_Printf("-- TAS_SimplePredict End --\n");
	}
}

// Level 3 functions - TAS_Get*Angle
// Return the plain desired angle between velocity and acceleration without anglemod.
double TAS_GetMaxSpeedAngle(double speed, double maxspeed, double accel, double wishspeed, double wishspeed_cap, double frametime, double pmove_friction)
{
	if (wishspeed > maxspeed)
		wishspeed = maxspeed;

	double A = accel * wishspeed * frametime * pmove_friction;

	if (wishspeed > wishspeed_cap)
		wishspeed = wishspeed_cap;

	if (A >= 0)
	{
		if (speed == 0)
			return 0;

		double alphacos = (wishspeed - A) / speed;
		if (alphacos >= 1) return 0;
		if (alphacos <= 0) return 90;

		return (acos(alphacos) * M_RAD2DEG);
	}
	else // Negative acceleration.
	{
		/* Stuff is simpler here - since A is less than 0, it is always less than
		   addspeed, that means that speed after PM_AirAccelerate will equal to
		   sqrt(speed^2 + A^2 + 2*speed*A*cos(alpha)), so for the highest final
		   speed we want cos(alpha) = -1, or alpha = 180. */

		return 180;
	}
}

double TAS_GetMaxRotationAngle(double speed, double maxspeed, double accel, double wishspeed, double wishspeed_cap, double frametime, double pmove_friction)
{
	if (wishspeed > maxspeed)
		wishspeed = maxspeed;

	double A = accel * wishspeed * frametime * pmove_friction;

	if (wishspeed > wishspeed_cap) // Not really used here.
		wishspeed = wishspeed_cap;

	/* If A is less than 0 here then we'll add speed in an opposite to wishdir
	   direction, so the angle will be (180 - normal max rotation angle),
	   which we see here: arccos(-x) = pi - arccos(x).*/

	if (speed == 0)
	{
		if (A >= 0)
			return 180;
		else
			return 0;
	}

	double alphacos = (-A) / speed;
	if (alphacos >= 1) return 0;
	if (alphacos <= -1) return 180;

	return (acos(alphacos) * M_RAD2DEG);
}

double TAS_GetLeastSpeedAngle(double speed, double maxspeed, double accel, double wishspeed, double wishspeed_cap, double frametime, double pmove_friction)
{
	if (wishspeed > maxspeed)
		wishspeed = maxspeed;

	double A = accel * wishspeed * frametime * pmove_friction;

	if (wishspeed > wishspeed_cap)
		wishspeed = wishspeed_cap;

	if (A >= 0)
	{
		// Assume angle = 180 first.
		double alphacos = -1;
		double finalspeed_squared = speed*speed + A*A + (2 * speed * A * alphacos);
		if (finalspeed_squared >= 0)
			return 180;

		alphacos = -((speed*speed + A*A) / (2 * speed * A));
		return (acos(alphacos) * M_RAD2DEG);
	}
	else
	{
		/* Alpha = 0 would not work here if speed is > wishspeed because
		   of the addspeed <= 0 check that leads to return.
		   Alpha = acos(wishspeed / speed) would POTENTIALLY not work
		   because of the said check. This case needs to be handled in
		   a function ontop of this one. */

		double alphacos = (wishspeed / speed);
		if (alphacos >= 1) alphacos = 1;

		double finalspeed_squared = speed*speed + A*A + (2 * speed * A * alphacos);
		if (finalspeed_squared < 0)
		{
			alphacos = -((speed*speed + A*A) / (2 * speed * A));
		}

		return (acos(alphacos) * M_RAD2DEG);
	}
}

// Level 2 - TAS_Strafe*
// Return two angles - one is the desired difference between velocity and acceleration CCW to the velocity (leftangle),
// another - CW to the velocity (rightangle). Anglemods are here.
// Return value is a bool which is false if CCW is more desired and true if CW is more desired.

// const vec3_t &velocity is passing a double-pointer here, which is pretty much useless, but for the sake of cleanness.
bool TAS_StrafeMaxSpeed(const vec3_t &velocity,
	double maxspeed, double accel, double wishspeed, double wishspeed_cap, double frametime, double pmove_friction,
	double *leftangle, double *rightangle)
{
	double speed = hypot(velocity[0], velocity[1]);
	double alpha = TAS_GetMaxSpeedAngle(speed, maxspeed, accel, wishspeed, wishspeed_cap, frametime, pmove_friction);
	double vel_angle = atan2(velocity[1], velocity[0]) * M_RAD2DEG;

	double anglemod_diff_left = normangleengine(vel_angle + alpha) - anglemod(vel_angle + alpha);
	double anglemod_diff_right = normangleengine(vel_angle - alpha) - anglemod(vel_angle - alpha);
	double beta_left[2], beta_right[2];
	beta_left[0] = anglemod(vel_angle + alpha);
	beta_left[1] = beta_left[0] + copysign(M_U, anglemod_diff_left);
	beta_right[0] = anglemod(vel_angle - alpha);
	beta_right[1] = beta_right[0] + copysign(M_U, anglemod_diff_right);

	double alpha_left[2], alpha_right[2];
	alpha_left[0] = normangle(beta_left[0] - vel_angle);
	alpha_left[1] = normangle(beta_left[1] - vel_angle);
	alpha_right[0] = normangle(beta_right[0] - vel_angle);
	alpha_right[1] = normangle(beta_right[1] - vel_angle);

	double speed_max_right = 0;
	int i; // VC++6.0 compiler, pls...
	for (i = 0; i < 2; i++)
	{
		vec3_t newvel;

		// We don't care about position in this test.
		vec3_t pos;
		VectorClear(pos);

		// Assume we're strafing straight forward with pitch = 0 and roll = 0.
		vec3_t angles;
		VectorClear(angles);
		angles[1] = beta_right[i];

		vec3_t wishvel;
		TAS_ConstructWishvel(angles, wishspeed, 0, 0, maxspeed, &wishvel);

		TAS_SimplePredict(wishvel, velocity, pos,
			maxspeed, accel, wishspeed_cap, frametime, pmove_friction,
			0, 0,
			&newvel, NULL);

		double newspeed = hypot(newvel[0], newvel[1]);
		if (newspeed > speed_max_right)
		{
			speed_max_right = newspeed;
			if (rightangle)
			{
				*rightangle = alpha_right[i];
			}
		}
	}

	double speed_max_left = 0;
	for (i = 0; i < 2; i++)
	{
		vec3_t newvel;

		// We don't care about position in this test.
		vec3_t pos;
		VectorClear(pos);

		// Assume we're strafing straight forward with pitch = 0 and roll = 0.
		vec3_t angles;
		VectorClear(angles);
		angles[1] = beta_left[i];

		vec3_t wishvel;
		TAS_ConstructWishvel(angles, wishspeed, 0, 0, maxspeed, &wishvel);

		TAS_SimplePredict(wishvel, velocity, pos,
			maxspeed, accel, wishspeed_cap, frametime, pmove_friction,
			0, 0,
			&newvel, NULL);

		double newspeed = hypot(newvel[0], newvel[1]);
		if (newspeed > speed_max_left)
		{
			speed_max_left = newspeed;
			if (leftangle)
			{
				*leftangle = alpha_left[i];
			}
		}
	}

	return (speed_max_right >= speed_max_left);
}

// Level 1
// Returns the final desired angle between velocity and acceleration, anglemod'd.
// Negative if counter-clockwise, positive otherwise.
// TBD

// YaLTeR End

/*
================
CL_CreateMove

Send the intended movement message to the server
if active == 1 then we are 1) not playing back demos ( where our commands are ignored ) and
2 ) we have finished signing on to server
================
*/
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

		CL_AdjustAngles ( frametime, viewangles );

		memset (cmd, 0, sizeof(*cmd));

		gEngfuncs.SetViewAngles( (float *)viewangles );

		if ( in_strafe.state & 1 )
		{
			cmd->sidemove += cl_sidespeed->value * CL_KeyState (&in_right);
			cmd->sidemove -= cl_sidespeed->value * CL_KeyState (&in_left);
		}

		cmd->sidemove += cl_sidespeed->value * CL_KeyState (&in_moveright);
		cmd->sidemove -= cl_sidespeed->value * CL_KeyState (&in_moveleft);

		cmd->upmove += cl_upspeed->value * CL_KeyState (&in_up);
		cmd->upmove -= cl_upspeed->value * CL_KeyState (&in_down);

		if ( !(in_klook.state & 1 ) )
		{
			cmd->forwardmove += cl_forwardspeed->value * CL_KeyState (&in_forward);
			cmd->forwardmove -= cl_backspeed->value * CL_KeyState (&in_back);
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

		// YaLTeR Start - mess to test stuff.
		if (CVAR_GET_FLOAT("tas_log"))
			gEngfuncs.Con_Printf("\n");

		vec3_t curviewangles;
		gEngfuncs.GetViewAngles( (float *)curviewangles );

		vec3_t wishvel;
		TAS_ConstructWishvel(curviewangles, cl_forwardspeed->value, 0, 0, 320, &wishvel);
		double fpsbug_frametime = ((int)(frametime * 1000) / 1000.0);
		TAS_SimplePredict(wishvel, g_vel, g_org, 320, 10, 30, fpsbug_frametime, 1, 800, 1, NULL, NULL);

		if (CVAR_GET_FLOAT("tas_log"))
			gEngfuncs.Con_Printf("\n");
		// YaLTeR End
	}

	cmd->impulse = in_impulse;
	in_impulse = 0;

	cmd->weaponselect = g_weaponselect;
	g_weaponselect = 0;
	//
	// set button and flag bits
	//
	cmd->buttons = CL_ButtonBits( 1 );

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
