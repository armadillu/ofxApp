//
//  ofxAppContent.cpp
//
//  Created by Oriol Ferrer Mesià aug/2016
//
//

#include "ofxAppContent.h"

void ofxAppContent::setup(	string ID,
					   	string jsonSrc,
						string jsonDestinationDir_,
						int numThreads_,
						int numConcurrentDownloads,
						int speedLimitKBs,
						int timeout,
						bool shouldSkipObjectTests,
						float idleTimeAfterEachDownload,
						const std::pair<string,string> & credentials,
						const ofxSimpleHttp::ProxyConfig & proxyConfig,
						const ofxApp::UserLambdas & contentCfg,
					    const ofxAssets::ObjectUsagePolicy objectUsagePolicy){


	parsedObjects.clear();
	this->ID = ID;
	this->jsonURL = jsonSrc;
	this->contentCfg = contentCfg;
	this->objectUsagePolicy = objectUsagePolicy;
	this->jsonDestinationDir = jsonDestinationDir_;
	this->numThreads = numThreads_;
	this->shouldSkipObjectTests = shouldSkipObjectTests;

	//config the http downloader if you need to (proxy, etc)
	dlc.setMaxConcurrentDownloads(numConcurrentDownloads);
	dlc.setSpeedLimit(speedLimitKBs);
	dlc.setTimeOut(timeout);
	dlc.setIdleTimeAfterEachDownload(idleTimeAfterEachDownload);
	dlc.setCredentials(credentials.first, credentials.second);
	dlc.setProxyConfiguration(proxyConfig);

	jsonParser.getHttp().setTimeOut(timeout);
	jsonParser.getHttp().setSpeedLimit(speedLimitKBs);
	jsonParser.getHttp().setProxyConfiguration(proxyConfig);
	jsonParser.getHttp().setCredentials(credentials.first, credentials.second);

	//subscribe to parsing events
	ofAddListener(jsonParser.eventJsonDownloaded, this, 	&ofxAppContent::jsonDownloaded);
	ofAddListener(jsonParser.eventJsonDownloadFailed, this, &ofxAppContent::jsonDownloadFailed);
	ofAddListener(jsonParser.eventJsonInitialCheckOK, this, &ofxAppContent::jsonInitialCheckOK);
	ofAddListener(jsonParser.eventJsonParseFailed, this, 	&ofxAppContent::jsonParseFailed);
	ofAddListener(jsonParser.eventAllObjectsParsed, this, 	&ofxAppContent::jsonContentReady);
}

void ofxAppContent::setJsonDownloadURL(string jsonURL){
	ofLogNotice("ofxAppContent") << "updating the JSON Content URL of " << ID << " to '" << jsonURL << "'";
	this->jsonURL = jsonURL;
};

void ofxAppContent::fetchContent(){
	if(state == IDLE || state == JSON_PARSE_FAILED || state == JSON_DOWNLOAD_FAILED){
		setState(DOWNLOADING_JSON);
	}else{
		ofLogError("ofxAppContent") << "Can't fetch content now!";
	}
}


void ofxAppContent::update(float dt){

	timeInState += ofGetLastFrameTime();
	jsonParser.update();
	dlc.update();
	assetChecker.update();

	switch(state){

		case DOWNLOADING_ASSETS:
			if(!dlc.isBusy()){ //downloader finished!
				ofLogNotice("ofxAppContent") << "Finished Asset downloads for \"" << ID << "\!";
				setState(FILTER_OBJECTS_WITH_BAD_ASSETS);
			}break;

		case FILTER_OBJECTS_WITH_BAD_ASSETS:
			if(timeInState > 0.1){ //show this on screen for a sec
				setState(SETUP_TEXTURED_OBJECTS);
			}
			break;

		default: break;
	}
}


