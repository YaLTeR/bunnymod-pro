#include <sstream>

#include "util_vector.h"

#include "ScriptExporter.h"

const char* const buttonCommands[] = { "attack",
                                       "jump",
                                       "duck",
                                       "forward",
                                       "back",
                                       "use",
                                       "cancelselect", // Special handling.
                                       "left",         // Skipped.
                                       "right",        // Skipped.
                                       "moveleft",
                                       "moveright",
                                       "attack2",
                                       "speed",
                                       "reload",
                                       "speed",        // Not used.
                                       "reload",
                                       "alt1",
                                       "showscores" };

void ScriptExporter::ConsiderCVarValues(float enable, const char* fileName, float paused, const char* demoName, const char* saveName)
{
	if ((enable != 0.0f) && !m_out.is_open())
	{
		std::ostringstream fileNameWithExt;
		fileNameWithExt << fileName << ".cfg";
		m_out.open(fileNameWithExt.str().c_str());

		if (m_out.is_open())
		{
			m_out.setf(std::ios::fixed, std::ios::floatfield);
			m_out.precision(8);

			if (demoName[0] != '\0')
				m_out << "record " << demoName << "\n";

			m_out << "+lookdown\n+left\ncl_pitchspeed 0\ncl_yawspeed 0\ncl_forwardspeed 10000\ncl_sidespeed 10000\ncl_backspeed 10000\ncl_upspeed 10000";

			m_fileName.assign(fileName);
			m_fileCount = 0;

			m_oldPitchspeed = 0;
			m_oldYawspeed = 0;
			m_oldButtons = 0;
			m_oldFrametime = 0;
		}
	}

	if ((enable == 0.0f) && m_out.is_open())
	{
		m_out << ";host_framerate 0;wait\ncl_forwardspeed 400\ncl_sidespeed 400\ncl_backspeed 400\ncl_upspeed 400\ncl_pitchspeed 225\ncl_yawspeed 210\n-lookdown\n-left";

		// Clear out the pressed buttons.
		for (unsigned char i = 0; i < 16; ++i)
		{
			if ((i >= 6) && (i <= 8)) // Skip IN_CANCEL, IN_LEFT and IN_RIGHT.
				continue;

			if ((m_oldButtons & (1 << i)) != 0)
				m_out << "\n-" << buttonCommands[i];
		}

		if (saveName[0] != '\0')
			m_out << "\nsave " << saveName;

		m_out.close();
	}
}

void ScriptExporter::FrameStart(const vec3_t& viewangles, float frametime)
{
	if (!m_out.is_open())
		return;

	if (frametime != m_oldFrametime)
	{
		m_oldFrametime = frametime;
		m_out << ";host_framerate " << frametime;
	}

	m_out << ";wait\n";

	m_startPitch = viewangles[0];
	m_startYaw   = viewangles[1];

	if (m_out.tellp() >= 16000)
		Split();
}

void ScriptExporter::FrameEnd(const vec3_t& viewangles, int buttons)
{
	if (!m_out.is_open())
		return;

	const double M_U_HALF = 0.00274658203125;

	// m_oldFrametime here is equal to the current frametime.
	double pitchspeed = ((viewangles[0] - m_startPitch + M_U_HALF) / m_oldFrametime),
	       yawspeed   = ((viewangles[1] - m_startYaw + M_U_HALF) / m_oldFrametime);

	if (pitchspeed != m_oldPitchspeed)
	{
		m_oldPitchspeed = pitchspeed;
		m_out << "cl_pitchspeed " << pitchspeed;
	}

	if (yawspeed != m_oldYawspeed)
	{
		m_oldYawspeed = yawspeed;
		m_out << ";cl_yawspeed " << yawspeed;
	}

	int buttonDifference = buttons ^ m_oldButtons;
	for (unsigned char i = 0; i < 16; ++i)
	{
		if ((i >= 6) && (i <= 8)) // Skip IN_CANCEL, IN_LEFT and IN_RIGHT.
			continue;

		if ((buttonDifference & (1 << i)) != 0)
		{
			if ((buttons & (1 << i)) != 0)
				m_out << ";+" << buttonCommands[i];
			else
				m_out << ";-" << buttonCommands[i];
		}
	}

	if ((buttons & (1 << 6)) != 0) // IN_CANCEL
		m_out << ";cancelselect";

	m_oldButtons = buttons;
}

void ScriptExporter::Split()
{
	if (!m_out.is_open())
		return;

	m_fileCount++;
	std::ostringstream newFileName;
	newFileName << m_fileName << "_" << m_fileCount << ".cfg";

	m_out << "exec " << newFileName.str();
	m_out.close();

	m_out.open(newFileName.str().c_str());
}
