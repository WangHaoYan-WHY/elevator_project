#include "stdafx.h"
#include "Person.h"


Person::Person(char *inName, int inSrcLevel, int inDetLevel)
{
	snprintf(name, sizeof(name), "%s", inName);
	srclevel = inSrcLevel;
	detlevel = inDetLevel;
}


Person::~Person()
{
}
