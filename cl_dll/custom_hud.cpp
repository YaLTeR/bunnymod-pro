#include <vector>
#include <string.h>

#include "hud.h"
#include "cl_util.h"
#include "demo_api.h"
#include "parsemsg.h"

#define JUMPSPEED_FADE_TIME 7

#define DAMAGE_MOVE_TIME	1

const char BUNNYMOD_VERSION[5] = "v1.1";

char m_sEntityName[32];
char m_sEntityModel[32];

DECLARE_MESSAGE( m_CustomHud, EntHealth )
DECLARE_MESSAGE( m_CustomHud, EntInfo )
DECLARE_MESSAGE( m_CustomHud, EntFired )
DECLARE_MESSAGE( m_CustomHud, FireReset )

int CHudCustom::Init( void )
{
	HOOK_MESSAGE( EntHealth )
	HOOK_MESSAGE( EntInfo )
	HOOK_MESSAGE( EntFired )
	HOOK_MESSAGE( FireReset )

	m_iFlags = HUD_ACTIVE;
	m_fJumpspeedFadeGreen = 0;
	m_fJumpspeedFadeRed = 0;
	m_fDamageAnimTime = 0;
	m_fChargingTime = 0;
	m_bChargingHealth = false;
	m_fEntityHealth = 0.0;
	m_iNumFires = 0;

	g_iHealthDifference = 0;

	gHUD.AddHudElem( this );

	return 1;
}

int CHudCustom::VidInit( void )
{
	m_iNumFires = 0;

	static bool bPrintedVersion = false;
	if ( !bPrintedVersion )
	{
		bPrintedVersion = true;
		gEngfuncs.Con_Printf( "\nBunnymod Pro %s\n\n", BUNNYMOD_VERSION );
	}

	return 1;
}

extern cvar_t *hud_accuracy;
extern cvar_t *hud_speedometer, *hud_speedometer_pos;
extern cvar_t *hud_jumpspeed, *hud_jumpspeed_pos;
extern cvar_t *hud_zspeed, *hud_zspeed_pos;
extern cvar_t *hud_acceleration, *hud_acceleration_pos;
extern cvar_t *hud_origin_x, *hud_origin_x_pos;
extern cvar_t *hud_origin_y, *hud_origin_y_pos;
extern cvar_t *hud_origin_z, *hud_origin_z_pos;
extern cvar_t *hud_viewangle_x, *hud_viewangle_x_pos;
extern cvar_t *hud_viewangle_y, *hud_viewangle_y_pos;
extern cvar_t *hud_grenadetimer, *hud_grenadetimer_pos;
extern cvar_t *hud_grenadetimer_width, *hud_grenadetimer_height;
extern cvar_t *hud_demorec_counter, *hud_demorec_counter_pos;
extern cvar_t *hud_gaussboost, *hud_gaussboost_pos;
extern cvar_t *hud_gaussammo, *hud_gaussammo_pos;
extern cvar_t *hud_gausscharge, *hud_gausscharge_pos;
extern cvar_t *hud_speedinfo, *hud_speedinfo_pos;
extern cvar_t *hud_grenadetimer_dontchange_resetto;
extern cvar_t *hud_gaussboost_dontchange_resetto;
extern cvar_t *hud_damage, *hud_damage_size, *hud_damage_anim;
extern cvar_t *hud_entityhealth, *hud_entityhealth_pos;
extern cvar_t *hud_entityinfo, *hud_entityinfo_pos;
extern cvar_t *hud_firemon, *hud_firemon_pos;
extern cvar_t *hud_health_pos;
extern cvar_t *con_color;
extern vec3_t g_vel, g_org;
extern Vector g_vecViewAngle;
extern Vector g_vecPlayerAngle;
extern Vector g_vecPlayerVelocity;
extern double jspeed, jspeeddelta;
extern double g_acceleration;
extern bool g_acceleration_negative;
extern bool g_bPaused;
extern bool g_bDemorecChangelevel;
extern bool g_bResetDemorecCounter;
extern bool g_bGrenTimeReset;
extern float g_fGrenTime;
extern bool g_bTimer;
extern bool g_bGausscharge;
extern float g_fGaussStart;
extern float gaussboost_ammoConsumed;
extern bool g_bHoldingGaussCannon;
extern bool g_bGaussboostReset;

extern cvar_t *cl_bhopcap;
extern "C" unsigned char g_Bhopcap; // Monitor cl_bhopcap and change this accordingly.

double demorec_counter_delta = 0.0;
double demorec_delta;

