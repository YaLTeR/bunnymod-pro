/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// hud.cpp
//
// implementation of CHud class
//

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include "parsemsg.h"
#include "hud_servers.h"
#include "vgui_int.h"
#include "vgui_TeamFortressViewport.h"

#include "ruler.h"

#include "demo.h"
#include "demo_api.h"
#include "vgui_scorepanel.h"

cvar_t *hud_color;

class CHLVoiceStatusHelper : public IVoiceStatusHelper
{
public:
	virtual void GetPlayerTextColor(int entindex, int color[3])
	{
		color[0] = color[1] = color[2] = 255;

		if( entindex >= 0 && entindex < sizeof(g_PlayerExtraInfo)/sizeof(g_PlayerExtraInfo[0]) )
		{
			int iTeam = g_PlayerExtraInfo[entindex].teamnumber;

			if ( iTeam < 0 )
			{
				iTeam = 0;
			}

			iTeam = iTeam % iNumberOfTeamColors;

			color[0] = iTeamColors[iTeam][0];
			color[1] = iTeamColors[iTeam][1];
			color[2] = iTeamColors[iTeam][2];
		}
	}

	virtual void UpdateCursorState()
	{
		gViewPort->UpdateCursorState();
	}

	virtual int	GetAckIconHeight()
	{
		return ScreenHeight - gHUD.m_iFontHeight*3 - 6;
	}

	virtual bool			CanShowSpeakerLabels()
	{
		if( gViewPort && gViewPort->m_pScoreBoard )
			return !gViewPort->m_pScoreBoard->isVisible();
		else
			return false;
	}
};
static CHLVoiceStatusHelper g_VoiceStatusHelper;


extern client_sprite_t *GetSpriteList(client_sprite_t *pList, const char *psz, int iRes, int iCount);

extern cvar_t *sensitivity;
cvar_t *cl_lw = NULL;

void ShutdownInput (void);

//DECLARE_MESSAGE(m_Logo, Logo)
int __MsgFunc_Logo(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_Logo(pszName, iSize, pbuf );
}

//DECLARE_MESSAGE(m_Logo, Logo)
int __MsgFunc_ResetHUD(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_ResetHUD(pszName, iSize, pbuf );
}

int __MsgFunc_InitHUD(const char *pszName, int iSize, void *pbuf)
{
	gHUD.MsgFunc_InitHUD( pszName, iSize, pbuf );
	return 1;
}

int __MsgFunc_ViewMode(const char *pszName, int iSize, void *pbuf)
{
	gHUD.MsgFunc_ViewMode( pszName, iSize, pbuf );
	return 1;
}

int __MsgFunc_SetFOV(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_SetFOV( pszName, iSize, pbuf );
}

int __MsgFunc_Concuss(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_Concuss( pszName, iSize, pbuf );
}

int __MsgFunc_GameMode(const char *pszName, int iSize, void *pbuf )
{
	return gHUD.MsgFunc_GameMode( pszName, iSize, pbuf );
}

// TFFree Command Menu
void __CmdFunc_OpenCommandMenu(void)
{
	if ( gViewPort )
	{
		gViewPort->ShowCommandMenu( gViewPort->m_StandardMenu );
	}
}

// TFC "special" command
void __CmdFunc_InputPlayerSpecial(void)
{
	if ( gViewPort )
	{
		gViewPort->InputPlayerSpecial();
	}
}

void __CmdFunc_CloseCommandMenu(void)
{
	if ( gViewPort )
	{
		gViewPort->InputSignalHideCommandMenu();
	}
}

void __CmdFunc_ForceCloseCommandMenu( void )
{
	if ( gViewPort )
	{
		gViewPort->HideCommandMenu();
	}
}

void __CmdFunc_ToggleServerBrowser( void )
{
	if ( gViewPort )
	{
		gViewPort->ToggleServerBrowser();
	}
}

