//#include "common.h"
#include "ParticleGen.h"
#include "EnemyGenerator.h"
#include "WiiMgr.h"
#include "Resources.h"
#include <sstream>
#include "OscListener.h"
#include "OscSender.h"
#include "cinder/ObjLoader.h"
#include "cinder/ImageIo.h"
#include "defs.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"

#define OSC_SEND_HOST "localhost"
#define OSC_SEND_PORT 9000
#define OSC_RECEIVE_PORT 3000

#define END_DEATH -1
#define END_TEAR -2
#define TEARTIME 5.0f
#define STARTUPTIME 4.0f

#define COLOR_P0 Color(.02f, .8f, 1.0f);
#define COLOR_P1 Color(1.0f, .1f, .02f);
#define COLOR_P2 Color(.1f, 1.0f, .02f);
#define COLOR_P3 Color(1.0f, .0f, 1.0f);

using namespace ci;
using namespace ci::app;
using namespace std;

enum gameState {
	RUNNING,
	TEARING,
	PAUSED,
	ENDGAME
};

Vec2f cart(float r, float theta)
{
	return Vec2f(r * math<float>::cos(theta), r * math<float>::sin(theta));
}

class tearApp : public AppBasic {
	
private:
	PolyLine<Vec2f> *blob;
	vector<Vec2f> dirs;
	vector<Vec2f> origdirs;
	Vec2f *centroid;
	float tug[NUMPLAYERS];
	float spin[NUMPLAYERS];
	float last;
	CameraOrtho camO;
	CameraPersp cam;
	static float TUGSC, TENSION, RIGIDITY, TEARTHRESHOLD;
	EnemyGenerator* egen;
	ParticleGen* partgen;
	bool debug;
	boost::shared_ptr<vector<Enemy*> > colliding;
	boost::shared_ptr<vector<cornerCollision> > collidingCorners;
	bool colliding_good, colliding_bad;
	bool FLAG_STARTUP;
	
	float score, life;
	float iscore[NUMPLAYERS];
	float sumdist;
	
	gameState state;
	float collectTimer[4];
	
	bool b_endgame, b_firstrun;
	int lastwinner;
	float tearTimer, noPauseTimer, startupTimer;
	
	Font helv48, helv24;
	
	audio::SourceRef au_uglyup, au_hurt, au_bg, au_feed;
	audio::TrackRef ref;
	
	gl::Texture scoreTexture, endgameTexture, pauseTexture, startupTexture;
	
	//bool tugged[4];
	
	osc::Listener listener;
	osc::Sender sender;
	float osc_x[NUMPLAYERS], osc_y[NUMPLAYERS];
	Vec2f osc_irvec[NUMPLAYERS];
	bool osc_btn_a[NUMPLAYERS];
	bool osc_btn_b[NUMPLAYERS];
	bool osc_btn_1[NUMPLAYERS];
	int player_caused_pause;
	bool pause_btn_released;
	
	Color playerColor[4];
	
	int mode;
	
	Rand *rand;
	
	// tune
	float zoom;
	float lifeDecAlwaysRate, lifeDecRate, lifeIncRate;
	
	
	// blur
	gl::Fbo mFboBlurred, mFboTemporary;
	gl::GlslProg mShaderBlur;
	Area viewport;
	
public:
	
	GameState* gs;
	
	void prepareSettings(Settings* settings);
	void setup();
	void mouseDown( MouseEvent event );	
	void keyDown( KeyEvent event );
	void update();
	void draw();
	bool hasTugged(int id);
	void endgame(int winner);
	void shutdown();
	void oscUpdate();
	void oscSend(string address, float value);
	void handleCollisions(float dt);
	void renderBackground();
	void renderObjects();
	void renderHUD();
	void prepareBlur();
	void doBlur(float offset, float alpha, float zoomdist=900);
	void renderCorners(float scale, bool fill, int which = -1);
};

void tearApp::prepareSettings(Settings* settings)
{
	b_firstrun = true;
	lastwinner = -1;
	
	//settings->setWindowSize(1024, 768);
	//return;
	
	vector<shared_ptr<Display> > displays = Display::getDisplays();
	if(displays.size() > 1)
	{
		settings->setDisplay(displays[1]);
		settings->setFullScreen(true);
	}
	else
	{
		settings->setFullScreen(true);
	}
	
	
}