float gaussboost_delta;
float gaussboost_damage;
Vector gaussboost_vecBoostWithCurrentViewangles;
Vector gaussboost_vecResultingSpeed;
Vector gaussboost_vecResultingSpeedBB;
float gaussboost_boostWithCurrentViewangles;
float gaussboost_boostWithOptimalViewangles;	
float gaussboost_resultingSpeed;
float gaussboost_resultingSpeedBB;
float gaussboost_resultingSpeedWithOptimalViewangles;
float gaussboost_resultingSpeedWithOptimalViewanglesBB;

float g_DefaultTextColor[3] = { 1.0f, (160.0f / 255.0f), 0.0f };

int recording = 0, oldrecording = 0;
int playingback = 0;

int CHudCustom::Draw( float fTime )
{
	int x = 0, y = 0;

	int sx = 0, sy = 0; // Speed counter
	
	int jr, jg, jb; // Jump speed
	int jx = 0, jy = 0;
	
	int zsx = 0, zsy = 0; // Z speed
	int ax = 0, ay = 0; // Acceleration
	int xx = 0, xy = 0; // X
	int yx = 0, yy = 0; // Y
	int zx = 0, zy = 0; // Z	
	int vxx = 0, vxy = 0; // Viewangle X	
	int vyx = 0, vyy = 0; // Viewangle Y	
	int gtx = 0, gty = 0; // Grenade Timer	
	int dx = 0, dy = 0; // Demorec counter
	
	bool accuracy = ( hud_accuracy->value || ( !strcmp(hud_accuracy->string, "quadrazid") ) );

	int HealthWidth = gHUD.GetSpriteRect(gHUD.m_HUD_number_0).right - gHUD.GetSpriteRect(gHUD.m_HUD_number_0).left;

	double speed = sqrt( (g_vel[0] * g_vel[0]) + (g_vel[1] * g_vel[1]) );

	if (con_color->string && con_color->string[0])
	{
		int cr = 0, cg = 0, cb = 0;

		sscanf(con_color->string, "%d %d %d", &cr, &cg, &cb);
		g_DefaultTextColor[0] = cr / 255.0f;
		g_DefaultTextColor[1] = cg / 255.0f;
		g_DefaultTextColor[2] = cb / 255.0f;
	}

    g_Bhopcap = (cl_bhopcap->value != 0.0f) ? 1 : 0;
	
	if (hud_speedometer->value)
	{
		x = ScreenWidth / 2 - HealthWidth / 2;
		y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;

		if (hud_speedometer_pos->string)
		{
			sscanf(hud_speedometer_pos->string, "%i %i", &sx, &sy);
		}

		if (accuracy)
		{
			DrawNumber(speed, x, y, sx, sy);
		}
		else
		{
			DrawNumber((int)speed, x, y, sx, sy);
		}
	}

	if (hud_jumpspeed->value)
	{	
		GetHudColor(jr, jg, jb);

		x = ScreenWidth / 2 - HealthWidth / 2;
		y = ScreenHeight - 3 * gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;

		if (hud_jumpspeed_pos->string)
		{
			sscanf(hud_jumpspeed_pos->string, "%i %i", &jx, &jy);
		}

		if (jspeeddelta > 0)
		{
			jspeeddelta = 0;
			m_fJumpspeedFadeGreen = JUMPSPEED_FADE_TIME;
			m_fJumpspeedFadeRed = 0;
		}
		else if (jspeeddelta < 0)
		{
			jspeeddelta = 0;
			m_fJumpspeedFadeGreen = 0;
			m_fJumpspeedFadeRed = JUMPSPEED_FADE_TIME;
		}

		if (m_fJumpspeedFadeGreen > 0)
		{
			m_fJumpspeedFadeGreen -= (gHUD.m_flTimeDelta * 20);

			if (m_fJumpspeedFadeGreen <= 0)
			{
				m_fJumpspeedFadeGreen = 0;
			}

			jr = jr - jr * (m_fJumpspeedFadeGreen / JUMPSPEED_FADE_TIME);
			jg = jg + (255 - jg) * (m_fJumpspeedFadeGreen / JUMPSPEED_FADE_TIME);
			jb = jb - jb * (m_fJumpspeedFadeGreen / JUMPSPEED_FADE_TIME);
		}
		else if (m_fJumpspeedFadeRed > 0)
		{
			m_fJumpspeedFadeRed -= (gHUD.m_flTimeDelta * 20);

			if (m_fJumpspeedFadeRed <= 0)
			{
				m_fJumpspeedFadeRed = 0;
			}
			
			jr = jr + (255 - jr) * (m_fJumpspeedFadeRed / JUMPSPEED_FADE_TIME);
			jg = jg - jg * (m_fJumpspeedFadeRed / JUMPSPEED_FADE_TIME);
			jb = jb - jb * (m_fJumpspeedFadeRed / JUMPSPEED_FADE_TIME);
		}

		if (accuracy)
		{
			DrawNumber(jspeed, x, y, jx, jy);
		}
		else
		{
			DrawNumber((int)jspeed, x, y, jx, jy, false, jr, jg, jb);
		}
	}

	double zspeed = g_vel[2];

	if (hud_zspeed->value)
	{
		x = ScreenWidth / 2 - HealthWidth / 2;
		y = ScreenHeight - 2 * gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;

		if (hud_zspeed_pos->string)
		{
			sscanf(hud_zspeed_pos->string, "%i %i", &zsx, &zsy);
		}

		if (accuracy)
		{
			DrawNumber(zspeed, x, y, zsx, zsy);
		}
		else
		{
			DrawNumber((int)zspeed, x, y, zsx, zsy);
		}
	}

	if (hud_acceleration->value)
	{
		x = ScreenWidth / 2 - HealthWidth / 2;
		y = ScreenHeight - 4 * gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;

		if (hud_acceleration_pos->string)
		{
			sscanf(hud_acceleration_pos->string, "%i %i", &ax, &ay);
		}

		if (accuracy)
		{
			if (g_acceleration_negative)
			{
				DrawNumber(-g_acceleration, x, y, ax, ay);
			}
			else
			{
				DrawNumber(g_acceleration, x, y, ax, ay);
			}
		}
		else
		{
			DrawNumber((int)g_acceleration, x, y, ax, ay, g_acceleration_negative);
		}
	}
	
	if (hud_origin_x->value)
	{
		double origin_x = g_org[0];
		
		x = 0;
		y = 0;

		if (hud_origin_x_pos->string)
		{
			sscanf(hud_origin_x_pos->string, "%i %i", &xx, &xy);
		}

		if (accuracy)
		{
			DrawNumber(origin_x, x, y, xx, xy);
		}
		else
		{
			DrawNumber((int)origin_x, x, y, xx, xy);
		}
	}
	
	if (hud_origin_y->value)
	{
		double origin_y = g_org[1];
	
		x = 0;
		y = gHUD.m_iFontHeight;

		if (hud_origin_y_pos->string)
		{
			sscanf(hud_origin_y_pos->string, "%i %i", &yx, &yy);
		}

		if (accuracy)
		{
			DrawNumber(origin_y, x, y, yx, yy);
		}
		else
		{
			DrawNumber((int)origin_y, x, y, yx, yy);
		}
	}
	
	if (hud_origin_z->value)
	{
		double origin_z = g_org[2];
	
		x = 0;
		y = 2 * gHUD.m_iFontHeight;

		if (hud_origin_z_pos->string)
		{
			sscanf(hud_origin_z_pos->string, "%i %i", &zx, &zy);
		}

		if (accuracy)
		{
			DrawNumber(origin_z, x, y, zx, zy);
		}
		else
		{
			DrawNumber((int)origin_z, x, y, zx, zy);
		}
	}
	
	if (hud_viewangle_x->value)
	{
		x = 0;
		y = 3.5 * gHUD.m_iFontHeight;
		
		char temp_viewangle_x[32];
		
		if ( !strcmp(hud_accuracy->string, "quadrazid") )
		{
			sprintf(temp_viewangle_x, "Viewangle X: %.15f", g_vecViewAngle[0]);
		}
		else
		{
			sprintf(temp_viewangle_x, "Viewangle X: %f", g_vecViewAngle[0]);
		}

		if (hud_viewangle_x_pos->string)
		{
			sscanf(hud_viewangle_x_pos->string, "%i %i", &vxx, &vxy);
		}

		DrawString(temp_viewangle_x, x, y, vxx, vxy);
	}
	
	if (hud_viewangle_y->value)
	{
		x = 0;
		y = 4.5 * gHUD.m_iFontHeight;
		
		char temp_viewangle_y[32];
		
		if ( !strcmp(hud_accuracy->string, "quadrazid") )
		{
			sprintf(temp_viewangle_y, "Viewangle Y: %.15f", g_vecViewAngle[1]);
		}
		else
		{
			sprintf(temp_viewangle_y, "Viewangle Y: %f", g_vecViewAngle[1]);
		}

		if (hud_viewangle_y_pos->string)
		{
			sscanf(hud_viewangle_y_pos->string, "%i %i", &vyx, &vyy);
		}

		DrawString(temp_viewangle_y, x, y, vyx, vyy);
	}

	// ===========
	// DEMO RECORD TIMER
	// ===========

	demorec_delta = gHUD.m_flTimeDelta;
	
	if (g_bDemorecChangelevel)
	{
		g_bDemorecChangelevel = false;
		demorec_delta = 0;
	}
	
	oldrecording = recording;
	recording = gEngfuncs.pDemoAPI->IsRecording();
	playingback = gEngfuncs.pDemoAPI->IsPlayingback();
	
	if (oldrecording != recording && !oldrecording)
	{
		demorec_counter_delta = 0.0;
	}
	
	if (g_bResetDemorecCounter)
	{
		demorec_delta = 0.0;
		demorec_counter_delta = 0.0;
		g_bResetDemorecCounter = false;
	}
	
	if ((recording || playingback) && !g_bPaused)
	{
		demorec_counter_delta += demorec_delta;
	}

	if (hud_demorec_counter->value)
	{		
		x = 0;
		y = ScreenHeight / 2;
		
		if (hud_demorec_counter_pos->string)
		{
			sscanf(hud_demorec_counter_pos->string, "%i %i", &dx, &dy);
		}
		
		DrawNumber(demorec_counter_delta, x, y, dx, dy);
	}
	
	// ===========
	// END DEMO RECORD TIMER
	// ===========
	// GRENADE TIMER
	// ===========
	
	if (g_bGrenTimeReset)
	{
		g_bGrenTimeReset = false;
		
		if (hud_grenadetimer_dontchange_resetto->value != -1)
		{		
			g_fGrenTime = gEngfuncs.GetClientTime() - hud_grenadetimer_dontchange_resetto->value + 0.1;
			gEngfuncs.Cvar_SetValue("hud_grenadetimer_dontchange_resetto", -1);
			gEngfuncs.Con_Printf("curtime %f\n", fTime);
			gEngfuncs.Con_Printf("grentime set to %f\n", g_fGrenTime);
		}
		else
		{
			g_fGrenTime = gEngfuncs.GetClientTime();
		}
	}
	
	float m_flGrenTimerDelta = fTime - g_fGrenTime;
	
	if (m_flGrenTimerDelta <= 3.1 && m_flGrenTimerDelta > 0)
	{
		g_bTimer = true;
	}
	else
	{
		g_bTimer = false;
	}
	
	if (g_bTimer && hud_grenadetimer->value)
	{		
		x = ScreenWidth / 2;
		y = 0;
		
		int m_iGrenTimerOut = (int) ceil(100 * (3.1 - m_flGrenTimerDelta));
		
		if (hud_grenadetimer_pos->string)
		{
			sscanf(hud_grenadetimer_pos->string, "%i %i", &gtx, &gty);
		}
		
		x = DrawNumber(m_iGrenTimerOut, x, y, gtx, gty);
		
		x += gtx;
		y -= gty;
		
		x += HealthWidth / 2;
		
		FillRGBA(x, y, hud_grenadetimer_width->value, hud_grenadetimer_height->value, 0, 255, 0, 128);		
		
		int y2 = (hud_grenadetimer_height->value * m_iGrenTimerOut) / 310;
		y += hud_grenadetimer_height->value - (hud_grenadetimer_height->value * m_iGrenTimerOut) / 310;
		FillRGBA(x, y, hud_grenadetimer_width->value, y2, 255, 255, 0, 128);
	}
	
	// ===========
	// END GRENADE TIMER
	// ===========
	// GAUSS BOOST INFO
	// ===========

	if (g_bGaussboostReset)
	{
		g_bGaussboostReset = false;
		
		if (hud_gaussboost_dontchange_resetto->value != -1)
		{		
			g_fGaussStart = gEngfuncs.GetClientTime() - hud_gaussboost_dontchange_resetto->value + 0.1;
			gEngfuncs.Cvar_SetValue("hud_gaussboost_dontchange_resetto", -1);
			gEngfuncs.Con_Printf("curtime %f\n", fTime);
			gEngfuncs.Con_Printf("gaussboost time set to %f\n", g_fGaussStart);
		}
		else
		{
			g_fGaussStart = gEngfuncs.GetClientTime();
			gEngfuncs.Con_Printf("curtime %f\n", fTime);
			gEngfuncs.Con_Printf("gaussboost time set to %f\n", g_fGaussStart);
		}
	}

	if (g_bGausscharge)
	{
		gaussboost_delta = fTime - g_fGaussStart;
		gaussboost_damage = (gaussboost_delta > 4) ? 200 : 200 * (gaussboost_delta / 4);

		Vector forward, right, up;
		AngleVectors(g_vecPlayerAngle, forward, right, up);
		gaussboost_vecBoostWithCurrentViewangles = -forward * gaussboost_damage * 5;
		gaussboost_boostWithOptimalViewangles = gaussboost_damage * 5;
		gaussboost_boostWithCurrentViewangles = sqrt(gaussboost_vecBoostWithCurrentViewangles[0] * gaussboost_vecBoostWithCurrentViewangles[0] + gaussboost_vecBoostWithCurrentViewangles[1] * gaussboost_vecBoostWithCurrentViewangles[1]);
		
		gaussboost_vecResultingSpeed = g_vecPlayerVelocity + gaussboost_vecBoostWithCurrentViewangles;
		gaussboost_vecResultingSpeedBB = g_vecPlayerVelocity - gaussboost_vecBoostWithCurrentViewangles;
		gaussboost_resultingSpeed = sqrt(gaussboost_vecResultingSpeed[0] * gaussboost_vecResultingSpeed[0] + gaussboost_vecResultingSpeed[1] * gaussboost_vecResultingSpeed[1]);
		gaussboost_resultingSpeedBB = sqrt(gaussboost_vecResultingSpeedBB[0] * gaussboost_vecResultingSpeedBB[0] + gaussboost_vecResultingSpeedBB[1] * gaussboost_vecResultingSpeedBB[1]);
		gaussboost_resultingSpeedWithOptimalViewangles = sqrt(g_vecPlayerVelocity[0] * g_vecPlayerVelocity[0] + g_vecPlayerVelocity[1] * g_vecPlayerVelocity[1]) - gaussboost_damage * 5;;
		gaussboost_resultingSpeedWithOptimalViewanglesBB = sqrt(g_vecPlayerVelocity[0] * g_vecPlayerVelocity[0] + g_vecPlayerVelocity[1] * g_vecPlayerVelocity[1]) + gaussboost_damage * 5;;
	}

	if (hud_gaussboost->value && g_bHoldingGaussCannon)
	{
		int dx = 0, dy = 0;

		x = ScreenWidth / 4;
		y = 0;

		if (hud_gaussboost_pos->string)
		{
			sscanf(hud_gaussboost_pos->string, "%i %i", &dx, &dy);
		}

		char temp[255] = "== Gauss boost info ==";
		DrawString(temp, x, y, dx, dy);

		if ( hud_gaussboost->value == 2 )
		{
			y += gHUD.m_iFontHeight;
			sprintf(temp, "Boost with current viewangles: %f", gaussboost_boostWithCurrentViewangles);
			DrawString(temp, x, y, dx, dy);
		}

		y += gHUD.m_iFontHeight;
		sprintf(temp, "Boost with optimal viewangles: %f", gaussboost_boostWithOptimalViewangles);
		DrawString(temp, x, y, dx, dy);

		if ( hud_gaussboost->value == 2 )
		{
			y += gHUD.m_iFontHeight;
			sprintf(temp, "Resulting speed: %f", gaussboost_resultingSpeed);
			DrawString(temp, x, y, dx, dy);

			y += gHUD.m_iFontHeight;
			sprintf(temp, "Resulting speed [back boost]: %f", gaussboost_resultingSpeedBB);
			DrawString(temp, x, y, dx, dy);

			y += gHUD.m_iFontHeight;
			sprintf(temp, "Optimal resulting speed: %f", gaussboost_resultingSpeedWithOptimalViewangles);
			DrawString(temp, x, y, dx, dy);
		}

		y += gHUD.m_iFontHeight;
		sprintf(temp, "Optimal resulting speed [back boost]: %f", gaussboost_resultingSpeedWithOptimalViewanglesBB);
		DrawString(temp, x, y, dx, dy);

		y += gHUD.m_iFontHeight;
		sprintf(temp, "Ammo consumed: %.2f", gaussboost_ammoConsumed);
		DrawString(temp, x, y, dx, dy);
	}

	if ( hud_gaussammo->value && g_bHoldingGaussCannon )
	{
		int dx = 0, dy = 0;

		x = ScreenWidth / 2;
		y = ScreenHeight / 2 - gHUD.m_iFontHeight;

		if (hud_gaussammo_pos->string)
		{
			sscanf(hud_gaussammo_pos->string, "%d %d", &dx, &dy);
		}

		char temp[10];
		sprintf(temp, "%.2f", gaussboost_ammoConsumed);
		DrawString(temp, x, y, dx, dy);
	}

	// ===========
	// END GAUSS BOOST INFO
	// ===========
	// SPEED INFO
	// ===========

	if (hud_speedinfo->value)
	{
		int dx, dy = 0;
		x = 0.75 * ScreenWidth;
		y = 0;

		if (hud_speedinfo_pos->string)
		{
			sscanf(hud_speedinfo_pos->string, "%i %i", &dx, &dy);
		}

		char temp[255] = "== Speed info ==";
		DrawString(temp, x, y, dx, dy);

		y += gHUD.m_iFontHeight;
		sprintf(temp, "X: %f", g_vel[0]);
		DrawString(temp, x, y, dx, dy);

		y += gHUD.m_iFontHeight;
		sprintf(temp, "Y: %f", g_vel[1]);
		DrawString(temp, x, y, dx, dy);

		y += gHUD.m_iFontHeight;
		sprintf(temp, "Z: %f", g_vel[2]);
		DrawString(temp, x, y, dx, dy);

		y += gHUD.m_iFontHeight;
		sprintf(temp, "XY: %f", sqrt(g_vel[0] * g_vel[0] + g_vel[1] * g_vel[1]) );
		DrawString(temp, x, y, dx, dy);

		y += gHUD.m_iFontHeight;
		sprintf( temp, "XYZ: %f", sqrt(g_vel[0] * g_vel[0] + g_vel[1] * g_vel[1] + g_vel[2] * g_vel[2]) );
		DrawString(temp, x, y, dx, dy);
	}

	// ===========
	// END SPEED INFO
	// ===========
	// DAMAGE INDICATOR
	// ===========

	int maxSize = ( hud_damage_size->value > 0 ) ? hud_damage_size->value : 1;

	if ( hud_damage_anim->value )
	{
		maxSize++;
	}

	if ( m_ivDamage.size() > maxSize )
	{
		m_ivDamage.erase( m_ivDamage.begin() + maxSize, m_ivDamage.end() );
	}

	if ( hud_damage->value )
	{
		int healthY = ScreenHeight - 2 * gHUD.m_iFontHeight;

		int HealthWidth = gHUD.GetSpriteRect(gHUD.m_HUD_number_0).right - gHUD.GetSpriteRect(gHUD.m_HUD_number_0).left;
		int CrossWidth = gHUD.GetSpriteRect(gHUD.m_Health.m_HUD_cross).right - gHUD.GetSpriteRect(gHUD.m_Health.m_HUD_cross).left;
		int healthX = CrossWidth + HealthWidth / 2;

		int dx = 0, dy = 0;

		bool isNegative = false;

		if ( hud_health_pos->string )
		{
			sscanf( hud_health_pos->string, "%i %i", &dx, &dy );
		}

		float a = 0, v0 = 0, sf = 0, s = 0, t = 0;

		if ( m_fDamageAnimTime > 0 )
		{
			t = DAMAGE_MOVE_TIME - m_fDamageAnimTime;
			v0 = ( ( 2 * gHUD.m_iFontHeight ) / DAMAGE_MOVE_TIME ); // v0 = 2h / n
			a = -1 * ( v0 / DAMAGE_MOVE_TIME ); // a = -2h / (n*n)
			s = v0 * t + ( ( a * t * t ) / 2 ); // s changes from 0 to h
			sf = gHUD.m_iFontHeight;
		}

		for ( int i = 0; i < m_ivDamage.size(); ++i )
		{
			healthY -= gHUD.m_iFontHeight;

			float currentY = healthY;
			
			if ( m_ivDamage[i] < 0 )
			{
				isNegative = true;
			}

			int r = 255;
			int g = 0;

			if ( !isNegative )
			{
				r = 0;
				g = 255;
			}

			if ( m_fDamageAnimTime > 0 )
			{
				currentY += gHUD.m_iFontHeight - s;

				if ( i == 0 )
				{
					r *= ( s / sf );
					g *= ( s / sf );
				}
				else if ( i == ( maxSize - 1 ) )
				{
					r *= ( 1 - ( s / sf ) );
					g *= ( 1 - ( s / sf ) );
				}
			}

			if ( !( i == ( maxSize - 1 ) && m_fDamageAnimTime == 0 && hud_damage_anim->value ) )
			{
				DrawNumber( m_ivDamage[i], healthX, currentY, dx, dy, false, r, g, 0, true);
			}

			isNegative = false;
		}

		if ( m_fDamageAnimTime > 0 )
		{
			m_fDamageAnimTime -= gHUD.m_flTimeDelta;
		
			if ( m_fDamageAnimTime < 0 )
			{
				m_fDamageAnimTime = 0;
			}
		}

		if ( m_fChargingTime > 0 )
		{
			m_fChargingTime -= gHUD.m_flTimeDelta;
		
			if ( m_fChargingTime < 0 )
			{
				m_fChargingTime = 0;
			}
		}
	}

	// ===========
	// END DAMAGE INDICATOR
	// ===========
	// ENTITY INFO
	// ===========

	if ( hud_entityhealth->value )
	{
		int ehx = 0, ehy = 0;

		if ( hud_entityhealth_pos->string )
		{
			sscanf( hud_entityhealth_pos->string, "%d %d", &ehx, &ehy );
		}

		DrawNumber( m_fEntityHealth, 0, 6 * gHUD.m_iFontHeight, ehx, ehy );
	}

	if ( hud_entityinfo->value )
	{
		int eix = 0, eiy = 0;

		if ( hud_entityinfo_pos->string )
		{
			sscanf( hud_entityinfo_pos->string, "%d %d", &eix, &eiy );
		}

		char temp[256];
		sprintf( temp, "== Entity Info ==" );
		DrawString( temp, 0, 7 * gHUD.m_iFontHeight, eix, eiy );

		sprintf( temp, "Classname: %s", m_sEntityName );
		DrawString( temp, 0, 8 * gHUD.m_iFontHeight, eix, eiy );

		sprintf( temp, "Model: %s", m_sEntityModel );
		DrawString( temp, 0, 9 * gHUD.m_iFontHeight, eix, eiy );
	}

	// ===========
	// END ENTITY INFO
	// ===========
	// FIRE MONITOR
	// ===========

	if ( hud_firemon->value )
	{
		int numberWidth = gHUD.GetSpriteRect( gHUD.m_HUD_number_0 ).right - gHUD.GetSpriteRect( gHUD.m_HUD_number_0 ).left;
		int fmx = 0, fmy = 0;

		if ( hud_firemon_pos->string )
		{
			sscanf( hud_firemon_pos->string, "%d %d", &fmx, &fmy );
		}

		DrawNumber( m_iNumFires, ScreenWidth - numberWidth * 3, ScreenHeight / 2, fmx, fmy );
	}

	// ===========
	// END FIRE MONITOR
	// ===========

	return 1;
}

