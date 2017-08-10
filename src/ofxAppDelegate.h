//
//  ofxAppDelegate.h
//  BaseApp
//
//  Created by Oriol Ferrer Mesià on 4/8/16.
//
//

#pragma once
#include "ofMain.h"
#include "ofxAppStructs.h"
#include "ofxTuio.h"
#include "ofxScreenSetup.h"

//to use ofxApp, your app must follow this protocol, so make sure your app
//is a subclass of ofxAppDelegate

class ofxAppDelegate{

public:

	// your app will be given a chance to run 4 custom tasks during startup:
	// 1 - To run code just b4 content loading starts (ofxApp::Phase == WILL_LOAD_CONTENT)
	// 2 - To process the loaded content (ofxApp::State == DID_DELIVER_CONTENT)
	// 3 - Once last callback just b4 the app starts Running (ofxApp::State == WILL_BEGIN_RUNNING)
	//
	// your ofxAppPhaseWillBegin(ofxApp::Phase) will get called when this happens;
	//
	// You can either run your code directly in the callback, or thread it and
	// make the loading process more friendly by using, ofxAppIsPhaseComplete()
	// ofxAppGetProgressForPhase() and ofxAppDrawPhaseProgress()
	// letting the app update while the loading happens; ofxApp will show a loading screen
	// with progressbar & such, and you can drive what to draw on screen while that happens.
	// just return true in ofxAppIsPhaseComplete() when the stage is concluded for ofxApp to proceed
	// to the next state.

	//this will be your entry point to start loading stuff
	virtual void ofxAppPhaseWillBegin(ofxApp::Phase) = 0;
	//after u are asked to start loading content, ofxApp will query every frame to check if you are done
	virtual bool ofxAppIsPhaseComplete(ofxApp::Phase){return true;} //override this method if you are loading custom content
	virtual void ofxAppDrawPhaseProgress(ofxApp::Phase, const ofRectangle & r){}; //override the loading screen drawing
	virtual string ofxAppGetStatusString(ofxApp::Phase){return "";}; //override the progress bar status text
	virtual string ofxAppGetLogString(ofxApp::Phase){return "";}; //override the text message above the progress bar (ie for showing script logs)
	virtual float ofxAppGetProgressForPhase(ofxApp::Phase){return -1;} //return [0..1] to report progressbar; -1 for indeterminate

	//this is how your app gets all the parsed objects - up to you how you store them
	virtual void ofxAppContentIsReady(const string & contentID, vector<ContentObject*> ) = 0;

	//tuio callbacks
	virtual void tuioAdded(ofxTuioCursor & tuioCursor){};
	virtual void tuioRemoved(ofxTuioCursor & tuioCursor){};
	virtual void tuioUpdated(ofxTuioCursor & tuioCursor){};

	//screen setup changed callback
	virtual void screenSetupChanged(ofxScreenSetup::ScreenSetupArg &arg){};

};
