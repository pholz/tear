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
#include "Resources.h"

#include "util.h"
#define VEL 1.0f

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

EnemyGenerator::~EnemyGenerator()
{
	delete rand;
}

void EnemyGenerator::update(float dt)
{
	last = total;
	total += dt;
	acc += dt;
	
	if(acc >= interval)
	{
		acc = 0;
		Enemy *e = new Enemy(gs, lifetime, rand);
		enemies.push_back(e);
		
	}
	
	vector<Enemy*>::iterator it;
	for(it = enemies.begin(); it < enemies.end(); it++)
	{
		(*it)->update(dt);
		if((*it)->expired > (*it)->lifetime)
		{
			delete *it;
			enemies.erase(it);
		}
			
	}
}

void EnemyGenerator::draw()
{
	vector<Enemy*>::iterator it;
	for(it = enemies.begin(); it < enemies.end(); it++)
	{
		(*it)->draw();
	}
}

boost::shared_ptr<vector<Enemy*> > EnemyGenerator::collideWithBlob()
{
	boost::shared_ptr< vector<Enemy*> > coll (new vector<Enemy*>());
	
	vector<Enemy*>::iterator it;
	for(it = enemies.begin(); it < enemies.end(); it++)
	{
		if(insidePolygon((*it)->pos, *(gs->blob)))
			coll->push_back(*it);
	}
	
	return coll;
}

boost::shared_ptr< vector<cornerCollision> > EnemyGenerator::collideWithCorners()
{
	boost::shared_ptr< vector<cornerCollision> > coll (new vector<cornerCollision>());
	
	vector<Enemy*>::iterator it;
	for(it = enemies.begin(); it < enemies.end(); it++)
	{
		if((*it)->type == UGLY)
		{
			for(int i = 0; i < 4; i++)
			{
				Vec2f& pt = gs->blob->getPoints()[i];
				if((*it)->pos.distance(pt) < 50.0f)
				{
					cornerCollision c;
					c.enemy = (*it);
					c.corner = i;
					c.collide_it = it;
					coll->push_back(c);
				}
			}
		}
	}
	
	return coll;
}

Enemy::Enemy(GameState* _gs, float lt, Rand* rand)
{
	lifetime = lt;
	expired = .0f;
	gs = _gs;
	int ty = rand->nextInt(0,10);
	this->rand = rand;
	
	if(ty < 3) type = BAD;
	else if(ty < 8) type = UGLY;
	else type = GOOD;
	

		r0 = rand->nextFloat(4.0f,10.0f);
		r1 = rand->nextFloat(4.0f,10.0f);
		r2 = rand->nextFloat(4.0f,10.0f);
		speed = rand->nextFloat(25.0f,55.0f);

	
	do{
		pos = Vec2f(rand->nextFloat(gs->centroid->x-500, gs->centroid->x+500), rand->nextFloat(gs->centroid->y-500, gs->centroid->y+500));
	}while( insidePolygon(pos, *(gs->blob)) );
	
	vel = Vec2f(rand->nextFloat(-VEL, VEL), rand->nextFloat(-VEL, VEL));
	
}

Enemy::~Enemy()
{
	
}

void Enemy::update(float dt)
{
	expired += dt;
	pos += vel;
}

void Enemy::draw()
{
	float alphaa = 1.0f;
	
	if(expired > lifetime - 5.0f)
		alphaa = math<float>::min(math<float>::max(math<float>::sin(expired*20.0f)/2.0f + .5f, .0f), 1.0f);
	
	glPushMatrix();
	
	gl::translate(pos);
	if(type == GOOD)
		gl::color(ColorA(.7f, .9f, 1.0f, alphaa));
	else if(type == BAD)
		gl::color(ColorA(1.0f, .1f, .0f, alphaa));
	else
		gl::color(ColorA(1.0f, 1.0f, .4f, alphaa));
	
	if(type == BAD)
	{
		/*
		
		 */
		//glEnable(GL_CULL_FACE);
//		glCullFace(GL_BACK);
//		gl::enableWireframe();
		
		gl::rotate( Vec3f(M_PI/2 + expired*speed, M_PI/2 + expired*speed*2.0f, M_PI/2 + expired*speed*.5f));
		float scalevbo = 7.0f;
		gl::scale(Vec3f(scalevbo, scalevbo, scalevbo));
		if(Enemy::mVBOBad)
			gl::draw( *Enemy::mVBOBad );
		
		//gl::disableWireframe();
//		glDisable(GL_CULL_FACE);
		
	} else if(type == GOOD){
		gl::rotate( M_PI/2 + expired*speed);
		gl::translate(Vec2f(-13.0f,-5.0f));
		gl::drawSolidCircle(Vec2f(.0f, .0f), r0, 32);
		gl::translate(Vec2f(13.0f,-5.0f));
		gl::drawSolidCircle(Vec2f(.0f, .0f), r1, 32);
		gl::translate(Vec2f(.0f,13.0f));
		gl::drawSolidCircle(Vec2f(.0f, .0f), r2, 32);
	} else {
		gl::drawStrokedCircle(Vec2f(.0f, .0f), 8.0f, 32);
		gl::drawSolidCircle(Vec2f(.0f, .0f), 4.0f, 32);
	}
	
	
	glPopMatrix();
	
	
}

TriMesh* Enemy::mMeshBad = NULL;
gl::VboMesh* Enemy::mVBOBad = NULL;