void ofxAppContent::setState(ContentState s){

	state = s;
	timeInState = 0;

	switch (s) {

		case DOWNLOADING_JSON:
			//start the download and parse process
			jsonParser.downloadAndParse(jsonURL,
										jsonDestinationDir,	//directory where to save
										numThreads,			//num threads
										contentCfg.describeJsonUserLambda,
										contentCfg.parseSingleObjectUserLambda
										);
			break;

		case CHECKING_ASSET_STATUS:{
			//sadly we need to cast our objects to AssetHolder* objects to check them
			if (parsedObjects.size()) {
				vector<AssetHolder*> assetObjs;
				for (int i = 0; i < parsedObjects.size(); i++) {
					assetObjs.push_back(dynamic_cast<AssetHolder*>(parsedObjects[i]));
				}
				ofAddListener(assetChecker.eventFinishedCheckingAllAssets, this, &ofxAppContent::assetCheckFinished);
				assetChecker.checkAssets(assetObjs, numThreads);
			} else {
				setState(DOWNLOADING_ASSETS);
			}

		}break;

		case DOWNLOADING_ASSETS:
			//fill in the list
			for(int i = 0; i < parsedObjects.size(); i++){
				parsedObjects[i]->downloadMissingAssets(dlc);
			}
			totalAssetsToDownload = dlc.getNumPendingDownloads();
			dlc.setNeedsChecksumMatchToSkipDownload(true);
			dlc.startDownloading();
			break;

		case FILTER_OBJECTS_WITH_BAD_ASSETS:{

			if(!shouldSkipObjectTests){
				objectsWithBadAssets.clear();

				vector<int> badObjects;
				vector<string> badObjectsIds;

				for(int i = 0; i < parsedObjects.size(); i++){

					//do some asset integrity tests...
					bool allAssetsOK = parsedObjects[i]->areAllAssetsOK();
					bool needsAllAssetsToBeOk = objectUsagePolicy.allObjectAssetsAreOK;
					int numImgAssets = parsedObjects[i]->getAssetDescriptorsForType(ofxAssets::IMAGE).size();
					int numVideoAssets = parsedObjects[i]->getAssetDescriptorsForType(ofxAssets::VIDEO).size();
					int numAudioAssets = parsedObjects[i]->getAssetDescriptorsForType(ofxAssets::AUDIO).size();

					bool rejectObject = false;
					string rejectionReason;

					//apply all policy rules to decide if object is rejected or not
					if(needsAllAssetsToBeOk){
						if(!allAssetsOK){
							rejectObject = true;
							auto brokenAssets = parsedObjects[i]->getBrokenAssets();
							rejectionReason = ofToString(brokenAssets.size()) + " Broken Asset(s)";
						}
					}

					if(numImgAssets < objectUsagePolicy.minNumberOfImageAssets){
						rejectObject = true;
						if(rejectionReason.size()) rejectionReason += " | ";
						rejectionReason += "Not Enough Images";
						ofLogError("ofxAppContent") << "Rejecting Object '" << parsedObjects[i]->getObjectUUID()
							<< "' because doesnt have the min # of images! (" << numImgAssets << "/"
							<< objectUsagePolicy.minNumberOfImageAssets << ")" ;
					}

					if(numVideoAssets > objectUsagePolicy.minNumberOfVideoAssets){
						rejectObject = true;
						if(rejectionReason.size()) rejectionReason += " | ";
						rejectionReason += "Not Enough Videos";
						ofLogError("ofxAppContent") << "Rejecting Object '" << parsedObjects[i]->getObjectUUID()
						<< "' because doesnt have the min # of Videos! (" << numVideoAssets << "/"
						<< objectUsagePolicy.minNumberOfVideoAssets << ")" ;
					}

					if(numAudioAssets > objectUsagePolicy.minNumberOfAudioAssets){
						rejectObject = true;
						if(rejectionReason.size()) rejectionReason += " | ";
						rejectionReason += "Not Enough AudioFiles";
						ofLogError("ofxAppContent") << "Rejecting Object '" << parsedObjects[i]->getObjectUUID()
						<< "' because doesnt have the min # of Audio Files! (" << numAudioAssets << "/"
						<< objectUsagePolicy.minNumberOfAudioAssets << ")" ;
					}

					if (rejectObject){
						badObjects.push_back(i);
						badObjectsIds.push_back(parsedObjects[i]->getObjectUUID());
						objectsWithBadAssets += "Object '" + badObjectsIds.back() + "' : " + rejectionReason + "\n";
					}
				}

				for(int i = badObjects.size() - 1; i >= 0; i--){
					ofLogError("ofxAppContent") << "Dropping object " << parsedObjects[i]->getObjectUUID();
					delete parsedObjects[badObjects[i]];
					parsedObjects.erase(parsedObjects.begin() + badObjects[i]);
				}

				objectsWithBadAssets = "\nRemoved " + ofToString(badObjects.size()) + " \"" + ID + "\" objects:\n\n" + objectsWithBadAssets;
			}else{
				ofLogWarning("ofxAppContent") << "skipping Object Drop Policy Tests!! \"" << ID << "\"";
			}

		}break;

		case SETUP_TEXTURED_OBJECTS:
			for(int i = 0; i < parsedObjects.size(); i++){
				auto setupTexObjUserLambda = contentCfg.setupTexturedObjectUserLambda;
				//call the User Supplied Lambda to setup the user's TexturedObject
				setupTexObjUserLambda( parsedObjects[i] );
			}
			setState(JSON_CONTENT_READY);
			break;

		case JSON_CONTENT_READY:{
			//keep the json as a good one
			ofFile jsonFile;
			jsonFile.open(jsonParser.getJsonLocalPath());
			string jsonPath = jsonParser.getJsonLocalPath();
			string dir = ofFilePath::getEnclosingDirectory(jsonPath);
			ofFilePath::createEnclosingDirectory(dir + "knownGood");
			jsonFile.moveTo(dir + "/knownGood/" + ID + ".json", false, true);
		}break;

		default: break;
	}

	string info = "\"" + ID + "\" > " + getNameForState(state);
	ofNotifyEvent(eventStateChanged, info);
}


string ofxAppContent::getLastKnownGoodJsonPath(){
	string dir = ofFilePath::getEnclosingDirectory(jsonParser.getJsonLocalPath());
	return dir + "knownGood/" + ID + ".json";
}