extern cvar_t *hud_damage_enable;

void CHudCustom::HealthChanged( int delta )
{
	if ( !hud_damage_enable->value )
	{
		return;
	}

	if ( delta == 1 )
	{
		if ( m_bChargingHealth )
		{
			if ( m_fChargingTime > 0 )
			{
				m_fChargingTime = 1;
				m_ivDamage[0]++;
				return;
			}
			else
			{
				m_fChargingTime = 1;
			}
		}
		else
		{
			m_bChargingHealth = true;
			m_fChargingTime = 1;
		}
	}
	else
	{
		m_bChargingHealth = false;
	}

	int maxSize = ( hud_damage_size->value > 0 ) ? hud_damage_size->value : 1;

	if ( hud_damage_anim->value )
	{
		maxSize++;
	}

	m_ivDamage.insert( m_ivDamage.begin(), delta );

	if ( m_ivDamage.size() > maxSize )
	{
		m_ivDamage.pop_back();
	}

	if ( hud_damage_anim->value )
	{
		m_fDamageAnimTime = DAMAGE_MOVE_TIME;
	}
}

void CHudCustom::DamageHistoryReset( void )
{
	m_ivDamage.erase( m_ivDamage.begin(), m_ivDamage.end() );

	m_fDamageAnimTime = 0;
	m_fChargingTime = 0;
	m_bChargingHealth = false;
}

