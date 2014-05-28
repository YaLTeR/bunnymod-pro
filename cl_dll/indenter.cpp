#include "indenter.h"

Indenter::Indenter(_PrintfFunc func, const char* sequence)
{
	printfFunc = func;
	indentSequence = sequence;
}

void Indenter::startSection(const char* sectionName)
{
	indentString += indentSequence;

	if (sectionName)
	{
		indent();
		printfFunc("-- %s Start --\n", sectionName);
	}
}

void Indenter::endSection(const char* sectionName)
{
	if (sectionName)
	{
		indent();
		printfFunc("-- %s End --\n", sectionName);
	}

	if (indentString.length() >= indentSequence.length())
		indentString.resize(indentString.length() - indentSequence.length());
}

void Indenter::indent()
{
	if (indentString.length() > 0)
		printfFunc( (char *)indentString.c_str() );
}
