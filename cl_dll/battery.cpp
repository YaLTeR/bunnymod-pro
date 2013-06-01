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
// battery.cpp
//
// implementation of CHudBattery class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

DECLARE_MESSAGE(m_Battery, Battery)

int CHudBattery::Init(void)
{
	m_iBat = 0;
	m_fFade = 0;
	m_iFlags = 0;

	HOOK_MESSAGE(Battery);

	gHUD.AddHudElem(this);

	return 1;
};


int CHudBattery::VidInit(void)
{
	int HUD_suit_empty = gHUD.GetSpriteIndex( "suit_empty" );
	int HUD_suit_full = gHUD.GetSpriteIndex( "suit_full" );

	m_hSprite1 = m_hSprite2 = 0;  // delaying get sprite handles until we know the sprites are loaded
	m_prc1 = &gHUD.GetSpriteRect( HUD_suit_empty );
	m_prc2 = &gHUD.GetSpriteRect( HUD_suit_full );
	m_iHeight = m_prc2->bottom - m_prc1->top;
	m_fFade = 0;
	return 1;
};

extern cvar_t *hud_battery_5dig;

int CHudBattery:: MsgFunc_Battery(const char *pszName,  int iSize, void *pbuf )
{
	m_iFlags |= HUD_ACTIVE;

	
	BEGIN_READ( pbuf, iSize );

	int x;
	if (hud_battery_5dig->value)
	{
		x = READ_SHORT();
	}
	else
	{
		x = READ_SHORT();
	}

	if (x != m_iBat)
	{
		m_fFade = FADE_TIME;
		m_iBat = x;
	}

	return 1;
}

extern cvar_t *hud_pos_percent;
extern cvar_t *hud_alpha;
extern cvar_t *hud_armor_pos;

int CHudBattery::Draw(float flTime)
{
	if ( gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH )
		return 1;

	int r, g, b, x, y, a;
	int x_offset = 0, y_offset = 0;
	wrect_t rc;

	rc = *m_prc2;
	rc.top  += m_iHeight * ((float)(100-(min(100,m_iBat))) * 0.01);	// battery can go from 0 to 100 so * 0.01 goes from 0 to 1

	GetHudColor(r, g, b);
	//UnpackRGB(r,g,b, RGB_YELLOWISH);

	if (!(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)) ))
		return 1;

	// Has health changed? Flash the health #
	if (m_fFade)
	{
		if (m_fFade > FADE_TIME)
			m_fFade = FADE_TIME;

		m_fFade -= (gHUD.m_flTimeDelta * 20);
		if (m_fFade <= 0)
		{
			a = 128;
			m_fFade = 0;
		}

		// Fade the health number back to dim

		a = MIN_ALPHA +  (m_fFade/FADE_TIME) * 128;

	}
	else
		a = MIN_ALPHA;
		
	if (hud_alpha->value)
	{
		a = hud_alpha->value;

		if ( a > 255 )	a = 255;
		if ( a < 1 )	a = 1;
	}
	else
	{
		a = 255;
	}

	ScaleColors(r, g, b, a );
	
	int iOffset = (m_prc1->bottom - m_prc1->top)/6;

	y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;
	x = ScreenWidth/5;
	
	if (hud_armor_pos->string)
	{
		sscanf(hud_armor_pos->string, "%i %i", &x_offset, &y_offset);
		x += x_offset;
	}

	// make sure we have the right sprite handles
	if ( !m_hSprite1 )
		m_hSprite1 = gHUD.GetSprite( gHUD.GetSpriteIndex( "suit_empty" ) );
	if ( !m_hSprite2 )
		m_hSprite2 = gHUD.GetSprite( gHUD.GetSpriteIndex( "suit_full" ) );

	SPR_Set(m_hSprite1, r, g, b );
	
	int x_temp = x_offset > 100 ? ScreenWidth : (x_offset * ScreenWidth) / 100;
	
	if (hud_pos_percent->value)
	{
		SPR_DrawAdditive( 0,  x_temp, y_offset > 100 ? ScreenHeight - iOffset : (y_offset * ScreenHeight) / 100 - iOffset, m_prc1);
	}
	else
	{
		SPR_DrawAdditive( 0,  x, y - y_offset - iOffset, m_prc1);
	}

	if (rc.bottom > rc.top)
	{
		SPR_Set(m_hSprite2, r, g, b );
		if (hud_pos_percent->value)
		{
			SPR_DrawAdditive( 0, x_temp, y_offset > 100 ? ScreenHeight - iOffset + (rc.top - m_prc2->top) : (y_offset * ScreenHeight) / 100 - iOffset + (rc.top - m_prc2->top), &rc);
		}
		else
		{
			SPR_DrawAdditive( 0, x, y - y_offset - iOffset + (rc.top - m_prc2->top), &rc);
		}
	}

	x += (m_prc1->right - m_prc1->left);
	x_temp += (m_prc1->right - m_prc1->left);
	
	if (hud_pos_percent->value)
	{
		x = gHUD.DrawHudNumber(x_temp, y_offset > 100 ? ScreenHeight : (y_offset * ScreenHeight) / 100, DHN_3DIGITS | DHN_DRAWZERO, m_iBat, r, g, b);
	}
	else
	{
		x = gHUD.DrawHudNumber(x, y - y_offset, DHN_3DIGITS | DHN_DRAWZERO, m_iBat, r, g, b);
	}

	return 1;
}