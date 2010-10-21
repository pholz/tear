#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/Camera.h"
#include "cinder/CinderMath.h"
#include "EnemyGenerator.h"
#include "WiiMgr.h"
#include "cinder/Thread.h"

using namespace ci;
using namespace ci::app;
using namespace std;





class tearApp : public AppBasic {
	
private:
	PolyLine<Vec2f> *blob;
	vector<Vec2f> dirs;
	Vec2f *centroid;
	float tug[4];
	float last;
	CameraOrtho cam;
	static const float TUGSC;
	EnemyGenerator* egen;
	WiiMgr* wiim;
	
	float wiitug[500];
	bool tugged;
	float tugctr;
	
public:
	
	GameState* gs;
	
	void prepareSettings(Settings* settings);
	void setup();
	void mouseDown( MouseEvent event );	
	void keyDown( KeyEvent event );
	void update();
	void draw();
	void foo();
	bool hasTugged();
};

void fooo()
{
	printf("FOOFOO");
}

void tearApp::prepareSettings(Settings* settings)
{
	settings->setWindowSize(1024, 768);
	wiim = new WiiMgr();
}

void tearApp::setup()
{
	blob = new PolyLine<Vec2f>();
	blob->setClosed(true);
	blob->push_back(Vec2f(20.0f, 50.0f));
	blob->push_back(Vec2f(40.0f, 50.0f));
	blob->push_back(Vec2f(40.0f, 100.0f));
	blob->push_back(Vec2f(20.0f, 100.0f));
	
	dirs = vector<Vec2f>();
	dirs.push_back(Vec2f(-1.0f, -1.0f));
	dirs.push_back(Vec2f(1.0f, -1.0f));
	dirs.push_back(Vec2f(1.0f, 1.0f));
	dirs.push_back(Vec2f(-1.0f, 1.0f));
	
	tugged = false;
	tugctr = .0f;
	
	for(int i = 0; i < 4; i++)
		tug[i] = .0f;
	
	for(int i = 0; i < 500; i++)
		wiitug[i] = .0f;
	
	last = .0f;
	
	cam = CameraOrtho(0, getWindowWidth(), getWindowHeight(), 0, 0, 1000);
	cam.lookAt(Vec3f(getWindowWidth()/2, getWindowHeight()/2, .0f));
	
	setFrameRate(60.0f);
	
	gs = new GameState();
	
	centroid = new Vec2f(.0f, .0f);
	
	gs->blob = blob;
	gs->centroid = centroid;
	
	egen = new EnemyGenerator(gs, 1.0f, 18.0f);
	
	wiim->go();
}

void tearApp::foo()
{
	printf("foo");
}

void tearApp::mouseDown( MouseEvent event )
{
}

void tearApp::keyDown( KeyEvent event )
{
	if( event.getChar() == 'q' ){
		tug[0] += TUGSC;
	}
	if( event.getChar() == 'p' ){
		tug[1] += TUGSC;
	}
	if( event.getChar() == 'm' ){
		tug[2] += TUGSC;
	}
	if( event.getChar() == 'c' ){
		tug[3] += TUGSC;
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
		blob->getPoints()[i] += dirs[i] * tug[i];
		
		for(int j = 0; j < 4; j++)
		{
			if(j == i) continue;
			blob->getPoints()[i] += dirs[j] * tug[j] * 0.3f;
			
		}
		
		
	}
	
	centroid->x = .0f;
	centroid->y = .0f;
	
	for(int i = 0; i < 4; i++)
	{
		*centroid += blob->getPoints()[i];
	}
	
	*centroid /= 4.0f;
	
	float sumdist = .0f;
	
	for(int i = 0; i < 4; i++)
	{
		sumdist += blob->getPoints()[i].distance(*centroid);
	}
	
	for(int i = 0; i < 4; i++)
	{
		Vec2f& pt = blob->getPoints()[i];
		
		
		if(pt.distance(*centroid) > 55.0f)
		{
			pt += (*centroid - pt).normalized() * ( TUGSC / 2.0f ) * sumdist/300.0f;
		}
	}
	
	egen->update(dt);
	
	//wiim->update();
	
	for(int i = 0; i < 499; i++)
	{
		wiitug[i] = wiitug[i+1];
	}
	
	wiitug[499] = wiim->state_pitch;
	
	if(tugctr > .0f) tugctr -= dt;
	if(tugctr < .0f) tugctr = .0f;
	
	tugged = hasTugged();
	if(tugged) tug[2] += TUGSC;
	
}