void tearApp::setup()
{
	state = RUNNING;
	
	sender.setup(OSC_SEND_HOST, OSC_SEND_PORT);
	listener.setup(OSC_RECEIVE_PORT);
	
	rand = new Rand();
	
	FLAG_STARTUP = true;
	
	startupTimer = .0f;
	b_endgame = false;
	debug = false;
	score = .0f;
	life = 100.0f;
	mode = 2;
	tearTimer = .0f;
	noPauseTimer = .0f;
	
	
	zoom = 350.0f;
	lifeDecAlwaysRate = 1.0f;
	lifeDecRate = 1.0f;
	lifeIncRate = 3.0f;
	
	colliding_good = colliding_bad = false;
	
	playerColor[0] = COLOR_P0;
	playerColor[1] = COLOR_P1;
	playerColor[2] = COLOR_P2;
	playerColor[3] = COLOR_P3;
	
	helv24 = Font("Helvetica", 24);
	helv48 = Font("Helvetica", 48);
	
	blob = new PolyLine<Vec2f>();
	blob->setClosed(true);
	blob->push_back(cart(50.0f, M_PI/2.0f));
	blob->push_back(cart(50.0f, M_PI/2.0f + 2.0f*M_PI/3.0f));
	blob->push_back(cart(50.0f, M_PI/2.0f + 4.0f*M_PI/3.0f));
	if(NUMPLAYERS > 3)
		blob->push_back(Vec2f(20.0f, 100.0f));
	
	origdirs = vector<Vec2f>();
	origdirs.push_back(Vec2f(-1.0f, -1.0f));
	origdirs.push_back(Vec2f(1.0f, -1.0f));
	origdirs.push_back(Vec2f(1.0f, 1.0f));
	if(NUMPLAYERS > 3)
		origdirs.push_back(Vec2f(-1.0f, 1.0f));
	
	dirs = vector<Vec2f>();
	dirs.assign(origdirs.begin(), origdirs.end());
	
	
	for(int i = 0; i < NUMPLAYERS; i++)
	{
		tug[i] = .0f;
		//tugged[i] = false;
		iscore[i] = .0f;
		spin[i] = .0f;
		osc_x[i] = .0f;
		osc_y[i] = .0f;
		osc_btn_a[i] = false;
		osc_btn_b[i] = false;
		osc_btn_1[i] = false;
		osc_irvec[i] = Vec2f(0, 0);
		collectTimer[i] = .0f;
	}
	player_caused_pause = 0;
	pause_btn_released = false;
	
	last = getElapsedSeconds();
	
	camO = CameraOrtho(0, getWindowWidth(), getWindowHeight(), 0, 0, 1000);
	camO.lookAt(Vec3f(getWindowWidth()/2, getWindowHeight()/2, .0f));
	cam = CameraPersp( getWindowWidth(), getWindowHeight(), 50, 0.1, 10000 );
	cam.lookAt(Vec3f(getWindowWidth()/2, getWindowHeight()/2, .0f));
	
	//cam.setWorldUp(Vec3f(.0f, -1.0f, .0f));
	
	setFrameRate(60.0f);
	
	gs = new GameState();
	
	centroid = new Vec2f(.0f, .0f);
	
	gs->blob = blob;
	gs->centroid = centroid;
	
	egen = new EnemyGenerator(gs, .5f, 15.0f);
	partgen = new ParticleGen(centroid, .25f, 10.0f);
	
	TextLayout simple;
	simple.setFont( Font( "Helvetica", 24 ) );
	simple.setColor( Color( 1.0f, 1.0f, 1.0f ) );
	simple.addLine( "score: 0" );
	scoreTexture = gl::Texture( simple.render( true, false ) );
	
	TextLayout startup;
	startup.setFont( Font( "Helvetica Bold", 72 ) );
	startup.setColor( Color( 1.0f, 1.0f, 1.0f ) );
	startup.addCenteredLine( "tear" );
	startup.setFont( Font( "Helvetica", 24 ) );
	startup.addCenteredLine( "[press 1 for help/pause]" );
	startupTexture = gl::Texture( startup.render( true, false ) );
	
	pauseTexture = gl::Texture(Surface(loadImage(loadResource("help.png"))));
	
	if(!Enemy::mVBOBad)
	{
		ObjLoader loader( loadResource( RES_SPIKEBALL_OBJ )->createStream() );
		Enemy::mMeshBad = new TriMesh();
		loader.load( Enemy::mMeshBad );
		Enemy::mVBOBad = new gl::VboMesh( *Enemy::mMeshBad );
	}
	
	
	oscSend("/cinder/osc/start", 1.0f);
	
	/////
	
	gl::Fbo::Format format;
	format.enableMipmapping(false);
	format.setCoverageSamples(16);
	format.setSamples(4);
	mFboBlurred = gl::Fbo(512, 512, format);
	mFboTemporary = gl::Fbo(512, 512, format);
	
	try { 
		mShaderBlur = gl::GlslProg(loadResource(RES_VERT_GLSL),
								   loadResource(RES_FRAG_GLSL)); 
	} catch(...) {
		console() << "Can't load/compile blur shader" << endl;
	}
}

