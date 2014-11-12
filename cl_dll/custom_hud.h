#include <vector>

class CHudCustom: public CHudBase
{
public:
	virtual int Init( void );
	virtual int VidInit( void );
	virtual int Draw( float fTime );

	void HealthChanged( int delta );
	void DamageHistoryReset( void );

	void HealthDifference( void );

	void CalculateLadderViewangles( void );

	int MsgFunc_EntHealth( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_EntInfo( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_EntFired( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_FireReset( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_VelClip( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_PlayerInfo( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_LadderInfo( const char *pszName, int iSize, void *pbuf );

	static int DrawNumber( int number, int x, int y, int dx, int dy, bool isNegative = false, int r = 0, int g = 0, int b = 0, bool transparent = false );
	static int DrawNumber( double number, int x, int y, int dx, int dy, float r = -1.0f, float g = 0.0f, float b = 0.0f );
	static int DrawString( char *stringToDraw, int x, int y, int dx, int dy, float r = -1.0f, float g = 0.0f, float b = 0.0f );

	int g_iHealthDifference;

	HSPRITE g_hsprWhite;

private:
	float m_fJumpspeedFadeGreen;
	float m_fJumpspeedFadeRed;

	std::vector<int> m_ivDamage;

	float m_fDamageAnimTime;
	float m_fChargingTime;
	bool m_bChargingHealth;

	float m_fEntityHealth;

	int m_iNumFires;

	unsigned int m_uiVelClipCount;
	float m_flVelClipPlaneAngle;
	float m_flVelClipTime;

	bool m_wasOnLadder;
	vec3_t m_ladderNormal;
};