bool tearApp::hasTugged()
{
	float max1 = .0f, min = .0f, max2 = .0f;
	int mindex = 0;
	
	bool spikefound = false;
	/*
	for(int i = 498; i >= 430; i--)
	{
		if(math<float>::abs(max1 - min) > 30.0f) spikefound = true;
		
		if(!spikefound){
			max1 = math<float>::max(max1, wiitug[i]);
			min = math<float>::min(min, wiitug[i]);
		} else {
			if(wiitug[i] < wiitug[i+1] - 3.0f) break;
			max2 = math<float>::max(max2, wiitug[i]);
		}
	}
	
	if(tugctr == .0f && max1 > .0f && max2 > .0f && min < -10.0f && spikefound && math<float>::abs(max1-max2) < 10.0f && math<float>::abs(max1-min) > 25.0f)
	{
		tugctr = 1.0f;
		return true;
		
	}
	else
		return false;
	 */
	
	for(int i = 450; i < 500; i++)
	{
		min = math<float>::min(min, wiitug[i]);
		mindex = i;
	}
		
	
	if(min < -20.0f)
	{
		for(int i = mindex; i < math<int>::min(mindex+30,500); i++)
		{
			max1 = math<float>::max(max1, wiitug[i]);
		}
		
		for(int i = mindex; i > math<int>::max(mindex-30,450); i--)
		{
			max2 = math<float>::max(max1, wiitug[i]);
		}
		
		if(tugctr == .0f && max1 > .0f && max2 > .0f && math<float>::abs(max1-max2) < 20.0f)
		{
			tugctr = 1.0f;
			return true;
			
		}
	}
	
	return false;
	
}

void tearApp::draw()
{
	cam.lookAt(Vec3f(centroid->x - getWindowWidth()/2, centroid->y - getWindowHeight()/2, 10.0f), Vec3f(centroid->x - getWindowWidth()/2, centroid->y - getWindowHeight()/2, .0f));
	gl::setMatrices(cam);
	
	gl::clear( Color( 0, 0, 0 ) ); 
	
	for(int i = 0; i < 100; i++)
		for(int j = 0; j < 100; j++)
		{
			if(centroid->distance(Vec2f(i*100-5000, j*100-5000)) > getWindowWidth() * 2) continue;
			
			glPushMatrix();
			gl::translate(Vec2f(i * 100 - 5000, j * 100 - 5000));
			gl::color(Color(.3f, .3f, .3f));
			gl::drawLine(Vec2f(-5.0f, -5.0f), Vec2f(5.0f, 5.0f));
			gl::drawLine(Vec2f(5.0f, -5.0f), Vec2f(-5.0f, 5.0f));
			glPopMatrix();
		}
	
	// clear out the window with black
	
	gl::color(Color(1.0f, .0f, .0f));
	gl::draw(*blob);
	
	for(int i = 0; i < 4; i++)
	{
		Vec2f& pt = blob->getPoints()[i];
		glPushMatrix();
		
		gl::translate(pt);
		
		gl::color(Color(1.0f, .0f, .0f));
		gl::drawStrokedCircle(Vec2f(.0f, .0f), 5.0f, 32);
		gl::drawVector(Vec3f(.0f, .0f, .0f), Vec3f(dirs[i].x, dirs[i].y, .0f) * 10);
		
		gl::color(Color(.7f, .7f, .0f));
		gl::drawSolidCircle(Vec2f(.0f, .0f), 4.8f, 32);
		
		glPopMatrix();
	}
	
	glPushMatrix();
	
	gl::translate(*centroid);
	
	gl::color(Color(1.0f, .0f, .0f));
	gl::drawSolidCircle(Vec2f(.0f, .0f), 5.0f, 32);
	
	glPopMatrix();
	
	egen->draw();
	
	glPushMatrix();
	gl::translate(Vec2f(cam.getEyePoint().x, cam.getEyePoint().y+getWindowHeight()/2));
	gl::color(Color(.0f, 1.0f, .0f));
	glBegin(GL_LINE_STRIP);
	for(int i = 0; i < 500; i++)
		glVertex2f(i, -wiitug[i]*2);
	glEnd();
	glPopMatrix();
	
//	if(tugged)
//	{
//		gl::drawSolidCircle(Vec2f(.0f, .0f), 30.0f, 32);
//	}
	
	
}

const float tearApp::TUGSC = 1.8f;

CINDER_APP_BASIC( tearApp, RendererGl )
