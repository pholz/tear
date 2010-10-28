/*
 *  EnemyGenerator.h
 *  tear
 *
 *  Created by Peter Holzkorn on 17.10.10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "GameState.h"
#include <vector>
#include "cinder/Rand.h"

using namespace ci;
using namespace std;
//using namespace boost;

enum enemy_type {
	GOOD,
	BAD,
	UGLY
};



class Enemy {
private:
public:
	Vec2f pos, vel;
	GameState* gs;
	float lifetime, expired;
	enemy_type type;
	
	Enemy(GameState* gs, float lt, Rand* r);
	//~Enemy();
	
	void update(float dt);
	void draw();
};

typedef struct 
{
	int corner;
	Enemy* enemy;
} cornerCollision;

class EnemyGenerator {
private:
	vector<Enemy*> enemies;
	vector<Enemy*> uglies;
	GameState* gs;
	float interval, lifetime;
	float last, acc, total;
	Rand* rand;
	
public:
	EnemyGenerator(GameState* gs, float interval, float lifetime);
//	~EnemyGenerator();
	
	boost::shared_ptr< vector<Enemy*> > collideWithBlob();
	boost::shared_ptr< vector<cornerCollision> > collideWithCorners();
	void update(float dt);
	void draw();
};