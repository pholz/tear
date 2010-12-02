/*
 *  GameState.h
 *  tear
 *
 *  Created by Peter Holzkorn on 19.10.10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */
#include "common.h"

using namespace ci;

class tearApp;

typedef struct {
	PolyLine<Vec2f>* blob;
	Vec2f* centroid;
	
} GameState;