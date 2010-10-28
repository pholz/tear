#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/Camera.h"
#include "cinder/CinderMath.h"
#include "EnemyGenerator.h"
#include "WiiMgr.h"
#include "cinder/Thread.h"
#include "cinder/gl/Texture.h"
#include "cinder/Text.h"
#include <sstream>

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
	bool debug;
	boost::shared_ptr<vector<Enemy> > colliding;
	float score;
	Font helv;
	
	gl::Texture scoreTexture;
	
	float wiitug[4][500];
	bool tugged[4];
	float tugctr[4];
	
public:
	
	GameState* gs;
	
	void prepareSettings(Settings* settings);
	void setup();
	void mouseDown( MouseEvent event );	
	void keyDown( KeyEvent event );
	void update();
	void draw();
	void foo();
	bool hasTugged(int id);
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
	debug = false;
	score = .0f;
	
	helv = Font("Helvetica", 24);
	
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
	
	//colliding = NULL;
	
	
	for(int i = 0; i < 4; i++)
	{
		tug[i] = .0f;
		tugctr[i] = .0f;
		tugged[i] = false;
	}
	
	for(int j = 0; j < 4; j++)
		for(int i = 0; i < 500; i++)
			wiitug[j][i] = .0f;
	
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
	
	TextLayout simple;
	simple.setFont( Font( "Helvetica", 24 ) );
	simple.setColor( Color( 1.0f, 1.0f, 1.0f ) );
	simple.addLine( "score: 0" );
	scoreTexture = gl::Texture( simple.render( true, false ) );
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
	if( event.getChar() == 'd' ){
		debug = !debug;
	}
}

void tearApp::update()
{
	
	
	float now = getElapsedSeconds();
	float dt = now - last;
	last = now;
	
	// move corners according to tug (indirectly and directly)
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
	
	// calc entroid
	for(int i = 0; i < 4; i++)
	{
		*centroid += blob->getPoints()[i];
	}
	
	*centroid /= 4.0f;
	
	float sumdist = .0f;
	
	// get sum of distances of corner points to centroid
	for(int i = 0; i < 4; i++)
	{
		sumdist += blob->getPoints()[i].distance(*centroid);
	}
	
	// add "elastic" force to corners, proportional to "tension" in the system
	for(int i = 0; i < 4; i++)
	{
		Vec2f& pt = blob->getPoints()[i];
		
		
		if(pt.distance(*centroid) > 55.0f)
		{
			pt += (*centroid - pt).normalized() * ( TUGSC / 2.0f ) * sumdist/300.0f;
		}
	}
	
	// update enemy generator
	egen->update(dt);
	
	// collision detection and reaction between enemies and blob
	colliding = egen->collideWithBlob();
	
	if(colliding){
		vector<Enemy>::iterator it;
		for(it = colliding->begin(); it < colliding->end(); it++)
		{
			if(it->type == GOOD) score += .001;
			else				score -= .005;
		}
	}
	
	
	// advance "history" of tugs per wiimote
	for(int j = 0; j < 4; j++)
	{
		for(int i = 0; i < 499; i++)
		{
			wiitug[j][i] = wiitug[j][i+1];
		}
		
		
		wiitug[j][499] = wiim->a_state_pitch[j];
		
		if(tugctr[j] > .0f) tugctr[j] -= dt;
		if(tugctr[j] < .0f) tugctr[j] = .0f;
		
		tugged[j] = hasTugged(j);
		
		// add tug force if a tug has been detected
		if(tugged[j]) tug[j] += TUGSC * 3;
	}
	
	TextLayout simple;
	simple.setFont( helv );
	simple.setColor( Color( 1.0f, 1.0f, 1.0f ) );
	stringstream ss;
	ss << "score: ";
	ss << score;
	simple.addLine( ss.str() );
	scoreTexture = gl::Texture( simple.render( true, false ) );
}

bool tearApp::hasTugged(int id)
{
	// find a minimum, then move to right and left to search for maxima
	// if all has been found, and the "spike" is big enough, and enough time has passed since
	// the last detection, detect a tug
	
	float max1 = .0f, min = .0f, max2 = .0f;
	int mindex = 0;
	
	for(int i = 450; i < 500; i++)
	{
		min = math<float>::min(min, wiitug[id][i]);
		mindex = i;
	}
		
	
	if(min < -20.0f)
	{
		for(int i = mindex; i < math<int>::min(mindex+30,500); i++)
		{
			max1 = math<float>::max(max1, wiitug[id][i]);
		}
		
		for(int i = mindex; i > math<int>::max(mindex-30,450); i--)
		{
			max2 = math<float>::max(max1, wiitug[id][i]);
		}
		
		
		if(tugctr[id] == .0f && max1 > .0f && max2 > .0f && math<float>::abs(max1-max2) < 20.0f)
		{
			tugctr[id] = 1.0f;
			return true;
			
		}
	}
	
	return false;
	
}

void tearApp::draw()
{
	gl::enableAlphaBlending( false );
	
	// set camera to center on centroid
	cam.lookAt(Vec3f(centroid->x - getWindowWidth()/2, centroid->y - getWindowHeight()/2, 10.0f), Vec3f(centroid->x - getWindowWidth()/2, centroid->y - getWindowHeight()/2, .0f));
	gl::setMatrices(cam);
	
	gl::clear( Color( 0, 0, 0 ) ); 
	
	// draw the background grid, total -10000 to 10000, but clip everything outside the frame
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
	
	// draw blob lines
	gl::color(Color(1.0f, .0f, .0f));
	gl::draw(*blob);
	
	// draw corners
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
	
	// draw centroid
	gl::translate(*centroid);
	
	gl::color(Color(1.0f, .0f, .0f));
	gl::drawSolidCircle(Vec2f(.0f, .0f), 5.0f, 32);
	
	glPopMatrix();
	
	// draw enemies
	egen->draw();
	
	// draw collision indicators
	if(colliding){
		vector<Enemy>::iterator it;
		for(it = colliding->begin(); it < colliding->end(); it++)
		{
			glPushMatrix();
			
			gl::translate(it->pos);
			gl::color(Color(1.0f, 1.0f, 1.0f));
			gl::drawStrokedCircle(Vec2f(.0f, .0f), 15.0f, 32);
			
			glPopMatrix();
		}
	}
	
	// draw debug wiimote data feeds
	if(debug)
	{
		for(int k = 0; k < 4; k++)
		{
			glPushMatrix();
			gl::translate(Vec2f(cam.getEyePoint().x, cam.getEyePoint().y+150+k*150));
			gl::color(Color(.0f, 1.0f/(float(k)+1), .21f * (float(k)+1)));
			glBegin(GL_LINE_STRIP);
			for(int i = 0; i < 500; i++)
				glVertex2f(i, -wiitug[k][i]);
			glEnd();
			glPopMatrix();
			
		}
	}
	
	// draw scores
	glColor3f( 1.0f, 1.0f, 1.0f );
	gl::draw( scoreTexture, Vec2f( cam.getEyePoint().x + 10, cam.getEyePoint().y + getWindowHeight() - scoreTexture.getHeight() - 5 ) );
	
	
}

const float tearApp::TUGSC = 1.8f;

CINDER_APP_BASIC( tearApp, RendererGl )
