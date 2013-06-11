#include <vector>

class CHudCustom: public CHudBase
{
public:
	virtual int Init( void );
	virtual int VidInit( void );
	virtual int Draw( float fTime );

	void HealthChanged( int delta );
	void DamageHistoryReset( void );

	static int DrawNumber( int number, int x, int y, int dx, int dy, bool isNegative = false, int r = 0, int g = 0, int b = 0, bool colors = false );
	static int DrawNumber( double number, int x, int y, int dx, int dy );
	static int DrawString( char *stringToDraw, int x, int y, int dx, int dy );

private:
	float m_fJumpspeedFadeGreen;
	float m_fJumpspeedFadeRed;

	std::vector<int> m_ivDamage;

	float m_fDamageAnimTime;
};