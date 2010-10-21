/*
 *  GameState.h
 *  tear
 *
 *  Created by Peter Holzkorn on 19.10.10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */
#include "cinder/Cinder.h"
#include "cinder/Vector.h"
#include "cinder/PolyLine.h"

using namespace ci;

typedef struct {
	PolyLine<Vec2f>* blob;
	Vec2f* centroid;
} GameState;