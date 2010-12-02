/*
 *  EnemyGenerator.h
 *  tear
 *
 *  Created by Peter Holzkorn on 17.10.10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

//#include "common.h"

#include "GameState.h"
#include <vector>
#include "cinder/Rand.h"
#include "cinder/gl/Vbo.h"

using namespace ci;
using namespace std;
//using namespace boost;

enum enemy_type {
	GOOD,
	BAD,
	UGLY,
	DECO
};



class Enemy {
private:
public:
	Vec2f pos, vel;
	GameState* gs;
	float lifetime, expired;
	enemy_type type;
	Rand* rand;
	float r0, r1, r2, speed;
	
	static TriMesh*			mMeshBad;
	static gl::VboMesh*		mVBOBad;
	
	Enemy(GameState* gs, float lt, Rand* r);
	~Enemy();
	
	void update(float dt);
	void draw();
};

typedef struct 
{
	int corner;
	Enemy* enemy;
	vector<Enemy*>::iterator collide_it;
} cornerCollision;

class EnemyGenerator {
private:
	
	vector<Enemy*> uglies;
	GameState* gs;
	float interval, lifetime;
	float last, acc, total;
	Rand* rand;
	
public:
	vector<Enemy*> enemies;
	
	EnemyGenerator(GameState* gs, float interval, float lifetime);
	~EnemyGenerator();
	
	boost::shared_ptr< vector<Enemy*> > collideWithBlob();
	boost::shared_ptr< vector<cornerCollision> > collideWithCorners();
	void update(float dt);
	void draw();
};