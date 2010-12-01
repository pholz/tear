/*
 *  Particle.cpp
 *  hex
 *
 *  Created by Peter Holzkorn on 29.11.10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "Particle.h"
	
Particle::Particle(Vec2f _pos, float _lifetime, Rand* _r)
{
	lifetime = _lifetime;
	rand = _r;
	pos = Vec3f(_pos.x, _pos.y, .0f) + Vec3f(rand->nextFloat(-1000.0f,1000.0f), rand->nextFloat(-1000.0f,1000.0f), rand->nextFloat(-10.0f, 10.0f));
	expired = .0f;
	
	vel = Vec2f(rand->nextFloat(-PSPEED, PSPEED), .0f);
}

void Particle::update(float dt)
{
	pos += Vec3f(vel.x, vel.y, .0f) * dt;
	
	expired += dt;
}

void Particle::draw()
{
	glPushMatrix();
	gl::translate(pos);
	gl::color(ColorA(.5f, .5f, .5f, .2f));
	glLineWidth(1.0f);
	gl::drawStrokedCircle(Vec2f(.0f, .0f), 35.0f, 8);
	glPopMatrix();
}