void tearApp::mouseDown( MouseEvent event )
{
}

void tearApp::keyDown( KeyEvent event )
{
	if(state == RUNNING && event.getChar() == ' ')
	{
		b_endgame = false;
		state = RUNNING;
		this->setup();
	}
	
	if(event.getChar() == 'g')
	{
		b_firstrun = false;
		b_endgame = false;
		state = RUNNING;
		this->setup();
	}
	if( event.getChar() == 'd' ){
		debug = !debug;
	}
	if( event.getChar() == '1' ){
		mode = 0;
	}
	if( event.getChar() == '2' ){
		mode = 1;
	}
	if( event.getChar() == '3' ){
		mode = 2;
	}
	
	if( event.getChar() == '4' ){
		mode = 3;
	}
}

void tearApp::oscSend(string address, float value)
{
	osc::Message message;
	message.addFloatArg(value);
	message.setAddress(address);
	message.setRemoteEndpoint(OSC_SEND_HOST, OSC_SEND_PORT);
	sender.sendMessage(message);
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
				
				
				int num = (int) addr[5] - (int) '0' - 1;
				
				if(num > NUMPLAYERS) continue;
				
				osc_x[num] = message.getArgAsFloat(0);
				osc_y[num] = message.getArgAsFloat(1);
				
				if(state == RUNNING)
					osc_irvec[num] = *centroid + Vec2f( (osc_x[num] - .5f) * getWindowWidth() * (zoom/330.0f)/2.0f, (osc_y[num] - .5f) * getWindowHeight() * (zoom/330.0f)/2.0f);

				console() << num << ": " << osc_x[num] << " / " << osc_y[num] << endl;
				
			} catch (...) {
				console() << "Exception reading argument as float" << std::endl;
			}
		}
		else if((addr == "/wii/1/button/B" ||
				   addr == "/wii/2/button/B" ||
				   addr == "/wii/3/button/B" ||
				   addr == "/wii/4/button/B" ) && message.getNumArgs() == 1)
		{
			
			try {
				
				
				
				int num = (int) addr[5] - (int) '0' - 1;
				
				if(num > NUMPLAYERS) continue;
				
				if(message.getArgAsInt32(0)) osc_btn_b[num] = true;
				else						 osc_btn_b[num] = false;
				
				console() << num << ": B " << (osc_btn_a[num] ? "on" : "off") << endl;
				
			} catch (...) {
				console() << "Exception reading argument as int" << std::endl;
			}
		}
		
		else if((addr == "/wii/1/button/A" ||
				 addr == "/wii/2/button/A" ||
				 addr == "/wii/3/button/A" ||
				 addr == "/wii/4/button/A" ) && message.getNumArgs() == 1)
		{
			
			try {
				
				
				
				int num = (int) addr[5] - (int) '0' - 1;
				
				if(num > NUMPLAYERS) continue;
				
				if(message.getArgAsInt32(0)) osc_btn_a[num] = true;
				else						 osc_btn_a[num] = false;
				
				console() << num << ": A " << (osc_btn_a[num] ? "on" : "off") << endl;
				
			} catch (...) {
				console() << "Exception reading argument as int" << std::endl;
			}
		}
		
		else if((addr == "/wii/1/button/1" ||
				 addr == "/wii/2/button/1" ||
				 addr == "/wii/3/button/1" ||
				 addr == "/wii/4/button/1" ) && message.getNumArgs() == 1)
		{
			
			try {
				
				
				
				int num = (int) addr[5] - (int) '0' - 1;
				
				if(num > NUMPLAYERS) continue;
				
				if(message.getArgAsInt32(0)) 
				{
					osc_btn_1[num] = true;
					player_caused_pause = num;
					
				}
				else						 
					osc_btn_1[num] = false;
				
				console() << num << ": 1 " << (osc_btn_1[num] ? "on" : "off") << endl;
				
			} catch (...) {
				console() << "Exception reading argument as int" << std::endl;
			}
		}
		
		else if((addr == "/max/zoom" ) && message.getNumArgs() == 1)
		{
			try {
				zoom = message.getArgAsFloat(0);
				
			} catch (...) {
				console() << "Exception reading argument as float" << std::endl;
			}
		}
		else if((addr == "/max/tugsc" ) && message.getNumArgs() == 1)
		{
			try {
				TUGSC = message.getArgAsFloat(0);
				
			} catch (...) {
				console() << "Exception reading argument as float" << std::endl;
			}
		}
		else if((addr == "/max/tension" ) && message.getNumArgs() == 1)
		{
			try {
				TENSION = message.getArgAsFloat(0);
				
			} catch (...) {
				console() << "Exception reading argument as float" << std::endl;
			}
		}
		else if((addr == "/max/rigidity" ) && message.getNumArgs() == 1)
		{
			try {
				RIGIDITY = message.getArgAsFloat(0);
				
			} catch (...) {
				console() << "Exception reading argument as float" << std::endl;
			}
		}
		else if((addr == "/max/lifeDecRate" ) && message.getNumArgs() == 1)
		{
			try {
				lifeDecRate = message.getArgAsFloat(0);
				
			} catch (...) {
				console() << "Exception reading argument as float" << std::endl;
			}
		}
		else if((addr == "/max/lifeDecAlwaysRate" ) && message.getNumArgs() == 1)
		{
			try {
				lifeDecAlwaysRate = message.getArgAsFloat(0);
				
			} catch (...) {
				console() << "Exception reading argument as float" << std::endl;
			}
		}
		else if((addr == "/max/lifeIncRate" ) && message.getNumArgs() == 1)
		{
			try {
				lifeIncRate = message.getArgAsFloat(0);
				
			} catch (...) {
				console() << "Exception reading argument as float" << std::endl;
			}
		}
		else if((addr == "/max/tearthreshold" ) && message.getNumArgs() == 1)
		{
			try {
				TEARTHRESHOLD = message.getArgAsFloat(0);
				
			} catch (...) {
				console() << "Exception reading argument as float" << std::endl;
			}
		}
		
	}
}