// TFFree Command Menu Message Handlers
int __MsgFunc_ValClass(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_ValClass( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_TeamNames(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_TeamNames( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_Feign(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_Feign( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_Detpack(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_Detpack( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_VGUIMenu(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_VGUIMenu( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_MOTD(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_MOTD( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_BuildSt(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_BuildSt( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_RandomPC(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_RandomPC( pszName, iSize, pbuf );
	return 0;
}
 
int __MsgFunc_ServerName(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_ServerName( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_ScoreInfo(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_ScoreInfo( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_TeamScore(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_TeamScore( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_TeamInfo(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_TeamInfo( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_Spectator(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_Spectator( pszName, iSize, pbuf );
	return 0;
}

int __MsgFunc_AllowSpec(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_AllowSpec( pszName, iSize, pbuf );
	return 0;
}

// <UNMODIFIED JULIEN'S CODE>
//duh..
cvar_t *cl_modelalpha,*hud_origin_x,*hud_origin_x_pos,*hud_origin_y,*hud_origin_y_pos,*hud_origin_z,*hud_origin_z_pos,*cl_wallhack,*hud_speedometer,*hud_health_pos,*hud_ammo_pos,*hud_acceleration,*hud_acceleration_pos,*hud_armor_pos,*hud_speedometer_pos,*hud_zspeed,*hud_jumpspeed,*hud_jumpspeed_pos,*hud_zspeed_pos;
cvar_t *hud_alpha,*hud_gaussboost;
// </UNMODIFIED JULIEN'S CODE>
 
cvar_t *r_drawviewmodel = NULL;

cvar_t *hud_pos_percent = NULL;

cvar_t *hud_gaussboost_pos;
cvar_t *hud_gaussammo;
cvar_t *hud_gaussammo_pos;

cvar_t *hud_grenadetimer,*hud_grenadetimer_pos, *hud_grenadetimer_width, *hud_grenadetimer_height;

cvar_t *hud_viewangle_x,*hud_viewangle_x_pos;
cvar_t *hud_viewangle_y,*hud_viewangle_y_pos;

cvar_t *hud_health_reqhev;
cvar_t *hud_health_5dig;

cvar_t *hud_battery_5dig;

cvar_t *hud_ammo_difference;
cvar_t *hud_ammo_difference_gauss;

cvar_t *hud_demorec_counter;
cvar_t *hud_demorec_counter_pos;

cvar_t *hud_grenadetimer_dontchange_resetto;
cvar_t *hud_gaussboost_dontchange_resetto;

cvar_t *hud_speedinfo, *hud_speedinfo_pos;

cvar_t *hud_damage;
cvar_t *hud_damage_enable, *hud_damage_size, *hud_damage_anim;

cvar_t *hud_entityhealth, *hud_entityhealth_pos;
cvar_t *hud_entityinfo, *hud_entityinfo_pos;

cvar_t *hud_firemon, *hud_firemon_pos;

cvar_t *hud_accuracy;

cvar_t *cl_rendermodels;
cvar_t *cl_rendergibs;

cvar_t *cl_ruler_enable;
cvar_t *cl_ruler_render;
cvar_t *cl_ruler_autodelay;

cvar_t *cl_autostopsave_cmd;
cvar_t *cl_autostopsave_radius;

cvar_t *cl_autocmd_enable, *cl_autocmd_render;
cvar_t *cl_autocmd_plane, *cl_autocmd_coord, *cl_autocmd_distance, *cl_autocmd_cmd;

/*cvar_t *cl_spawns_render, *cl_spawns_wireframe;
cvar_t *cl_spawns_drawcross;
cvar_t *cl_spawns_alpha;*/

cvar_t *cl_gauss_tracer;

cvar_t *cl_boxes_render;

bool g_bResetDemorecCounter = false;
 
void ResetDemorecCounter( void )
{
	g_bResetDemorecCounter = true;
}

bool g_bGrenTimeReset = false;

void ResetGrenadeTimer( void )
{
	g_bGrenTimeReset = true;
}

bool g_bDemorecChangelevel = false;

void ChangelevelOccured( void )
{
	g_bDemorecChangelevel = true;
}

extern bool g_bGausscharge;

void GaussboostDisable( void )
{
	g_bGausscharge = false;
}

bool g_bGaussboostReset = false;

void GaussboostReset( void )
{
	g_bGaussboostReset = true;
}

void RulerReset( void )
{
	ResetRuler();
}

void RulerPrintDistance( void )
{
	PrintDistance();
}

void RulerAimPos( void )
{
	AimPos();
}

void RulerPlayerPos( void )
{
	PlayerPos();
}

void RulerViewPos( void )
{
	ViewPos();
}

void RulerDeleteLast( void )
{
	DeleteLast();
}

void RulerAddOrigin( void )
{
	AddOrigin();
}

void RulerAutostopsaveAddPoint( void )
{
	AutostopsaveAddPoint();
}

void RulerAutostopsaveDeletePoint( void )
{
	AutostopsaveDeletePoint();
}

void RulerAutostopsavePrintOrigin( void )
{
	AutostopsavePrintOrigin();
}

void DamageHistoryReset( void )
{
	gHUD.m_CustomHud.DamageHistoryReset();
}

void HealthDifference( void )
{
	gHUD.m_CustomHud.HealthDifference();
}

// void FindSpawns( void )
// {
// 	FindSpawnsInMap();
// }
 
 // This is called every time the DLL is loaded
void CHud :: Init( void )
{
	HOOK_MESSAGE( Logo );
	HOOK_MESSAGE( ResetHUD );
	HOOK_MESSAGE( GameMode );
	HOOK_MESSAGE( InitHUD );
	HOOK_MESSAGE( ViewMode );
	HOOK_MESSAGE( SetFOV );
	HOOK_MESSAGE( Concuss );

	// TFFree CommandMenu
	HOOK_COMMAND( "+commandmenu", OpenCommandMenu );
	HOOK_COMMAND( "-commandmenu", CloseCommandMenu );
	HOOK_COMMAND( "ForceCloseCommandMenu", ForceCloseCommandMenu );
	HOOK_COMMAND( "special", InputPlayerSpecial );
	HOOK_COMMAND( "togglebrowser", ToggleServerBrowser );

	HOOK_MESSAGE( ValClass );
	HOOK_MESSAGE( TeamNames );
	HOOK_MESSAGE( Feign );
	HOOK_MESSAGE( Detpack );
	HOOK_MESSAGE( MOTD );
	HOOK_MESSAGE( BuildSt );
	HOOK_MESSAGE( RandomPC );
	HOOK_MESSAGE( ServerName );
	HOOK_MESSAGE( ScoreInfo );
	HOOK_MESSAGE( TeamScore );
	HOOK_MESSAGE( TeamInfo );

	HOOK_MESSAGE( Spectator );
	HOOK_MESSAGE( AllowSpec );

	// VGUI Menus
	HOOK_MESSAGE( VGUIMenu );

	CVAR_CREATE( "hud_classautokill", "1", FCVAR_ARCHIVE | FCVAR_USERINFO );		// controls whether or not to suicide immediately on TF class switch
	CVAR_CREATE( "hud_takesshots", "0", FCVAR_ARCHIVE );		// controls whether or not to automatically take screenshots at the end of a round

	InitRuler();
	InitBoxes();

	m_iLogo = 0;
	m_iFOV = 0;

	CVAR_CREATE( "zoom_sensitivity_ratio", "1.2", 0 );
	default_fov = CVAR_CREATE( "default_fov", "90", 0 );
	m_pCvarStealMouse = CVAR_CREATE( "hud_capturemouse", "1", FCVAR_ARCHIVE );
	m_pCvarDraw = CVAR_CREATE( "hud_draw", "1", FCVAR_ARCHIVE );
	cl_lw = gEngfuncs.pfnGetCvarPointer( "cl_lw" );
hud_color = gEngfuncs.pfnRegisterVariable( "hud_color", "-1", FCVAR_ARCHIVE);

hud_viewangle_x = gEngfuncs.pfnRegisterVariable( "hud_viewangle_x","0",FCVAR_ARCHIVE);
hud_viewangle_x_pos = gEngfuncs.pfnRegisterVariable( "hud_viewangle_x_pos","0",FCVAR_ARCHIVE);
hud_viewangle_y = gEngfuncs.pfnRegisterVariable( "hud_viewangle_y","0",FCVAR_ARCHIVE);
hud_viewangle_y_pos = gEngfuncs.pfnRegisterVariable( "hud_viewangle_y_pos","0",FCVAR_ARCHIVE);

hud_speedometer = gEngfuncs.pfnRegisterVariable( "hud_speedometer","1",FCVAR_ARCHIVE);
hud_speedometer_pos = gEngfuncs.pfnRegisterVariable( "hud_speedometer_pos", "0",FCVAR_ARCHIVE);
hud_zspeed = gEngfuncs.pfnRegisterVariable( "hud_zspeed","0",FCVAR_ARCHIVE);
hud_zspeed_pos = gEngfuncs.pfnRegisterVariable ( "hud_zspeed_pos","0",FCVAR_ARCHIVE);
hud_health_pos = gEngfuncs.pfnRegisterVariable( "hud_health_pos","0",FCVAR_ARCHIVE);
hud_ammo_pos = gEngfuncs.pfnRegisterVariable( "hud_ammo_pos","0",FCVAR_ARCHIVE);
hud_armor_pos = gEngfuncs.pfnRegisterVariable( "hud_armor_pos","0",FCVAR_ARCHIVE);
hud_acceleration = gEngfuncs.pfnRegisterVariable( "hud_acceleration","0",FCVAR_ARCHIVE);
hud_acceleration_pos = gEngfuncs.pfnRegisterVariable( "hud_acceleration_pos","0",FCVAR_ARCHIVE);
hud_jumpspeed = gEngfuncs.pfnRegisterVariable( "hud_jumpspeed", "1", FCVAR_ARCHIVE);
hud_jumpspeed_pos = gEngfuncs.pfnRegisterVariable ( "hud_jumpspeed_pos","0",FCVAR_ARCHIVE);
cl_wallhack = gEngfuncs.pfnRegisterVariable ("cl_wallhack","0",FCVAR_ARCHIVE);
cl_modelalpha = gEngfuncs.pfnRegisterVariable("cl_modelalpha","0",FCVAR_ARCHIVE);
hud_origin_x = gEngfuncs.pfnRegisterVariable("hud_origin_x","0",FCVAR_ARCHIVE);
hud_origin_x_pos = gEngfuncs.pfnRegisterVariable("hud_origin_x_pos","0",FCVAR_ARCHIVE);
hud_origin_y = gEngfuncs.pfnRegisterVariable("hud_origin_y","0",FCVAR_ARCHIVE);
hud_origin_y_pos = gEngfuncs.pfnRegisterVariable("hud_origin_y_pos","0",FCVAR_ARCHIVE);
hud_origin_z = gEngfuncs.pfnRegisterVariable("hud_origin_z","0",FCVAR_ARCHIVE);
hud_origin_z_pos = gEngfuncs.pfnRegisterVariable("hud_origin_z_pos","0",FCVAR_ARCHIVE);
r_drawviewmodel = gEngfuncs.pfnGetCvarPointer("r_drawviewmodel");
hud_pos_percent = gEngfuncs.pfnRegisterVariable ("hud_pos_percent", "0", FCVAR_ARCHIVE);

hud_alpha = gEngfuncs.pfnRegisterVariable("hud_alpha","0",FCVAR_ARCHIVE);

hud_gaussboost = gEngfuncs.pfnRegisterVariable("hud_gaussboost","0",FCVAR_ARCHIVE);
hud_gaussboost_pos = gEngfuncs.pfnRegisterVariable("hud_gaussboost_pos","0",FCVAR_ARCHIVE);

hud_gaussammo = gEngfuncs.pfnRegisterVariable("hud_gaussammo","0",FCVAR_ARCHIVE);
hud_gaussammo_pos = gEngfuncs.pfnRegisterVariable("hud_gaussammo_pos","0",FCVAR_ARCHIVE);

hud_grenadetimer = gEngfuncs.pfnRegisterVariable("hud_grenadetimer","1",FCVAR_ARCHIVE);
hud_grenadetimer_pos = gEngfuncs.pfnRegisterVariable("hud_grenadetimer_pos","0",FCVAR_ARCHIVE);
hud_grenadetimer_width = gEngfuncs.pfnRegisterVariable("hud_grenadetimer_width","10",FCVAR_ARCHIVE);
hud_grenadetimer_height = gEngfuncs.pfnRegisterVariable("hud_grenadetimer_height","100",FCVAR_ARCHIVE);

hud_health_reqhev =	gEngfuncs.pfnRegisterVariable("hud_health_reqhev", "1", FCVAR_ARCHIVE);
hud_health_5dig =	gEngfuncs.pfnRegisterVariable("hud_health_5dig", "1", FCVAR_ARCHIVE);

hud_battery_5dig =	gEngfuncs.pfnRegisterVariable("hud_battery_5dig", "1", FCVAR_ARCHIVE);

hud_demorec_counter = gEngfuncs.pfnRegisterVariable("hud_demorec_counter","1",FCVAR_ARCHIVE);
hud_demorec_counter_pos = gEngfuncs.pfnRegisterVariable("hud_demorec_counter_pos","0",FCVAR_ARCHIVE);

hud_grenadetimer_dontchange_resetto = gEngfuncs.pfnRegisterVariable("hud_grenadetimer_dontchange_resetto","-1",FCVAR_ARCHIVE);
hud_gaussboost_dontchange_resetto = gEngfuncs.pfnRegisterVariable("hud_gaussboost_dontchange_resetto","-1",FCVAR_ARCHIVE);

hud_speedinfo = gEngfuncs.pfnRegisterVariable("hud_speedinfo","0",FCVAR_ARCHIVE);
hud_speedinfo_pos = gEngfuncs.pfnRegisterVariable("hud_speedinfo_pos","0",FCVAR_ARCHIVE);

hud_damage = gEngfuncs.pfnRegisterVariable("hud_damage","0",FCVAR_ARCHIVE);
hud_damage_enable = gEngfuncs.pfnRegisterVariable("hud_damage_enable","1",FCVAR_ARCHIVE);
hud_damage_size = gEngfuncs.pfnRegisterVariable("hud_damage_size","5",FCVAR_ARCHIVE);
hud_damage_anim = gEngfuncs.pfnRegisterVariable("hud_damage_anim","1",FCVAR_ARCHIVE);

hud_entityhealth = gEngfuncs.pfnRegisterVariable("hud_entityhealth","0",FCVAR_ARCHIVE);
hud_entityhealth_pos = gEngfuncs.pfnRegisterVariable("hud_entityhealth_pos","0",FCVAR_ARCHIVE);

hud_entityinfo = gEngfuncs.pfnRegisterVariable("hud_entityinfo","0",FCVAR_ARCHIVE);
hud_entityinfo_pos = gEngfuncs.pfnRegisterVariable("hud_entityinfo_pos","0",FCVAR_ARCHIVE);

hud_accuracy = gEngfuncs.pfnRegisterVariable("hud_accuracy","0",FCVAR_ARCHIVE);

cl_rendermodels = gEngfuncs.pfnRegisterVariable("cl_rendermodels", "1", FCVAR_ARCHIVE);
cl_rendergibs = gEngfuncs.pfnRegisterVariable("cl_rendergibs", "1", FCVAR_ARCHIVE);

cl_ruler_enable = gEngfuncs.pfnRegisterVariable( "cl_ruler_enable", "1", FCVAR_ARCHIVE );
cl_ruler_render = gEngfuncs.pfnRegisterVariable( "cl_ruler_render", "1", FCVAR_ARCHIVE );
cl_ruler_autodelay = gEngfuncs.pfnRegisterVariable( "cl_ruler_autodelay", "0", FCVAR_ARCHIVE );

cl_autostopsave_cmd = gEngfuncs.pfnRegisterVariable( "cl_autostopsave_cmd", "stop;save autostopsave", FCVAR_ARCHIVE );
cl_autostopsave_radius = gEngfuncs.pfnRegisterVariable( "cl_autostopsave_radius", "50", FCVAR_ARCHIVE );

cl_autocmd_enable = gEngfuncs.pfnRegisterVariable( "cl_autocmd_enable", "0", FCVAR_ARCHIVE );
cl_autocmd_render = gEngfuncs.pfnRegisterVariable( "cl_autocmd_render", "0", FCVAR_ARCHIVE );
cl_autocmd_plane = gEngfuncs.pfnRegisterVariable( "cl_autocmd_plane", "x", FCVAR_ARCHIVE );
cl_autocmd_coord = gEngfuncs.pfnRegisterVariable( "cl_autocmd_coord", "0", FCVAR_ARCHIVE );
cl_autocmd_distance = gEngfuncs.pfnRegisterVariable( "cl_autocmd_distance", "34.0", FCVAR_ARCHIVE );
cl_autocmd_cmd = gEngfuncs.pfnRegisterVariable( "cl_autocmd_cmd", "stop;save autocmdsave", FCVAR_ARCHIVE );

hud_ammo_difference = CVAR_CREATE( "hud_ammo_difference", "0", FCVAR_ARCHIVE );
hud_ammo_difference_gauss = CVAR_CREATE( "hud_ammo_difference_gauss", "0", FCVAR_ARCHIVE );

hud_firemon = CVAR_CREATE( "hud_firemon", "0", FCVAR_ARCHIVE );
hud_firemon_pos = CVAR_CREATE( "hud_firemon_pos", "0", FCVAR_ARCHIVE );

// cl_spawns_render = CVAR_CREATE( "cl_spawns_render", "0", FCVAR_ARCHIVE );
// cl_spawns_wireframe = CVAR_CREATE( "cl_spawns_wireframe", "1", FCVAR_ARCHIVE );
// cl_spawns_drawcross = CVAR_CREATE( "cl_spawns_drawcross", "1", FCVAR_ARCHIVE );
// cl_spawns_alpha = CVAR_CREATE( "cl_spawns_alpha", "0.2", FCVAR_ARCHIVE );

cl_gauss_tracer = CVAR_CREATE( "cl_gauss_tracer", "0", FCVAR_ARCHIVE );

cl_boxes_render = CVAR_CREATE( "cl_boxes_render", "1", FCVAR_ARCHIVE );

gEngfuncs.pfnAddCommand("hud_demorec_reset", ResetDemorecCounter);
gEngfuncs.pfnAddCommand("hud_grenadetimer_reset", ResetGrenadeTimer);
gEngfuncs.pfnAddCommand("hud_dontchange_changelevel_occured", ChangelevelOccured);
gEngfuncs.pfnAddCommand("hud_gaussboost_disable", GaussboostDisable);
gEngfuncs.pfnAddCommand("hud_gaussboost_reset", GaussboostReset);
gEngfuncs.pfnAddCommand("hud_damage_reset", DamageHistoryReset);
gEngfuncs.pfnAddCommand("hud_health_difference", HealthDifference);

gEngfuncs.pfnAddCommand( "cl_ruler_reset", RulerReset );
gEngfuncs.pfnAddCommand( "cl_ruler_distance", RulerPrintDistance );
gEngfuncs.pfnAddCommand( "cl_ruler_aimpos", RulerAimPos );
gEngfuncs.pfnAddCommand( "cl_ruler_playerpos", RulerPlayerPos );
gEngfuncs.pfnAddCommand( "cl_ruler_viewpos", RulerViewPos );
gEngfuncs.pfnAddCommand( "cl_ruler_dellast", RulerDeleteLast );
gEngfuncs.pfnAddCommand( "cl_ruler_addorigin", RulerAddOrigin );

gEngfuncs.pfnAddCommand( "cl_autostopsave_addpoint", RulerAutostopsaveAddPoint );
gEngfuncs.pfnAddCommand( "cl_autostopsave_delpoint", RulerAutostopsaveDeletePoint );
gEngfuncs.pfnAddCommand( "cl_autostopsave_printorigin", RulerAutostopsavePrintOrigin );

gEngfuncs.pfnAddCommand( "cl_boxes_reset", ResetBoxes );
gEngfuncs.pfnAddCommand( "cl_boxes_dellast", DeleteLastBox );
gEngfuncs.pfnAddCommand( "cl_boxes_add", AddBox );

// gEngfuncs.pfnAddCommand( "cl_findspawns", FindSpawns );

	m_pSpriteList = NULL;

	// Clear any old HUD list
	if ( m_pHudList )
	{
		HUDLIST *pList;
		while ( m_pHudList )
		{
			pList = m_pHudList;
			m_pHudList = m_pHudList->pNext;
			free( pList );
		}
		m_pHudList = NULL;
	}

	// In case we get messages before the first update -- time will be valid
	m_flTime = 1.0;

	m_Ammo.Init();
	m_Health.Init();
	m_SayText.Init();
	m_Spectator.Init();
	m_Geiger.Init();
	m_Train.Init();
	m_Battery.Init();
	m_Flash.Init();
	m_Message.Init();
	m_StatusBar.Init();
	m_DeathNotice.Init();
	m_AmmoSecondary.Init();
	m_TextMessage.Init();
	m_StatusIcons.Init();

	// YaLTeR
	m_CustomHud.Init();

	GetClientVoiceMgr()->Init(&g_VoiceStatusHelper, (vgui::Panel**)&gViewPort);

	m_Menu.Init();
	
	ServersInit();

	MsgFunc_ResetHUD(0, 0, NULL );
}

// CHud destructor
// cleans up memory allocated for m_rg* arrays
CHud :: ~CHud()
{
	ResetRuler();
	ResetBoxes();
	AutostopsaveDeletePoint();

	delete [] m_rghSprites;
	delete [] m_rgrcRects;
	delete [] m_rgszSpriteNames;

	if ( m_pHudList )
	{
		HUDLIST *pList;
		while ( m_pHudList )
		{
			pList = m_pHudList;
			m_pHudList = m_pHudList->pNext;
			free( pList );
		}
		m_pHudList = NULL;
	}

	ServersShutdown();
}

// GetSpriteIndex()
// searches through the sprite list loaded from hud.txt for a name matching SpriteName
// returns an index into the gHUD.m_rghSprites[] array
// returns 0 if sprite not found
int CHud :: GetSpriteIndex( const char *SpriteName )
{
	// look through the loaded sprite name list for SpriteName
	for ( int i = 0; i < m_iSpriteCount; i++ )
	{
		if ( strncmp( SpriteName, m_rgszSpriteNames + (i * MAX_SPRITE_NAME_LENGTH), MAX_SPRITE_NAME_LENGTH ) == 0 )
			return i;
	}

	return -1; // invalid sprite
}

void CHud :: VidInit( void )
{
	m_scrinfo.iSize = sizeof(m_scrinfo);
	GetScreenInfo(&m_scrinfo);

	// ----------
	// Load Sprites
	// ---------
//	m_hsprFont = LoadSprite("sprites/%d_font.spr");
	
	m_hsprLogo = 0;	
	m_hsprCursor = 0;

	if (ScreenWidth < 640)
		m_iRes = 320;
	else
		m_iRes = 640;

	// Only load this once
	if ( !m_pSpriteList )
	{
		// we need to load the hud.txt, and all sprites within
		m_pSpriteList = SPR_GetList("sprites/hud.txt", &m_iSpriteCountAllRes);

		if (m_pSpriteList)
		{
			// count the number of sprites of the appropriate res
			m_iSpriteCount = 0;
			client_sprite_t *p = m_pSpriteList;
			for ( int j = 0; j < m_iSpriteCountAllRes; j++ )
			{
				if ( p->iRes == m_iRes )
					m_iSpriteCount++;
				p++;
			}

			// allocated memory for sprite handle arrays
 			m_rghSprites = new HSPRITE[m_iSpriteCount];
			m_rgrcRects = new wrect_t[m_iSpriteCount];
			m_rgszSpriteNames = new char[m_iSpriteCount * MAX_SPRITE_NAME_LENGTH];

			p = m_pSpriteList;
			int index = 0;
			for ( j = 0; j < m_iSpriteCountAllRes; j++ )
			{
				if ( p->iRes == m_iRes )
				{
					char sz[256];
					sprintf(sz, "sprites/%s.spr", p->szSprite);
					m_rghSprites[index] = SPR_Load(sz);
					m_rgrcRects[index] = p->rc;
					strncpy( &m_rgszSpriteNames[index * MAX_SPRITE_NAME_LENGTH], p->szName, MAX_SPRITE_NAME_LENGTH );

					index++;
				}

				p++;
			}
		}
	}
	else
	{
		// we have already have loaded the sprite reference from hud.txt, but
		// we need to make sure all the sprites have been loaded (we've gone through a transition, or loaded a save game)
		client_sprite_t *p = m_pSpriteList;
		int index = 0;
		for ( int j = 0; j < m_iSpriteCountAllRes; j++ )
		{
			if ( p->iRes == m_iRes )
			{
				char sz[256];
				sprintf( sz, "sprites/%s.spr", p->szSprite );
				m_rghSprites[index] = SPR_Load(sz);
				index++;
			}

			p++;
		}
	}

	// assumption: number_1, number_2, etc, are all listed and loaded sequentially
	m_HUD_number_0 = GetSpriteIndex( "number_0" );

	m_iFontHeight = m_rgrcRects[m_HUD_number_0].bottom - m_rgrcRects[m_HUD_number_0].top;

	m_Ammo.VidInit();
	m_Health.VidInit();
	m_Spectator.VidInit();
	m_Geiger.VidInit();
	m_Train.VidInit();
	m_Battery.VidInit();
	m_Flash.VidInit();
	m_Message.VidInit();
	m_StatusBar.VidInit();
	m_DeathNotice.VidInit();
	m_SayText.VidInit();
	m_Menu.VidInit();
	m_AmmoSecondary.VidInit();
	m_TextMessage.VidInit();
	m_StatusIcons.VidInit();

	// YaLTeR
	m_CustomHud.VidInit();
	
	GetClientVoiceMgr()->VidInit();
}

int CHud::MsgFunc_Logo(const char *pszName,  int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	// update Train data
	m_iLogo = READ_BYTE();

	return 1;
}

float g_lastFOV = 0.0;

/*
============
COM_FileBase
============
*/
// Extracts the base name of a file (no path, no extension, assumes '/' as path separator)
void COM_FileBase ( const char *in, char *out)
{
	int len, start, end;

	len = strlen( in );
	
	// scan backward for '.'
	end = len - 1;
	while ( end && in[end] != '.' && in[end] != '/' && in[end] != '\\' )
		end--;
	
	if ( in[end] != '.' )		// no '.', copy to end
		end = len-1;
	else 
		end--;					// Found ',', copy to left of '.'


	// Scan backward for '/'
	start = len-1;
	while ( start >= 0 && in[start] != '/' && in[start] != '\\' )
		start--;

	if ( in[start] != '/' && in[start] != '\\' )
		start = 0;
	else 
		start++;

	// Length of new sting
	len = end - start + 1;

	// Copy partial string
	strncpy( out, &in[start], len );
	// Terminate it
	out[len] = 0;
}

/*
=================
HUD_IsGame

=================
*/
int HUD_IsGame( const char *game )
{
	const char *gamedir;
	char gd[ 1024 ];

	gamedir = gEngfuncs.pfnGetGameDirectory();
	if ( gamedir && gamedir[0] )
	{
		COM_FileBase( gamedir, gd );
		if ( !stricmp( gd, game ) )
			return 1;
	}
	return 0;
}

/*
=====================
HUD_GetFOV

Returns last FOV
=====================
*/
float HUD_GetFOV( void )
{
	if ( gEngfuncs.pDemoAPI->IsRecording() )
	{
		// Write it
		int i = 0;
		unsigned char buf[ 100 ];

		// Active
		*( float * )&buf[ i ] = g_lastFOV;
		i += sizeof( float );

		Demo_WriteBuffer( TYPE_ZOOM, i, buf );
	}

	if ( gEngfuncs.pDemoAPI->IsPlayingback() )
	{
		g_lastFOV = g_demozoom;
	}
	return g_lastFOV;
}

int CHud::MsgFunc_SetFOV(const char *pszName,  int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	int newfov = READ_BYTE();
	int def_fov = CVAR_GET_FLOAT( "default_fov" );

	//Weapon prediction already takes care of changing the fog. ( g_lastFOV ).
	if ( cl_lw && cl_lw->value )
		return 1;

	g_lastFOV = newfov;

	if ( newfov == 0 )
	{
		m_iFOV = def_fov;
	}
	else
	{
		m_iFOV = newfov;
	}

	// the clients fov is actually set in the client data update section of the hud

	// Set a new sensitivity
	if ( m_iFOV == def_fov )
	{  
		// reset to saved sensitivity
		m_flMouseSensitivity = 0;
	}
	else
	{  
		// set a new sensitivity that is proportional to the change from the FOV default
		m_flMouseSensitivity = sensitivity->value * ((float)newfov / (float)def_fov) * CVAR_GET_FLOAT("zoom_sensitivity_ratio");
	}

	return 1;
}


void CHud::AddHudElem(CHudBase *phudelem)
{
	HUDLIST *pdl, *ptemp;

//phudelem->Think();

	if (!phudelem)
		return;

	pdl = (HUDLIST *)malloc(sizeof(HUDLIST));
	if (!pdl)
		return;

	memset(pdl, 0, sizeof(HUDLIST));
	pdl->p = phudelem;

	if (!m_pHudList)
	{
		m_pHudList = pdl;
		return;
	}

	ptemp = m_pHudList;

	while (ptemp->pNext)
		ptemp = ptemp->pNext;

	ptemp->pNext = pdl;
}

float CHud::GetSensitivity( void )
{
	return m_flMouseSensitivity;
}


