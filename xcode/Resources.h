/*
 *  Resources.h
 *  tear
 *
 *  Created by Peter Holzkorn on 28.10.10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once
#include "cinder/CinderResources.h"

#define RES_UGLYUP	CINDER_RESOURCE( ../resources/, s_uglyup.wav, 128, WAV )
#define RES_FEED	CINDER_RESOURCE( ../resources/, s_feed.wav, 129, WAV )
#define RES_BG		CINDER_RESOURCE( ../resources/, s4_bg.wav, 130, WAV )
#define RES_HURT	CINDER_RESOURCE( ../resources/, s_hurt.wav, 131, WAV )
#define RES_SPIKEBALL_OBJ		CINDER_RESOURCE( ../resources/, spikeball.obj, 132, DATA )
#define RES_VERT_GLSL		CINDER_RESOURCE( ../resources/, blur_vert.glsl, 133, GLSL )
#define RES_FRAG_GLSL		CINDER_RESOURCE( ../resources/, blur_frag.glsl, 134, GLSL )