void tearApp::update()
{
	oscUpdate();
	
	float now = getElapsedSeconds();
	float dt = now - last;
	last = now;
	
	bool found;
	
	switch(state)
	{
		// ---------------------------------------------------------------------------
		case RUNNING:
		// ---------------------------------------------------------------------------
			if(noPauseTimer > 0) noPauseTimer -= dt;
			if(noPauseTimer < 0) noPauseTimer = 0;
			
			if(startupTimer < STARTUPTIME)	startupTimer += dt;
			else if(FLAG_STARTUP)			FLAG_STARTUP = false;
			
			// handle life & score updates
			oscSend("/cinder/osc/life", life);
			
			if(life <= .0f)
				endgame(END_DEATH);
			
			for(int i = 0; i < NUMPLAYERS; i++)
			{
				if(iscore[i] >= 100.0f)
					endgame(i);
				
				collectTimer[i] -= dt;
				if(collectTimer[i] < 0) collectTimer[i] = .0f;
			}
				
			
			life -= dt * lifeDecAlwaysRate;
			
			
			// move corners according to tug (indirectly and directly)
			// ---------------------------------------------------------------------------

			for(int i = 0; i < NUMPLAYERS; i++)
			{
				dirs[i] = (osc_irvec[i] - blob->getPoints()[i]).normalized();
				if(tug[i] > .0f) tug[i] *= .93f;
				if(blob->getPoints()[i].distance(osc_irvec[i]) > 0.0f)
					blob->getPoints()[i] += dirs[i] * tug[i] * math<float>::min(blob->getPoints()[i].distance(osc_irvec[i]), 50.0f) / 50.0f;
				
				// apply weaker force to other corners
				for(int j = 0; j < NUMPLAYERS; j++)
				{
					if(j == i) continue;
					blob->getPoints()[i] += dirs[j] * tug[j] * RIGIDITY;	
				}
			}
			// ---------------------------------------------------------------------------

			
			// calc centroid
			// ---------------------------------------------------------------------------
			centroid->x = .0f; centroid->y = .0f;
			
			for(int i = 0; i < NUMPLAYERS; i++)
			{
				*centroid += blob->getPoints()[i];
			}
			
			*centroid /= (float) NUMPLAYERS;
			// ---------------------------------------------------------------------------
			
			
			// get sum of distances, check for tearing
			// ---------------------------------------------------------------------------

			sumdist = .0f;
			for(int i = 0; i < NUMPLAYERS; i++)
			{
				sumdist += blob->getPoints()[i].distance(*centroid);
			}
			
			if(sumdist > TEARTHRESHOLD)
			{
				oscSend("/cinder/osc/tear", 1.0f);
				state = TEARING;
			}
			// ---------------------------------------------------------------------------

			
			// add elastic forces
			// ---------------------------------------------------------------------------
			for(int i = 0; i < NUMPLAYERS; i++)
			{
				Vec2f& pt = blob->getPoints()[i];
				
				if(pt.distance(*centroid) > 55.0f)
				{
					pt += (*centroid - pt).normalized() * ( TENSION / 2.0f ) * sumdist/300.0f;
				}
			}
			// ---------------------------------------------------------------------------

			
			// apply tug for next round
			// ---------------------------------------------------------------------------
			for(int j = 0; j < NUMPLAYERS; j++)
				if(hasTugged(j)) tug[j] += TUGSC * 0.5f;
			// ---------------------------------------------------------------------------
			
			// collision detection, particle updates
			// ---------------------------------------------------------------------------
			handleCollisions(dt);
			egen->update(dt);
		//	partgen->update(dt);
			// ---------------------------------------------------------------------------

			// check for pause requested
			// ---------------------------------------------------------------------------

			found = false;
			for(int i = 0; i < NUMPLAYERS; i++)
			{
				if(osc_btn_1[i])
				{
					found = true;
				}
			}
			
			if(found && !noPauseTimer) 
			{
				state = PAUSED;
				pause_btn_released = false;
			}
			// ---------------------------------------------------------------------------

			
			break;
			
		// ---------------------------------------------------------------------------
		case TEARING:
		// ---------------------------------------------------------------------------

			for(int i = 0; i < NUMPLAYERS; i++)
			{
				dirs[i] = (blob->getPoints()[i] - *centroid).normalized();
				blob->getPoints()[i] += dirs[i] * tug[i] * 3.0f;
			}
				
			tearTimer += dt;
			if(tearTimer > TEARTIME) endgame(END_TEAR);
			
			// apply tug
			// ---------------------------------------------------------------------------
			
			for(int j = 0; j < NUMPLAYERS; j++)
				tug[j] = 4.0f;
			
			// ---------------------------------------------------------------------------
		
			egen->update(dt);
		//	partgen->update(dt);
			
			break;
		
		// ---------------------------------------------------------------------------
		case ENDGAME:
		// ---------------------------------------------------------------------------

			found = false;
			for(int i = 0; i < NUMPLAYERS; i++)
			{
				if(osc_btn_a[i])
				{
					found = true;
				}
			}
			
			if(found) setup();
			
			break;
		
		// ---------------------------------------------------------------------------
		case PAUSED:
		// ---------------------------------------------------------------------------
			
			FLAG_STARTUP = false;
			
			// check for unpause requested
			// ---------------------------------------------------------------------------

			if(osc_btn_1[player_caused_pause] == 0)
				pause_btn_released = true;
			
			found = false;
			for(int i = 0; i < NUMPLAYERS; i++)
			{
				if(osc_btn_1[i] && pause_btn_released)
				{
					found = true;
				}
			}
			
			if(found) 
			{
				state = RUNNING;
				noPauseTimer = .4f;
				
			}
			// ---------------------------------------------------------------------------

			
			break;
	}
	
}

