/*
 *  EnemyGenerator.cpp
 *  tear
 *
 *  Created by Peter Holzkorn on 17.10.10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "EnemyGenerator.h"
#include "cinder/gl/gl.h"

#include "util.h"

EnemyGenerator::EnemyGenerator(GameState* _gs, float _iv, float _lt)
{
	gs = _gs;
	interval = _iv;
	lifetime = _lt;
	
	last = .0f;
	acc = .0f;
	total = .0f;
	
	rand = new Rand();
}

void EnemyGenerator::update(float dt)
{
	last = total;
	total += dt;
	acc += dt;
	
	if(acc >= interval)
	{
		acc = 0;
		enemies.push_back(Enemy(gs, lifetime, rand));
	}
	
	vector<Enemy>::iterator it;
	for(it = enemies.begin(); it < enemies.end(); it++)
	{
		it->update(dt);
		if(it->expired > it->lifetime)
			enemies.erase(it);
	}
}

void EnemyGenerator::draw()
{
	vector<Enemy>::iterator it;
	for(it = enemies.begin(); it < enemies.end(); it++)
	{
		it->draw();
	}
}

boost::shared_ptr<vector<Enemy> > EnemyGenerator::collideWithBlob()
{
	boost::shared_ptr< vector<Enemy> > coll (new vector<Enemy>());
	
	vector<Enemy>::iterator it;
	for(it = enemies.begin(); it < enemies.end(); it++)
	{
		if(insidePolygon(it->pos, *(gs->blob)))
			coll->push_back(*it);
	}
	
	return coll;
}

Enemy::Enemy(GameState* _gs, float lt, Rand* rand)
{
	lifetime = lt;
	expired = .0f;
	gs = _gs;
	type = rand->nextInt(0,6) == 1 ? GOOD : BAD;
	
	
	do{
		pos = Vec2f(rand->nextFloat(gs->centroid->x-500, gs->centroid->x+500), rand->nextFloat(gs->centroid->y-500, gs->centroid->y+500));
	}while( insidePolygon(pos, *(gs->blob)) );
	
	vel = Vec2f(rand->nextFloat(-.4f, .4f), rand->nextFloat(-.4f, .4f));
	
}

void Enemy::update(float dt)
{
	expired += dt;
	pos += vel;
}

void Enemy::draw()
{
	glPushMatrix();
	
	gl::translate(pos);
	if(type == GOOD)
		gl::color(Color(.7f, .9f, 1.0f));
	else
		gl::color(Color(1.0f, .1f, 1.0f));
	gl::drawSolidCircle(Vec2f(.0f, .0f), 10.0f, 32);
	
	glPopMatrix();
}