void CHudCustom::HealthDifference( void )
{
	if ( gEngfuncs.Cmd_Argc() != 2 )
	{
		gEngfuncs.Con_Printf( "Usage: hud_health_difference <difference>\n" );
		gEngfuncs.Con_Printf( "Current difference: %d\n", g_iHealthDifference );
		return;
	}

	int iNewHealthDifference = 0;
	sscanf( gEngfuncs.Cmd_Argv( 1 ), "%d", &iNewHealthDifference );

	if ( iNewHealthDifference != g_iHealthDifference )
	{
		int iDelta = iNewHealthDifference - g_iHealthDifference;

		g_iHealthDifference = iNewHealthDifference;		
		gHUD.m_Health.m_iHealth += iDelta;

		HealthChanged( iDelta );
	}
}

int CHudCustom::MsgFunc_EntHealth( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	m_fEntityHealth = READ_FLOAT();

	return 1;
}

int CHudCustom::MsgFunc_EntInfo( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	strcpy( m_sEntityName, READ_STRING() );
	strcpy( m_sEntityModel, READ_STRING() );

	return 1;
}

int CHudCustom::MsgFunc_EntFired( const char *pszName, int iSize, void *pbuf )
{
	m_iNumFires++;

	return 1;
}

int CHudCustom::MsgFunc_FireReset( const char *pszName, int iSize, void *pbuf )
{
	m_iNumFires = 0;

	return 1;
}

