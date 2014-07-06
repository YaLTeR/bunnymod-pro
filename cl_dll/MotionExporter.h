#pragma once

#include <fstream>

class MotionExporter
{
public:
	void ConsiderCVarValues(float enable, const char* fileName, float frameTime);
	void ConsiderPlayerData(const vec3_t& origin, const vec3_t& angles);

protected:
	std::ofstream m_out;
	std::streampos m_framesPos;
	long int m_frameCount;

};