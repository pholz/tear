#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/Camera.h"
#include "cinder/CinderMath.h"
#include "EnemyGenerator.h"
#include "WiiMgr.h"
#include "cinder/Thread.h"
#include "cinder/gl/Texture.h"
#include "cinder/Text.h"
#include "cinder/audio/Output.h"
#include "cinder/audio/Io.h"
#include "cinder/audio/Callback.h"
#include "Resources.h"
#include <sstream>
#include "OscListener.h"

using namespace ci;
using namespace ci::app;
using namespace std;





class tearApp : public AppBasic {
	
private:
	PolyLine<Vec2f> *blob;
	vector<Vec2f> dirs;
	vector<Vec2f> origdirs;
	Vec2f *centroid;
	float tug[4];
	float spin[4];
	float last;
	CameraOrtho cam;
	static const float TUGSC;
	EnemyGenerator* egen;
	WiiMgr* wiim;
	bool debug;
	boost::shared_ptr<vector<Enemy*> > colliding;
	boost::shared_ptr<vector<cornerCollision> > collidingCorners;
	
	float score, life;
	float iscore[4];
	float sumdist;
	
	bool b_endgame, b_firstrun;
	int lastwinner;
	
	Font helv;
	
	audio::SourceRef au_uglyup, au_hurt, au_bg, au_feed;
	audio::TrackRef ref;
	
	gl::Texture scoreTexture, endgameTexture;
	
	float wiitug[4][500];
	bool tugged[4];
	float tugctr[4];
	
	shared_ptr< audio::Callback<tearApp, float> > cb1, cb2, cb3;
	
	float mFreqTarget, mFreqTargetQ, mFreqTargetP;
	float mPhase, mPhaseQ, mPhaseP;
	float mPhaseAdjust, mPhaseAdjustQ, mPhaseAdjustP;
	float mMaxFreq, mMaxFreqQ, mMaxFreqP;
	
	float enableQ, enableGoodQ, enablePointQ;
	
	osc::Listener listener;
	float osc_x[4], osc_y[4];
	Vec2f osc_irvec[4];
	bool osc_btn_a[4];
	
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
	void endgame(int winner);
	void shutdown();
	void squareWave( uint64_t inSampleOffset, uint32_t ioSampleCount, audio::Buffer32f *ioBuffer );
	void goodWave( uint64_t inSampleOffset, uint32_t ioSampleCount, audio::Buffer32f *ioBuffer );
	void pointWave( uint64_t inSampleOffset, uint32_t ioSampleCount, audio::Buffer32f *ioBuffer );
	void oscUpdate();
};

void fooo()
{
	printf("FOOFOO");
	
}

void tearApp::prepareSettings(Settings* settings)
{
	settings->setWindowSize(1024, 768);
//	wiim = new WiiMgr();
	b_firstrun = true;
	lastwinner = -1;
	
	cb1 = audio::createCallback( this, &tearApp::squareWave );
	cb2 = audio::createCallback( this, &tearApp::goodWave );
	cb3 = audio::createCallback( this, &tearApp::pointWave );
	
	audio::Output::play( cb1 );
	audio::Output::play( cb2 );
	audio::Output::play( cb3 );
	
	// audio
	
	mMaxFreq = 2000.0f;
	mFreqTarget = 0.0f;
	mPhase = 0.0f;
	mPhaseAdjust = 0.0f;
	
	mMaxFreqQ = 2000.0f;
	mFreqTargetQ = 0.0f;
	mPhaseQ = 0.0f;
	mPhaseAdjustQ = 0.0f;
	
	mMaxFreqP = 2000.0f;
	mFreqTargetP = 0.0f;
	mPhaseP = 0.0f;
	mPhaseAdjustP = 0.0f;
	enableQ = enableGoodQ = enablePointQ = .0f;
}

