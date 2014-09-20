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
	float m_startPitch;
	float m_startYaw;
	float m_oldPitchDifference;
	float m_oldYawDifference;
	int m_oldButtons;
	float m_oldFrametime;
};