void tearApp::handleCollisions(float dt)
{
	colliding = egen->collideWithBlob();
	
	colliding_good = colliding_bad = false;
	if(colliding)
	{
		vector<Enemy*>::iterator it;
		for(it = colliding->begin(); it < colliding->end(); it++)
		{
			if((*it)->type == GOOD) 
			{
				score += .001;
				life += lifeIncRate * dt;
				colliding_good = true;
				oscSend("/cinder/osc/health", 1.0f);
				
			}
			else if((*it)->type == BAD) 
			{
				score = math<float>::max(score - .005, .0f);
				life -= lifeDecRate * dt;
				colliding_bad = true;
				oscSend("/cinder/osc/hurt", 1.0f);
			}
		}
	}
	
	
	collidingCorners = egen->collideWithCorners();
	if(collidingCorners)
	{
		vector<cornerCollision>::iterator it;
		for(it = collidingCorners->begin(); it < collidingCorners->end(); it++)
		{
			switch(it->enemy->type)
			{
				case UGLY:
					iscore[it->corner] = math<float>::min(iscore[it->corner] + 10.0f, 100.0f);
					oscSend("/cinder/osc/collect", 1.0f);
					collectTimer[it->corner] = .5f;
					break;
					
				case COLORED:
					if(it->enemy->special_type == it->corner)
					{
						iscore[it->corner] = math<float>::min(iscore[it->corner] + 10.0f, 100.0f);
						oscSend("/cinder/osc/collect", 1.0f);
						collectTimer[it->corner] = .5f;
					}
					else
					{
						iscore[it->enemy->special_type] = math<float>::max(iscore[it->enemy->special_type] - 10.0f, 0.0f);
						oscSend("/cinder/osc/anticollect", 1.0f);
					}
					break;
			}
			
			it->enemy->expired = it->enemy->lifetime;
		}
	}
	
}

bool tearApp::hasTugged(int id)
{
	return osc_btn_b[id];
}