void tearApp::setup()
{
	/*
	au_uglyup = audio::load( loadResource( RES_UGLYUP ) );
	au_feed = audio::load( loadResource( RES_FEED ) );
	au_bg = audio::load( loadResource( RES_BG ) );
	au_hurt = audio::load( loadResource( RES_HURT ) );
	
	ref = audio::Output::addTrack(au_bg, true);
	ref->setLooping(true);
	 */
	
	listener.setup(3000);
	
	b_endgame = false;
	debug = false;
	score = .0f;
	life = 100.0f;
	
	helv = Font("Helvetica", 24);
	
	blob = new PolyLine<Vec2f>();
	blob->setClosed(true);
	blob->push_back(Vec2f(20.0f, 50.0f));
	blob->push_back(Vec2f(40.0f, 50.0f));
	blob->push_back(Vec2f(40.0f, 100.0f));
	blob->push_back(Vec2f(20.0f, 100.0f));
	
	origdirs = vector<Vec2f>();
	origdirs.push_back(Vec2f(-1.0f, -1.0f));
	origdirs.push_back(Vec2f(1.0f, -1.0f));
	origdirs.push_back(Vec2f(1.0f, 1.0f));
	origdirs.push_back(Vec2f(-1.0f, 1.0f));
	
	dirs = vector<Vec2f>();
	dirs.assign(origdirs.begin(), origdirs.end());
	
	//colliding = NULL;
	
	
	for(int i = 0; i < 4; i++)
	{
		tug[i] = .0f;
		tugctr[i] = .0f;
		tugged[i] = false;
		iscore[i] = .0f;
		spin[i] = .0f;
		osc_x[i] = .0f;
		osc_y[i] = .0f;
		osc_btn_a[i] = false;
		osc_irvec[i] = Vec2f(0, 0);
	}
	
	for(int j = 0; j < 4; j++)
		for(int i = 0; i < 500; i++)
			wiitug[j][i] = .0f;
	
	last = getElapsedSeconds();
	
	cam = CameraOrtho(0, getWindowWidth(), getWindowHeight(), 0, 0, 1000);
	cam.lookAt(Vec3f(getWindowWidth()/2, getWindowHeight()/2, .0f));
	
	setFrameRate(60.0f);
	
	gs = new GameState();
	
	centroid = new Vec2f(.0f, .0f);
	
	gs->blob = blob;
	gs->centroid = centroid;
	
	egen = new EnemyGenerator(gs, 1.0f, 18.0f);
	
//	if(b_firstrun) wiim->go();
	
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
	if(b_endgame && event.getChar() == ' ')
	{
		b_endgame = false;
		this->setup();
	}
	
	if(event.getChar() == 'g')
	{
		b_firstrun = false;
		b_endgame = false;
		this->setup();
	}
	
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
	
	if( event.getChar() == 'x' ){
		spin[3] += 3;
	}
	if( event.getChar() == 'v' ){
		spin[3] -= 3;
	}
}

void tearApp::oscUpdate()
{
	while (listener.hasWaitingMessages()) {
		osc::Message message;
		listener.getNextMessage(&message);
		
		string addr = message.getAddress();
		
		if((addr == "/wii/1/ir" ||
			addr == "/wii/2/ir" ||
			addr == "/wii/3/ir" ||
			addr == "/wii/4/ir" ) && message.getNumArgs() == 2)
		{
			try {
				
				
				int num = (int) addr[5] - (int) '0';
				
				osc_x[num] = message.getArgAsFloat(0);
				osc_y[num] = message.getArgAsFloat(1);
				
				osc_irvec[num] = *centroid + Vec2f( (osc_x[num] - .5f) * getWindowWidth(), -(osc_y[num] - .5f) * getWindowHeight());
				
				console() << num << ": " << osc_x[num] << " / " << osc_y[num] << endl;
				
			} catch (...) {
				console() << "Exception reading argument as float" << std::endl;
			}
		}
		else if((addr == "/wii/1/button/A" ||
				   addr == "/wii/2/button/A" ||
				   addr == "/wii/3/button/A" ||
				   addr == "/wii/4/button/A" ) && message.getNumArgs() == 1)
		{
			
			try {
				
				
				int num = (int) addr[5] - (int) '0';
				
				if(message.getArgAsInt32(0)) osc_btn_a[num] = true;
				else						 osc_btn_a[num] = false;
				
				console() << num << ": A " << (osc_btn_a[num] ? "on" : "off") << endl;
				
			} catch (...) {
				console() << "Exception reading argument as int" << std::endl;
			}
		}
	}
}

