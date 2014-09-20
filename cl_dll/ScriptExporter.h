#pragma once

#include <fstream>

class ScriptExporter
{
public:
	void ConsiderCVarValues(float enable, const char* fileName, float paused, const char* demoName, const char* saveName);
	void FrameStart(const vec3_t& viewangles, float frametime);
	void FrameEnd(const vec3_t& viewangles, int buttons);

protected:
	std::ofstream m_out;
	double m_startPitch;
	double m_startYaw;
	double m_oldPitchDifference;
	double m_oldYawDifference;
	int m_oldButtons;
	float m_oldFrametime;
};