/*
 *  WiiMgr.cpp
 *  tear
 *
 *  Created by Peter Holzkorn on 19.10.10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "WiiMgr.h"

int LEDMAP[4] = {CWiimote::LED_1, CWiimote::LED_2, CWiimote::LED_3, CWiimote::LED_4};

WiiMgr::WiiMgr()
{
	m_stoprequested = false;
	
	reloadWiimotes = 0;
	
	cout << "Searching for wiimotes->.. Turn them on!" << endl;
	
    //Find the wiimote
    numFound = wii.Find(1);
	
    // Search for up to five seconds;
	
    cout << "Found " << numFound << " wiimotes" << endl;
    cout << "Connecting to wiimotes->.." << endl;
	
    // Connect to the wiimote
    wiimotes = &(wii.Connect());
	
	cout << "Connected to " << (unsigned int)wiimotes->size() << " wiimotes" << endl;
	
	for(index = 0, i = wiimotes->begin(); i != wiimotes->end(); ++i, ++index)
    {
        // Use a reference to make working with the iterator handy.
        CWiimote & wiimote = *i;
		
        //Set Leds
        wiimote.SetLEDs(LEDMAP[index]);
		
        //Rumble for 0.2 seconds as a connection ack
        wiimote.SetRumbleMode(CWiimote::ON);
        usleep(200000);
        wiimote.SetRumbleMode(CWiimote::OFF);
		wiimote.SetMotionSensingMode(CWiimote::ON);
    }
	
	for(int i = 0; i < 4; i++)
	{
		a_state_roll[i] = .0f;
		a_state_pitch[i] = .0f;
	}
	
}

void WiiMgr::go()
{
	assert(!thrd);
	thrd = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&WiiMgr::foo, this)));
	//thrd->join();
}

void WiiMgr::foo()
{
	while(true){
		
		usleep(10000);
		update();
	}
}

void WiiMgr::stop() // Note 1
{
	assert(thrd);
	m_stoprequested = true;
	thrd->join();
}

void WiiMgr::update()
{

	//do{
		
	
	
	if(reloadWiimotes)
	{
		// Regenerate the list of wiimotes
		wiimotes = &(wii.GetWiimotes());
		reloadWiimotes = 0;
	}
	
	//Poll the wiimotes to get the status like pitch or roll
	if(wii.Poll())
	{
		for(i = wiimotes->begin(); i != wiimotes->end(); ++i)
		{
			// Use a reference to make working with the iterator handy.
			CWiimote & wiimote = *i;
			switch(wiimote.GetEvent())
			{
					
				case CWiimote::EVENT_EVENT:
					HandleEvent(wiimote);
					break;
					
				case CWiimote::EVENT_STATUS:
					HandleStatus(wiimote);
					break;
					
				case CWiimote::EVENT_DISCONNECT:
				case CWiimote::EVENT_UNEXPECTED_DISCONNECT:
					HandleDisconnect(wiimote);
					reloadWiimotes = 1;
					break;
					
				case CWiimote::EVENT_READ_DATA:
					HandleReadData(wiimote);
					break;
					
				default:
					break;
			}
		}
	}
		
//	} while(wiimotes->size());

	
}

void WiiMgr::HandleEvent(CWiimote &wm)
{
    char prefixString[64];
	
    if(wm.Buttons.isJustPressed(CButtons::BUTTON_MINUS))
    {
        wm.SetMotionSensingMode(CWiimote::OFF);
    }
	
    if(wm.Buttons.isJustPressed(CButtons::BUTTON_PLUS))
    {
        wm.SetMotionSensingMode(CWiimote::ON);
    }
	
    if(wm.Buttons.isJustPressed(CButtons::BUTTON_DOWN))
    {
        wm.IR.SetMode(CIR::OFF);
    }
	
    if(wm.Buttons.isJustPressed(CButtons::BUTTON_UP))
    {
        wm.IR.SetMode(CIR::ON);
    }
	
    if(wm.Buttons.isJustPressed(CButtons::BUTTON_RIGHT))
    {
        wm.EnableMotionPlus(CWiimote::ON);
    }
	
    if(wm.Buttons.isJustPressed(CButtons::BUTTON_LEFT))
    {
		wm.EnableMotionPlus(CWiimote::OFF);
    }
	
    if(wm.Buttons.isJustPressed(CButtons::BUTTON_B))
    {
        wm.ToggleRumble();
    }
	
    sprintf(prefixString, "Controller [%i]: ", wm.GetID());
	
    if(wm.Buttons.isPressed(CButtons::BUTTON_A))
    {
        printf("%s A pressed\n", prefixString);
    }
	
    if(wm.Buttons.isPressed(CButtons::BUTTON_B))
    {
        printf("%s B pressed\n", prefixString);
    }
	
    if(wm.Buttons.isPressed(CButtons::BUTTON_UP))
    {
        printf("%s Up pressed\n", prefixString);
    }
	
    if(wm.Buttons.isPressed(CButtons::BUTTON_DOWN))
    {
        printf("%s Down pressed\n", prefixString);
    }
	
    if(wm.Buttons.isPressed(CButtons::BUTTON_LEFT))
    {
        printf("%s Left pressed\n", prefixString);
    }
	
    if(wm.Buttons.isPressed(CButtons::BUTTON_RIGHT))
    {
        printf("%s Right pressed\n", prefixString);
    }
	
    if(wm.Buttons.isPressed(CButtons::BUTTON_MINUS))
    {
        printf("%s Minus pressed\n", prefixString);
    }
	
    if(wm.Buttons.isPressed(CButtons::BUTTON_PLUS))
    {
        printf("%s Plus pressed\n", prefixString);
    }
	
    if(wm.Buttons.isPressed(CButtons::BUTTON_ONE))
    {
        printf("%s One pressed\n", prefixString);
    }
	
    if(wm.Buttons.isPressed(CButtons::BUTTON_TWO))
    {
        printf("%s Two pressed\n", prefixString);
    }
	
    if(wm.Buttons.isPressed(CButtons::BUTTON_HOME))
    {
        printf("%s Home pressed\n", prefixString);
    }
	
    // if the accelerometer is turned on then print angles
    if(wm.isUsingACC())
    {
        float pitch, roll, yaw, a_pitch, a_roll;
        wm.Accelerometer.GetOrientation(pitch, roll, yaw);
        wm.Accelerometer.GetRawOrientation(a_pitch, a_roll);
        printf("%s wiimote roll = %f [%f]\n", prefixString, roll, a_roll);
        printf("%s wiimote pitch = %f [%f]\n", prefixString, pitch, a_pitch);
        printf("%s wiimote yaw = %f\n", prefixString, yaw);
		
		a_state_roll[wm.GetID()-1] = roll;
		a_state_pitch[wm.GetID()-1] = pitch;
    }
	
    // if(IR tracking is on then print the coordinates
    if(wm.isUsingIR())
    {
        std::vector<CIRDot>::iterator i;
        int x, y;
        int index;
		
        printf("%s Num IR Dots: %i\n", prefixString, wm.IR.GetNumDots());
        printf("%s IR State: %u\n", prefixString, wm.IR.GetState());
		
        std::vector<CIRDot>& dots = wm.IR.GetDots();
		
        for(index = 0, i = dots.begin(); i != dots.end(); ++index, ++i)
        {
            if((*i).isVisible())
            {
                (*i).GetCoordinate(x, y);
                printf("%s IR source %i: (%i, %i)\n", prefixString, index, x, y);
				
                wm.IR.GetCursorPosition(x, y);
                printf("%s IR cursor: (%i, %i)\n", prefixString, x, y);
                printf("%s IR z distance: %f\n", prefixString, wm.IR.GetDistance());
            }
        }
    }
	
    
}

void WiiMgr::HandleStatus(CWiimote &wm)
{
    printf("\n");
    printf("--- CONTROLLER STATUS [wiimote id %i] ---\n\n", wm.GetID());
	
    printf("attachment: %i\n", wm.ExpansionDevice.GetType());
    printf("speaker: %i\n", wm.isUsingSpeaker());
    printf("ir: %i\n", wm.isUsingIR());
    printf("leds: %i %i %i %i\n", wm.isLEDSet(1), wm.isLEDSet(2), wm.isLEDSet(3), wm.isLEDSet(4));
    printf("battery: %f %%\n", wm.GetBatteryLevel());
}

void WiiMgr::HandleDisconnect(CWiimote &wm)
{
    printf("\n");
    printf("--- DISCONNECTED [wiimote id %i] ---\n", wm.GetID());
    printf("\n");
}

void WiiMgr::HandleReadData(CWiimote &wm)
{
    printf("\n");
    printf("--- DATA READ [wiimote id %i] ---\n", wm.GetID());
    printf("\n");
}