void tearApp::update()
{
//	if(!ref->isPlaying())
//		ref->play();
	
	oscUpdate();
	
	if(life <= .0f && !b_endgame)
		endgame(-1);
	
	for(int i = 0; i < 4; i++)
	{
		if(iscore[i] >= 100.0f && !b_endgame)
			endgame(i);
	}
	
	
	float now = getElapsedSeconds();
	float dt = now - last;
	last = now;
	
	life -= dt;
	
	//dirs.clear();
	// move corners according to tug (indirectly and directly)
	for(int i = 0; i < 4; i++)
	{
		dirs[i] = (osc_irvec[i] - blob->getPoints()[i]).normalized();
		
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
	
	sumdist = .0f;
	
	// get sum of distances of corner points to centroid
	for(int i = 0; i < 4; i++)
	{
		sumdist += blob->getPoints()[i].distance(*centroid);
	}
	
	if(sumdist > 600.0f)
	{
		for(int i = 0; i < 4; i++)
		{
			tug[i] += 1.0f;
		}
		endgame(-2);
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
	
	enableQ = enableGoodQ = enablePointQ = .0f;
	if(colliding)
	{
		vector<Enemy*>::iterator it;
		for(it = colliding->begin(); it < colliding->end(); it++)
		{
			if((*it)->type == GOOD) 
			{
				score += .001;
				life += 3 * dt;
				enableGoodQ = .2f;
			}
			else if((*it)->type == BAD) 
			{
				score = math<float>::max(score - .005, .0f);
				life -= dt;
				enableQ = .05f;
			}
		}
	}
	
	
	collidingCorners = egen->collideWithCorners();
	if(collidingCorners)
	{
		vector<cornerCollision>::iterator it;
		for(it = collidingCorners->begin(); it < collidingCorners->end(); it++)
		{
			iscore[it->corner] = math<float>::min(iscore[it->corner] + dt * 4.0f, 100.0f);
			enablePointQ = .2f;
		}
	}
	
	/*
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
		
		
		// ROLL
//		spin[j] = 2*M_PI*(float)((int)(getElapsedSeconds() * 1000.0f) % 1000)/1000.0f;
		spin[j] = .0f;
		
	}
	 */
	for(int j = 0; j < 4; j++)
		if(hasTugged(j)) tug[j] += TUGSC * 0.5f;
	
	
	TextLayout simple;
	simple.setFont( helv );
	simple.setColor( Color( 1.0f, 1.0f, 1.0f ) );
	stringstream ss;
	ss << "score: ";
	ss << ((int) score);
	ss << " -- life: ";
	ss << ((int) life);
	simple.addLine( ss.str() );
	scoreTexture = gl::Texture( simple.render( false, false ) );
	
	mFreqTargetQ = 200.0f;
	mFreqTargetP = 400.0f;
	mFreqTarget = 300.0f;
}

bool tearApp::hasTugged(int id)
{
		
	return osc_btn_a[id];
	
	
	
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
	
	if(!(b_endgame && lastwinner == -2)){
		// draw blob lines
		
		glLineWidth(3.0f);
		if(sumdist > 250.0f)
		{
			int lvl = (int) (sumdist/100.0f);
			glLineStipple((int) (sumdist/100.0f), lvl < 4 ? 0xAAAA : 0x8888);
			glEnable(GL_LINE_STIPPLE);
		}
		gl::color(Color(1.0f, .0f, .0f));
		gl::draw(*blob);
		glDisable(GL_LINE_STIPPLE);
		glLineWidth(1.0f);
	
	// draw inner area
	
	
		gl::color(ColorA(1.0f - life/100.0f, life/100.0f, .0f, .5f));
		
		glBegin(GL_TRIANGLE_FAN);
		
		gl::vertex(*centroid);
		
		for(int i = 0; i < 4; i++)
		{
			Vec2f& pt = blob->getPoints()[i];
			
			gl::vertex(pt);
		}
		
		gl::vertex(blob->getPoints()[0]);
				   
		glEnd();
		
	}
	
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
		
		
		
		
		float step = 2 * M_PI / 10.0f;
		float maxscore = 100.0f;
		for(float rad = .0f; rad < 2*M_PI*iscore[i]/maxscore; rad += step)
		{
			if(rad >= 2*M_PI*iscore[i]/maxscore - step)
				gl::color(ColorA(.7f, .7f, .0f, (float) ((int)iscore[i] % 10) / 10.0f ));
			else
				gl::color(ColorA(.7f, .7f, .0f, 1.0f ));
			glPushMatrix();
			gl::translate(Vec2f(math<float>::cos(rad) * 20.0f, -math<float>::sin(rad) * 20.0f));
			gl::drawSolidCircle(Vec2f(.0f, .0f), 3.0f, 16);
			glPopMatrix();
		}
		
		
		
		
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
		vector<Enemy*>::iterator it;
		for(it = colliding->begin(); it < colliding->end(); it++)
		{
			glPushMatrix();
			
			gl::translate((*it)->pos);
			gl::color(Color(1.0f, 1.0f, 1.0f));
			gl::drawStrokedCircle(Vec2f(.0f, .0f), 15.0f, 32);
			
			glPopMatrix();
		}
	}
	
	if(collidingCorners)
	{
		vector<cornerCollision>::iterator it;
		for(it = collidingCorners->begin(); it < collidingCorners->end(); it++)
		{
			glPushMatrix();
			
			Vec2f& pt = blob->getPoints()[it->corner];
			
			gl::translate(pt);
			gl::color(Color(1.0f, 1.0f, 1.0f));
			gl::drawStrokedCircle(Vec2f(.0f, .0f), 25.0f, 32);
			
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
	
	if(b_endgame)
	{
		gl::draw( endgameTexture, Vec2f( cam.getEyePoint().x + getWindowWidth()/2 - endgameTexture.getWidth()/2, cam.getEyePoint().y + getWindowHeight()/2 ) );
	}
	
	// cursors
	
	for(int i = 0; i < 4; i++)
	{
		//if(wiim->lox.timed_lock(boost::get_system_time()+boost::posix_time::milliseconds(10)))
		{
			//int x = wiim->a_ir_x[i];
			//int y = wiim->a_ir_y[i];
			
			//wiim->lox.unlock();
			
			gl::color(Color(1.0f, 1.0f, 1.0f));
			
			gl::drawSolidCircle(osc_irvec[i], 10.0f, 16);
		}
		
		
	}
	
}

void tearApp::endgame(int winner)
{
	b_endgame = true;
	b_firstrun = false;
	lastwinner = winner;
	
	TextLayout simple;
	simple.setFont( helv );
	simple.setColor( Color( 1.0f, 1.0f, 1.0f ) );
	stringstream ss;
	
	if(winner >= 0){
		ss << "game over - player ";
		ss << (winner+1);
		ss << " won. collective score: ";
		ss << ((int) score);
	} else if(winner == -1){
		ss << "game over - you died. collective score: ";
		ss << ((int) score);
	} else if(winner == -2){
		ss << "game over - you were torn apart. collective score: ";
		ss << ((int) score);
	}
	
	simple.addLine( ss.str() );
	endgameTexture = gl::Texture( simple.render( false, false ) );
	
}

void tearApp::shutdown()
{
	delete gs;
	delete egen;
	delete wiim;
	delete centroid;
	delete blob;
	
	
}

void tearApp::squareWave( uint64_t inSampleOffset, uint32_t ioSampleCount, audio::Buffer32f *ioBuffer ) 
{
	mPhaseAdjustQ = mPhaseAdjustQ * 0.95f + ( mFreqTargetQ * 1.5f / 44100.0f ) * 0.05f;
	for( int  i = 0; i < ioSampleCount; i++ ) {
		mPhaseQ += mPhaseAdjustQ;
		mPhaseQ = mPhaseQ - math<float>::floor( mPhaseQ );
		float val = enableQ * math<float>::signum(math<float>::sin( mPhaseQ * 2.0f * M_PI ));
		
		ioBuffer->mData[i*ioBuffer->mNumberChannels] = val;
		ioBuffer->mData[i*ioBuffer->mNumberChannels + 1] = val;
	}
}

void tearApp::goodWave( uint64_t inSampleOffset, uint32_t ioSampleCount, audio::Buffer32f *ioBuffer ) 
{
	mPhaseAdjust = mPhaseAdjust * 0.95f + ( mFreqTarget * 1.5f / 44100.0f ) * 0.05f;
	for( int  i = 0; i < ioSampleCount; i++ ) {
		mPhase += mPhaseAdjust;
		mPhase = mPhase - math<float>::floor( mPhase );
		float val = enableGoodQ * math<float>::sin( mPhase * 2.0f * M_PI );
		
		ioBuffer->mData[i*ioBuffer->mNumberChannels] = val;
		ioBuffer->mData[i*ioBuffer->mNumberChannels + 1] = val;
	}
}

void tearApp::pointWave( uint64_t inSampleOffset, uint32_t ioSampleCount, audio::Buffer32f *ioBuffer ) 
{
	mPhaseAdjustP = mPhaseAdjustP * 0.95f + ( mFreqTargetP * 1.5f / 44100.0f ) * 0.05f;
	for( int  i = 0; i < ioSampleCount; i++ ) {
		mPhaseP += mPhaseAdjustP;
		mPhaseP = mPhaseP - math<float>::floor( mPhaseP );
		float val = enablePointQ * math<float>::sin( mPhaseP * 2.0f * M_PI );
		
		ioBuffer->mData[i*ioBuffer->mNumberChannels] = val;
		ioBuffer->mData[i*ioBuffer->mNumberChannels + 1] = val;
	}
}

const float tearApp::TUGSC = 2.8f;

CINDER_APP_BASIC( tearApp, RendererGl )
