#pragma once

#include "ofMain.h"
#include "ofxApp.h"

class ofApp : public ofBaseApp, public ofxAppDelegate{

public:
	
	void setup();
	void update();
	void draw();

	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y );
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);

	void setupChanged(ofxScreenSetup::ScreenSetupArg &arg);

	//ofxApp imposed - responder methods
	void startUserProcess(ofxApp::State);
	bool isUserProcessDone(ofxApp::State){return true;}
	void drawLoadingScreenForUserProcess(ofxApp::State, const ofRectangle & r){};
	float getProgressForUserProcess(ofxApp::State){return -1;}

	void contentIsReady(const string & contentID, vector<ContentObject*>){};

};