extern cvar_t *hud_alpha;
extern cvar_t *hud_pos_percent;

int CHudCustom::DrawNumber( int number, int x, int y, int dx, int dy, bool isNegative, int r, int g, int b, bool transparent )
{
	int a;
	
	if ( r == 0 && g == 0 && b == 0 && !transparent )
	{
		if ( isNegative || number < 0 )
		{
			r = 255;
			g = 0;
			b = 0;
		}
		else
		{
			GetHudColor( r, g, b );
		}
	}
	
	if ( number < 0 )
	{
		number = -1 * number;
	}
	
	if ( hud_alpha->string && hud_alpha->string[0] )
	{
		if ( strcmp(hud_alpha->string, "auto") )
		{
			a = hud_alpha->value;

			if ( a > 255 )	a = 255;
			if ( a < 1 )	a = 1;
		}
		else
		{
			a = 255;
		}
	}
	else
	{
		a = 255;
	}
	
	ScaleColors( r, g, b, a );
	
	if ( hud_pos_percent->value )
	{
		return gHUD.DrawHudNumber( dx > 100 ? ScreenWidth : ( dx * ScreenWidth ) / 100, dy > 100 ? ScreenHeight : ( dx * ScreenHeight ) / 100, DHN_3DIGITS | DHN_DRAWZERO, number, r, g, b );
	}
	else
	{
		return gHUD.DrawHudNumber( x + dx, y - dy, DHN_3DIGITS | DHN_DRAWZERO, number, r, g, b );
	}
}