void tearApp::renderBackground()
{
	
	
	// draw the background grid, total -10000 to 10000, but clip everything outside the frame
	// ---------------------------------------------------------------------------
	
	for(int i = 0; i < 100; i++)
		for(int j = 0; j < 100; j++)
		{
			if(centroid->distance(Vec2f(i*100-5000, j*100-5000)) > getWindowWidth() * 2) continue;
			
			glPushMatrix();
			gl::translate(Vec3f(i * 100 - 5000, j * 100 - 5000, -300.0f));
			gl::color(ColorA(1.0f, .5f, 1.0f, .4f));
			glLineWidth(1.0f);
			glLineStipple(3, 0xAAAA);
			glEnable(GL_LINE_STIPPLE);
			gl::drawLine(Vec2f(-52.0f, 0.0f), Vec2f(52.0f, 0.0f));
			gl::drawLine(Vec2f(.0f, -52.0f), Vec2f(0.0f, 52.0f));
			glDisable(GL_LINE_STIPPLE);
			glPopMatrix();
		}
	
	for(int j = 0; j < 50; j++)
	{
		if(centroid->distance(Vec2f(j*200-5000, .0f)) > getWindowWidth() * 5) continue;
		
		glPushMatrix();
		gl::translate(Vec3f(j * 200 - 5000, .0f, -2000.0f));
		gl::color(ColorA(.7f, .2f, .7f, .2f));
		gl::drawLine(Vec2f(.0f, -4500.0f), Vec2f(0.0f, 4500.0f));
		glPopMatrix();
	}
	
	
	//glPushMatrix();
//	gl::translate(Vec3f(.0f, .0f, -50.0f));
//	partgen->draw();
//	glPopMatrix();
//	
	// ---------------------------------------------------------------------------
}

void tearApp::renderCorners(float scale, bool fill, int which)
{
	for(int i = 0; i < NUMPLAYERS; i++)
	{
		if(which != -1 && i != which) continue;
		
		Vec2f& pt = blob->getPoints()[i];
		glPushMatrix();
		
		gl::translate(pt);
		
		//gl::color(Color(.0f, .0f, .0f));
		//gl::drawStrokedCircle(Vec2f(.0f, .0f), 5.0f, 32);
		
		gl::color(playerColor[i]);
		
		if(fill)
			gl::drawSolidCircle(Vec2f(.0f, .0f), scale * (5.0f + iscore[i] / 10.0f), 32);
		else
			gl::drawStrokedCircle(Vec2f(.0f, .0f), scale * (5.0f + iscore[i] / 10.0f), 32);
		
		if(which == -1)
			gl::drawStrokedCircle(Vec2f(.0f, .0f), 15.0f, 32);
		
		
		
		glPopMatrix();
	}
}

void tearApp::renderObjects()
{
	// draw blob lines
	// ---------------------------------------------------------------------------
	if(state != TEARING && !(state == ENDGAME && lastwinner == END_TEAR)){
		glLineWidth(3.0f);
		if(sumdist > 250.0f)
		{
			int lvl = (int) (sumdist/100.0f);
			glLineStipple((int) (sumdist/100.0f), lvl < 4 ? 0xAAAA : 0x8888);
			glEnable(GL_LINE_STIPPLE);
		}
		gl::color(Color(1.0f, 1.0f, 1.0f));
		gl::draw(*blob);
		glDisable(GL_LINE_STIPPLE);
		glLineWidth(1.0f);
		// ---------------------------------------------------------------------------
		
		
		// draw inner area
		// ---------------------------------------------------------------------------
		
		gl::color(ColorA(1.0f, 1.0f, 1.0f, .2f));
		
		glBegin(GL_TRIANGLE_FAN);
		
		gl::vertex(*centroid);
		
		for(int i = 0; i < NUMPLAYERS; i++)
		{
			Vec2f& pt = blob->getPoints()[i];
			
			gl::vertex(pt);
		}
		
		gl::vertex(blob->getPoints()[0]);
		
		glEnd();
		
	}
	// ---------------------------------------------------------------------------
	
	
	// draw corners
	// ---------------------------------------------------------------------------
	
	renderCorners(1.0f, true);
	
	prepareBlur();
	
	renderCorners(10.0f, true);
	
	doBlur(10.0f, .6f + sin(getElapsedSeconds() * 3.0f) / 10.0f);
	
	prepareBlur();
	
	renderCorners(1.0f, true);
	
	doBlur(1.0f, .7f, 1100);
	
	for(int i = 0; i < NUMPLAYERS; i++)
	{
		if(collectTimer[i] > .0f)
		{
			prepareBlur();
			
			renderCorners((.5f - collectTimer[i]) * 30.0f + 5.0f, false, i);
			
			doBlur(1.5f, collectTimer[i] * 1.9f, 1100);
		}
	}
	
	
	
	
	// ---------------------------------------------------------------------------
	
	// draw enemies
	egen->draw();
	
	// draw collision indicators
	//---------------------------------------------------------------------------
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
	// ---------------------------------------------------------------------------
	
	// draw cursors
	// ---------------------------------------------------------------------------
	
	if(state == RUNNING)
	{
		for(int i = 0; i < NUMPLAYERS; i++)
		{
			gl::color(playerColor[i]);
			glLineWidth(1.5f);
			gl::drawStrokedCircle(osc_irvec[i], 10.0f, 16);

			gl::color(ColorA(playerColor[i], .5f));
			
			Vec2f& pt = blob->getPoints()[i];
			
			glPushMatrix();
			gl::translate(pt);
			Vec3f v = Vec3f(osc_irvec[i].x, osc_irvec[i].y, .0f) - Vec3f(pt.x, pt.y, .0f);
			gl::drawVector(Vec3f(.0f, .0f, .0f), v - v.normalized()*10.0f);
			glPopMatrix();
		}
	}
	// ---------------------------------------------------------------------------
}

