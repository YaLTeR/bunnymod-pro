#pragma once

#include <fstream>
#include <string>

class ScriptExporter
{
public:
	void ConsiderCVarValues(float enable, const char* fileName, float paused, const char* demoName, const char* saveName);
	void FrameStart(const vec3_t& viewangles, float frametime);
	void FrameEnd(const vec3_t& viewangles, int buttons);

protected:
	void Split();

	std::string m_fileName;
	std::ofstream m_out;
	unsigned long int m_fileCount;

	double m_startPitch;
	double m_startYaw;
	double m_oldPitchspeed;
	double m_oldYawspeed;
	int m_oldButtons;
	float m_oldFrametime;
};