string ofxAppContent::getStatus(){

	string r;
	string plainFormat = " %0.8 #0x888888 \n"; //text format for logging on screen - see ofxFontStash.h drawMultiLineColumn()
	string errorFormat = " %0.8 #0xBB0000 \n"; //text format for logging on screen - see ofxFontStash.h drawMultiLineColumn()

	switch (state) {
		case DOWNLOADING_JSON: r = plainFormat + jsonParser.getHttp().drawableString(); break;
		case JSON_DOWNLOAD_FAILED: r = errorFormat + errorMessage; break;
		case CHECKING_JSON: r = plainFormat + jsonParser.getDrawableState(); break;
		case PARSING_JSON: r = plainFormat + jsonParser.getDrawableState(); break;
		case CHECKING_ASSET_STATUS: r = plainFormat + assetChecker.getDrawableState(); break;
		case JSON_PARSE_FAILED: r = errorFormat +  errorMessage; break;
		case DOWNLOADING_ASSETS: r =  plainFormat + dlc.getDrawableInfo(true, false); break;
		case FILTER_OBJECTS_WITH_BAD_ASSETS: r = plainFormat + objectsWithBadAssets; break;
		case SETUP_TEXTURED_OBJECTS: r = plainFormat; break;
		case JSON_CONTENT_READY: r = plainFormat + "READY"; break;
		default: break;
	}
	return r;
}


float ofxAppContent::getPercentDone(){
	float p = -1.0f;
	switch (state) {
		case DOWNLOADING_JSON: p = jsonParser.getHttp().getCurrentDownloadProgress(); break;
		case CHECKING_JSON: p = -1.0; break;
		case PARSING_JSON: p = jsonParser.getTotalProgress(); break;
		case CHECKING_ASSET_STATUS: p = assetChecker.getProgress(); break;
		case FILTER_OBJECTS_WITH_BAD_ASSETS: p = -1; break;
		case DOWNLOADING_ASSETS:
			p = 1.0 - float(dlc.getNumPendingDownloads()) / totalAssetsToDownload;
			break;
		default: break;
	}
	return p;
}


// CALBACKS ////////////////////////////////////////////////////////////////////////////////////
#pragma mark Callbacks

void ofxAppContent::jsonDownloaded(ofxSimpleHttpResponse & arg){
	ofLogNotice("ofxAppContent") << "JSON download OK!";
	setState(CHECKING_JSON);
}


void ofxAppContent::jsonDownloadFailed(ofxSimpleHttpResponse & arg){
	ofLogError("ofxAppContent") << "JSON download failed!";
	errorMessage = arg.reasonForStatus + " (" + arg.url + ")";
	setState(JSON_DOWNLOAD_FAILED);
}


void ofxAppContent::jsonInitialCheckOK(){
	ofLogNotice("ofxAppContent") << "JSON Initial Check OK!";
	setState(PARSING_JSON);
}


void ofxAppContent::jsonParseFailed(){
	ofLogError("ofxAppContent") << "JSON Parse Failed!";
	setState(JSON_PARSE_FAILED);
}


void ofxAppContent::jsonContentReady(vector<ParsedObject*> &parsedObjects_){
	ofLogNotice("ofxAppContent") << "JSON Content Ready! " << parsedObjects.size() << " Objects received.";
	parsedObjects.reserve(parsedObjects_.size());
	for(auto o : parsedObjects_){
		//parsedObjects.push_back((ContentObject*)o);		
		//ContentObject * co = static_cast<ContentObject*>(o);
		ContentObject * co = (ContentObject*)(o);

		parsedObjects.push_back(co);
	}
	setState(CHECKING_ASSET_STATUS);
}


void ofxAppContent::assetCheckFinished(){
	ofLogNotice("ofxAppContent") << "Asset Check Finished!";
	setState(DOWNLOADING_ASSETS);
}


string ofxAppContent::getNameForState(ofxAppContent::ContentState state){

	switch (state) {
		case IDLE: return "IDLE";
		case DOWNLOADING_JSON: return "DOWNLOADING_JSON";
		case JSON_DOWNLOAD_FAILED: return "JSON_DOWNLOAD_FAILED";
		case CHECKING_JSON: return "CHECKING_JSON";
		case JSON_PARSE_FAILED: return "JSON_PARSE_FAILED";
		case PARSING_JSON: return "PARSING_JSON";
		case CHECKING_ASSET_STATUS: return "CHECKING_ASSET_STATUS";
		case DOWNLOADING_ASSETS: return "DOWNLOADING_ASSETS";
		case FILTER_OBJECTS_WITH_BAD_ASSETS: return "FILTER_OBJECTS_WITH_BAD_ASSETS";
		case SETUP_TEXTURED_OBJECTS: return "SETUP_TEXTURED_OBJECTS";
		case JSON_CONTENT_READY: return "JSON_CONTENT_READY";
		case NUM_CONTENT_MANAGER_STATES: return "NUM_CONTENT_MANAGER_STATES";
		default: break;
	}
	return "UNKNOWN STATE";
}
