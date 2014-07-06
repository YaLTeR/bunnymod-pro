#include <string>

#include "util_vector.h"
#include "MotionExporter.h"

void MotionExporter::ConsiderCVarValues(float enable, const char* fileName, float frameTime)
{
	if ((enable != 0.0f) && !m_out.is_open())
	{
		m_frameCount = 0;

		std::string fileNameWithExt(fileName);
		fileNameWithExt.append(".bvh");
		m_out.open(fileNameWithExt.c_str(), std::ios_base::binary | std::ios_base::out); // Open in binary mode for the correct carriage return.

		if (m_out.is_open())
		{
			m_out.setf(std::ios::fixed, std::ios::floatfield);
			m_out.precision(6);
			m_out << "HIERARCHY\nROOT MdtCam\n{\n\tOFFSET 0.00 0.00 0.00\n\tCHANNELS 6 Xposition Yposition Zposition Zrotation Xrotation Yrotation\n\tEnd Site\n\t{\n\t\tOFFSET 0.00 0.00 -1.00\n\t}\n}\nMOTION\nFrames:";
			m_framesPos = m_out.tellp();
			m_out << "123456789012\nFrame Time: " << frameTime << "\n";
		}
	}

	if ((enable == 0.0f) && m_out.is_open())
	{
		m_out.seekp(m_framesPos);
		m_out.width(12);
		m_out << m_frameCount;
		m_out.close();
	}
}

void MotionExporter::ConsiderPlayerData(const vec3_t& origin, const vec3_t& angles)
{
	if (!m_out.is_open())
		return;

	m_frameCount++;

	m_out << -origin[1] << " " <<  origin[2] << " " << -origin[0] << " ";
	m_out << -angles[2] << " " << -angles[0] << " " <<  angles[1] << "\n";
}