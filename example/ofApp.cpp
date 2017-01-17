#include "ofApp.h"
#include "ofxAppLambdas.h"

void ofApp::setup(){

	//create my custom lambdas for parsing / preparing objects
	ofxAppLambdas myLambdas = ofxAppLambdas();

	map<string, ofxApp::ParseFunctions> userLambdas;
	//put them in a map - named as the "ofxAppSettings.json" section ("content/JsonSources")
	userLambdas["CWRU"] = myLambdas.cwru;
	userLambdas["CH"] = myLambdas.ch;

	//start the ofxApp setup process
	ofxApp::get().setup(userLambdas, this);
}


void ofApp::ofxAppPhaseWillBegin(ofxApp::Phase s){
	ofLogNotice("ofApp") << "Start User Process " << ofxApp::toString(s);
	switch (s) {
		case ofxApp::Phase::WILL_LOAD_CONTENT: break;
		case ofxApp::Phase::DID_DELIVER_CONTENT: break;
		case ofxApp::Phase::WILL_BEGIN_RUNNING: break;
	}
};


void ofApp::ofxAppContentIsReady(const string & contentID, vector<ContentObject*> objs){

	ofLogNotice("ofApp") << "Content '" << contentID << "' is ready! " << objs.size() << " objects!";

	if(contentID == "CWRU"){
		for(auto o : objs){
			cwruObjects.push_back(dynamic_cast<CWRU_Object*>(o)); //cast up to CWRU_Object*
		}
	}

	if(contentID == "CH"){
		for(auto o : objs){
			CH_Object * cho = dynamic_cast<CH_Object*>(o);
			chObjects.push_back(cho); //cast up to CWRU_Object*
			//demo on how to "fish back" asset info
			vector<ofxAssets::Descriptor> assetKeys = cho->getAssetDescsWithTag("isPrimary");
			if(assetKeys.size() == 1){
				ofLogNotice("ofApp") << assetKeys.back().relativePath << " is Primary for " << cho->objectID;
			}
		}
	}
}


void ofApp::update(){
	float dt = 1./60.;
}


void ofApp::draw(){

	if(ofxApp::get().getState() == ofxApp::State::RUNNING){

		ofxApp::get().textures().drawAll(ofRectangle(100, 100, ofGetMouseX(), ofGetMouseY()));
		G_TEX("sf2")->draw(0,0);
		G_FONT("NoManSky")->draw("My Font", 20, ofGetMouseX(), ofGetMouseY());
	}
}


void ofApp::keyPressed(int key){
	if (key == 'a') {
		OFXAPP_REPORT("testAlert", "testing", 0);
	}
}


void ofApp::keyReleased(int key){

}


void ofApp::mouseMoved(int x, int y ){

}


void ofApp::mouseDragged(int x, int y, int button){

}


void ofApp::mousePressed(int x, int y, int button){

}


void ofApp::mouseReleased(int x, int y, int button){

}


void ofApp::windowResized(int w, int h){

}


void ofApp::gotMessage(ofMessage msg){

}


void ofApp::dragEvent(ofDragInfo dragInfo){
	
}