void tearApp::renderHUD()
{
	// draw health bar
	// ---------------------------------------------------------------------------
	
	glColor3f( 1.0f, 1.0f, 1.0f );
	
	
	glPushMatrix();
	
	gl::translate(Vec3f(camO.getEyePoint().x + getWindowWidth()/2, camO.getEyePoint().y+50.0f, .0f));
	
	gl::color(ColorA(1.0f, life/100.0f, life/100.0f, .9f));
	
	gl::drawSolidRect(Rectf(-life*3.0f, .0f, life*3.0f, 10.0f));
	
	if(colliding_bad)
	{
		gl::color(ColorA(1.0f, .0f, .0f, math<float>::sin(getElapsedSeconds() * 10.0f)));
		gl::scale(Vec3f(1.0f, 1.2f, 1.0f));
		gl::drawSolidRect(Rectf(-life*3.0f, .0f, life*3.0f, 10.0f));
	}
	
	if(colliding_good)
	{
		gl::color(ColorA(.6f, .6f, 1.0f, math<float>::sin(getElapsedSeconds() * 10.0f)));
		gl::scale(Vec3f(1.0f, 1.2f, 1.0f));
		gl::drawSolidRect(Rectf(-life*3.0f, .0f, life*3.0f, 10.0f));
	}
	
	glPopMatrix();
	// ---------------------------------------------------------------------------
	
	
	// state/flag overlays
	// ---------------------------------------------------------------------------
	
	switch(state)
	{
		case ENDGAME:
			gl::color(ColorA(.0f, .0f, .0f, .5f));
			gl::drawSolidRect(Rectf(camO.getEyePoint().x, camO.getEyePoint().y, camO.getEyePoint().x + getWindowWidth(), camO.getEyePoint().y + getWindowHeight()));
			gl::color(Color(1.0f, 1.0f, 1.0f));
			gl::draw( endgameTexture, Vec2f( camO.getEyePoint().x + getWindowWidth()/2 - endgameTexture.getWidth()/2, camO.getEyePoint().y + getWindowHeight()/2 ) );
			
			
			break;
			
		case PAUSED:
			
			gl::color(ColorA(.0f, .0f, .0f, .5f));
			gl::drawSolidRect(Rectf(camO.getEyePoint().x, camO.getEyePoint().y, camO.getEyePoint().x + getWindowWidth(), camO.getEyePoint().y + getWindowHeight()));
			gl::color(ColorA(1.0f, 1.0f, 1.0f, .8f));
			gl::draw( pauseTexture, Vec2f( camO.getEyePoint().x + getWindowWidth()/2 - pauseTexture.getWidth()/2, camO.getEyePoint().y + getWindowHeight()/2 - pauseTexture.getHeight()/2 ) );
			
			break;
	}
	
	if(FLAG_STARTUP && state == RUNNING)
	{
		gl::color(ColorA(1.0f, 1.0f, 1.0f, startupTimer > 3.0f ? (STARTUPTIME - startupTimer) / (STARTUPTIME - 3.0f) : 1.0f));
		gl::draw( startupTexture, Vec2f( camO.getEyePoint().x + getWindowWidth()/2 - startupTexture.getWidth()/2, camO.getEyePoint().y + getWindowHeight()/2 - startupTexture.getHeight()/2 - 200 ) );
		
	}
	
	// ---------------------------------------------------------------------------
}

void tearApp::prepareBlur()
{
	viewport = gl::getViewport();
	gl::setViewport( mFboBlurred.getBounds() );
	mFboBlurred.bindFramebuffer();
	gl::clear( Color::black() );
	
	cam.setAspectRatio(1.0f);
}

