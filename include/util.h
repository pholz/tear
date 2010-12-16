/*
 *  util.h
 *  tear
 *
 *  Created by Peter Holzkorn on 17.10.10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

bool insidePolygon(Vec2f p, PolyLine<Vec2f>& poly)
{
	bool inside = false;
	for(int i = 0, j = poly.size()-1; i < poly.size(); j = i++)
	{
		Vec2f& p1 = poly.getPoints()[i];
		Vec2f& p2 = poly.getPoints()[j];
		
		if( ( p1.y <= p.y && p.y < p2.y || p2.y <= p.y && p.y < p1.y ) &&
			p.x < (p2.x - p1.x) * (p.y - p1.y) / (p2.y - p1.y) + p1.x )
		{
			inside = !inside;
		}
	}
	
	return inside;
}

