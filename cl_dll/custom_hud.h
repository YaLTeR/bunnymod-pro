#define JUMPSPEED_FADE_TIME 7

class CHudCustom: public CHudBase
{
public:
	virtual int Init( void );
	virtual int VidInit( void );
	virtual int Draw( float fTime );

	static int DrawNumber( int number, int x, int y, int dx, int dy, bool isNegative = false, int r = 0, int g = 0, int b = 0 );
	static int DrawNumber( double number, int x, int y, int dx, int dy );
	static int DrawString( char *stringToDraw, int x, int y, int dx, int dy );

	float m_fJumpspeedFadeGreen;
	float m_fJumpspeedFadeRed;
};