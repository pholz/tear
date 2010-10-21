/*
 *  WiiMgr.h
 *  tear
 *
 *  Created by Peter Holzkorn on 19.10.10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "wiicpp.h"
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

using namespace std;

class WiiMgr
{
	CWii wii;
	vector<CWiimote>::iterator i;
	int reloadWiimotes;
	int numFound;
	int index;
	vector<CWiimote>* wiimotes;
	volatile bool m_stoprequested;
	
public:
	WiiMgr();
	void update();
	void go();
	void stop();
	void foo();
	void HandleEvent(CWiimote &wm);
	void HandleReadData(CWiimote &wm);
	void HandleDisconnect(CWiimote &wm);
	void HandleStatus(CWiimote &wm);
	
	float a_state_roll[4], a_state_pitch[4];
	boost::shared_ptr<boost::thread> thrd;
};