void tearApp::doBlur(float offset, float alpha, float zoomdist)
{
	mFboBlurred.unbindFramebuffer();
	
	mShaderBlur.bind();
	mShaderBlur.uniform("tex0", 0); // use mFboBlurred, see lower
	mShaderBlur.uniform("sampleOffset", Vec2f(offset / 512.0f, 0.0f));
	
	mFboTemporary.bindFramebuffer();
	gl::clear( Color::black() );
	mFboBlurred.bindTexture(0); // use rendered scene as texture
	gl::pushMatrices();
	gl::setMatricesWindow(512, 512, false);
	gl::drawSolidRect( mFboBlurred.getBounds() );
	gl::popMatrices();
	mFboBlurred.unbindTexture();
	mFboTemporary.unbindFramebuffer();
	
	mShaderBlur.uniform("sampleOffset", Vec2f(0.0f, offset / 512.0f));
	
	mFboBlurred.bindFramebuffer();
	gl::clear( Color::black() );
	mFboTemporary.bindTexture(0); // use horizontally blurred scene as texture
	gl::pushMatrices();
	gl::setMatricesWindow(512, 512, false);
	gl::drawSolidRect( mFboTemporary.getBounds() );
	gl::popMatrices();
	mFboTemporary.unbindTexture();
	mFboBlurred.unbindFramebuffer();
	
	mShaderBlur.unbind();
	
	gl::setViewport( viewport );
	cam.setAspectRatio(getWindowAspectRatio());
	
	gl::enableAdditiveBlending();
	gl::color( Color::white() );
	glPushMatrix();
	gl::translate(Vec3f(0, 0, zoom-zoomdist));
	gl::color(ColorA(1.0f, 1.0f, 1.0f, alpha));
	gl::draw( mFboBlurred.getTexture(), Rectf(centroid->x - getWindowWidth()/2, centroid->y-getWindowHeight()/2, centroid->x+getWindowWidth()/2, centroid->y + getWindowHeight()/2) );
	glPopMatrix();
	gl::enableAlphaBlending( false );
}

void tearApp::draw()
{
	
	gl::enableAlphaBlending( false );
	
	
	
	
	// -------------------------------------------------------------------------
	// PERSPECTIVE SECTION
	// -------------------------------------------------------------------------
	
	glPushMatrix();
	
	// set camera to center on centroid
	cam.lookAt(Vec3f(centroid->x, centroid->y, zoom), Vec3f(centroid->x, centroid->y, .0f));
	gl::setMatrices(cam);
	
	gl::clear( Color( .1f, .05f, .1f ) ); 

	
	renderBackground();
	renderObjects();
	
	

	glPopMatrix();
	
	// END PERSPECTIVE SECTION ---------------------------------------------------------------------------

	
	
	// -------------------------------------------------------------------------
	// HUD / ORTHO SECTION
	// -------------------------------------------------------------------------
	
	glPushMatrix();
	
	camO.lookAt(Vec3f(centroid->x - getWindowWidth()/2, centroid->y-getWindowHeight()/2, 10.0f), Vec3f(centroid->x- getWindowWidth()/2, centroid->y-getWindowHeight()/2, .0f));
	gl::setMatrices(camO);
	
	renderHUD();

	glPopMatrix();
	
	// END HUD/ORTHO SECTION -------------------------------------------------------------------------

	
}

void tearApp::endgame(int winner)
{
	oscSend("/cinder/osc/endgame", 1.0f);
	
	state = ENDGAME;
	b_endgame = true;
	b_firstrun = false;
	lastwinner = winner;
	
	TextLayout simple;
	simple.setFont( helv48 );
	simple.clear(ColorA(.0f, .0f, .0f, .3f));
	simple.setColor( Color( 1.0f, 1.0f, 1.0f ) );
	simple.setLeadingOffset(10.0f);

	if(winner >= 0)
	{
		stringstream ss;
		ss << "player ";
		ss << (winner+1);
		ss << " won";
		simple.setColor(playerColor[winner]);
		simple.addCenteredLine(ss.str());
		simple.setFont(helv24);
	} 
	else if(winner == END_DEATH)
	{
		simple.addCenteredLine("everybody lost");
		simple.setFont(helv24);
		simple.addCenteredLine("you died");
	} 
	else if(winner == END_TEAR)
	{
		simple.addCenteredLine("everybody lost");
		simple.setFont(helv24);
		simple.addCenteredLine("you were torn apart");
	}

	simple.setColor( Color( 1.0f, 1.0f, 1.0f ) );
	simple.addCenteredLine( "[press A to play again]" );
	
	endgameTexture = gl::Texture( simple.render( true, false ) );
	
}

void tearApp::shutdown()
{
	oscSend("/cinder/osc/start", .0f);
	delete rand;
	delete egen;
	delete partgen;
	delete gs;
	delete centroid;
	delete blob;
}


float tearApp::TUGSC = 1.8f;
float tearApp::TENSION = 3.8f;
float tearApp::RIGIDITY = .4f;
float tearApp::TEARTHRESHOLD = 600.0f;

CINDER_APP_BASIC( tearApp, RendererGl )
