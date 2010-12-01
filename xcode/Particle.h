/*
 *  Particle.h
 *  hex
 *
 *  Created by Peter Holzkorn on 29.11.10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */


#include "common.h"

#define PSPEED 400

using namespace ci;

class Particle
{
public:
	Vec3f pos;
	Vec2f vel;
	float expired, lifetime;
	Rand *rand;
	
	Particle(Vec2f _pos, float _lifetime, Rand* _r);
	
	void update(float dt);
	
	void draw();
};