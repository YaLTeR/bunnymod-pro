#include <string>

typedef void (*_PrintfFunc) (char* format, ...);

class Indenter
{
public:
	_PrintfFunc printfFunc;
	std::string indentString;
	std::string indentSequence;

public:
	Indenter(_PrintfFunc func, const char* sequence);

	void startSection(const char* sectionName);
	void endSection(const char* sectionName);
	void indent();
};