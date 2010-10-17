#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"

#define TUGSCALE 1.3f;

using namespace ci;
using namespace ci::app;
using namespace std;

class tearApp : public AppBasic {
	
private:
	PolyLine<Vec2f> blob;
	vector<Vec2f> dirs;
	Vec2f centroid;
	float tug[4];
	float last;
	
public:
	void setup();
	void mouseDown( MouseEvent event );	
	void keyDown( KeyEvent event );
	void update();
	void draw();
};

void tearApp::setup()
{
	blob = PolyLine<Vec2f>();
	blob.setClosed(true);
	blob.push_back(Vec2f(20.0f, 50.0f));
	blob.push_back(Vec2f(40.0f, 50.0f));
	blob.push_back(Vec2f(40.0f, 100.0f));
	blob.push_back(Vec2f(20.0f, 100.0f));
	
	dirs = vector<Vec2f>();
	dirs.push_back(Vec2f(-1.0f, -1.0f));
	dirs.push_back(Vec2f(1.0f, -1.0f));
	dirs.push_back(Vec2f(1.0f, 1.0f));
	dirs.push_back(Vec2f(-1.0f, 1.0f));
	
	for(int i = 0; i < 4; i++)
		tug[i] = .0f;
	
	last = .0f;
}

void tearApp::mouseDown( MouseEvent event )
{
}

void tearApp::keyDown( KeyEvent event )
{
	if( event.getChar() == 'q' ){
		tug[0] += TUGSCALE;
	}
	if( event.getChar() == 'p' ){
		tug[1] += TUGSCALE;
	}
	if( event.getChar() == 'm' ){
		tug[2] += TUGSCALE;
	}
	if( event.getChar() == 'c' ){
		tug[3] += TUGSCALE;
	}
}

void tearApp::update()
{
	float now = getElapsedSeconds();
	float dt = now - last;
	last = now;
	
	for(int i = 0; i < 4; i++)
	{
		if(tug[i] > .0f) tug[i] *= .93f;
		blob.getPoints()[i] += dirs[i] * tug[i];
		
		for(int j = 0; j < 4; j++)
		{
			if(j == i) continue;
			blob.getPoints()[i] += dirs[j] * tug[j] * 0.3f;
			
		}
		
		
	}
	
	centroid = Vec2f(.0f, .0f);
	
	for(int i = 0; i < 4; i++)
	{
		centroid += blob.getPoints()[i];
	}
	
	centroid /= 4.0f;
	
	float sumdist = .0f;
	
	for(int i = 0; i < 4; i++)
	{
		sumdist += blob.getPoints()[i].distance(centroid);
	}
	
	for(int i = 0; i < 4; i++)
	{
		Vec2f& pt = blob.getPoints()[i];
		
		
		if(pt.distance(centroid) > 55.0f)
		{
			pt += (centroid - pt).normalized() * (1.3f / 3.0f) * sumdist/300.0f;
		}
	}
	
	
}

void tearApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) ); 
	gl::color(Color(1.0f, .0f, .0f));
	gl::draw(blob);
	
	for(int i = 0; i < 4; i++)
	{
		Vec2f& pt = blob.getPoints()[i];
		glPushMatrix();
		
		gl::translate(pt);
		
		gl::color(Color(1.0f, .0f, .0f));
		gl::drawStrokedCircle(Vec2f(.0f, .0f), 5.0f, 32);
		gl::color(Color(.7f, .7f, .0f));
		gl::drawSolidCircle(Vec2f(.0f, .0f), 4.8f, 32);
		
		glPopMatrix();
	}
	
	gl::color(Color(1.0f, .0f, .0f));
	gl::drawSolidCircle(centroid, 5.0f, 32);
}


CINDER_APP_BASIC( tearApp, RendererGl )
