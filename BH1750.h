#pragma once

#ifndef BH1750_h
#define BH1750_h

#include "Arduino.h"
#include <Wire.h>


#define BH1750_INSIDE_ADDRESS	0x23
#define BH1750_OUTSIDE_ADDRESS	0x24


class BH1750Class
{
public:
	bool INIT (int address);
	int BH1750_Read(int address);
};

extern BH1750Class BH1750;


#endif