int CHudCustom::DrawNumber( double number, int x, int y, int dx, int dy, float r, float g, float b )
{
	char temp[255];
	int ret;

	if (r == -1.0f)
	{
		gEngfuncs.pfnDrawSetTextColor( g_DefaultTextColor[0], g_DefaultTextColor[1], g_DefaultTextColor[2] );
	}
	else
	{
		gEngfuncs.pfnDrawSetTextColor( r, g, b );
	}

	if (!strcmp( hud_accuracy->string, "quadrazid" ))
	{
		sprintf( temp, "%.15f", number );
	}
	else
	{
		sprintf( temp, "%f", number );
	}

	if (hud_pos_percent->value)
	{
		ret = gEngfuncs.pfnDrawConsoleString( dx > 100 ? ScreenWidth : (dx * ScreenWidth) / 100, dy > 100 ? ScreenHeight : (dx * ScreenHeight) / 100, temp );
	}
	else
	{
		ret = gEngfuncs.pfnDrawConsoleString( x + dx, y - dy, temp );
	}

	gEngfuncs.pfnDrawSetTextColor( g_DefaultTextColor[0], g_DefaultTextColor[1], g_DefaultTextColor[2] );

	return ret;
}

int CHudCustom::DrawString( char *stringToDraw, int x, int y, int dx, int dy, float r, float g, float b )
{
	int ret;

	if (r == -1.0f)
	{
		gEngfuncs.pfnDrawSetTextColor( g_DefaultTextColor[0], g_DefaultTextColor[1], g_DefaultTextColor[2] );
	}
	else
	{
		gEngfuncs.pfnDrawSetTextColor( r, g, b );
	}

	if (hud_pos_percent->value)
	{
		ret = gEngfuncs.pfnDrawConsoleString( dx > 100 ? ScreenWidth : (dx * ScreenWidth) / 100, dy > 100 ? ScreenHeight : (dx * ScreenHeight) / 100, stringToDraw );
	}
	else
	{
		ret = gEngfuncs.pfnDrawConsoleString( x + dx, y - dy, stringToDraw );
	}

	gEngfuncs.pfnDrawSetTextColor( g_DefaultTextColor[0], g_DefaultTextColor[1], g_DefaultTextColor[2] );

	return ret;
}