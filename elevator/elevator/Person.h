#pragma once
#include <string>
using namespace std;
class Person
{
public:
	char name[16];
	int srclevel;
	int detlevel;
	Person(char *inName,int inSrcLevel,int inDetLevel);
	~Person();
};

