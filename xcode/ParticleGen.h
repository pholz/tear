/*
 *  ParticleGen.h
 *  hex
 *
 *  Created by Peter Holzkorn on 29.11.10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include <vector>
#include "Particle.h"

using namespace ci;
using namespace std;

class ParticleGen
{
public:
	vector<Particle*> particles;
	float interval, lifetime, acc;
	Vec2f pos;
	Rand *rand;
	const Vec2f* centroid;
	
	ParticleGen(const Vec2f* centroid, float _iv, float _lt);
	~ParticleGen();
	
	void update(float dt);
	
	void draw();
};