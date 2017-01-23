//
//  ofxAppParsers.cpp
//  BaseApp
//
//  Created by Oriol Ferrer Mesià on 12/8/16.
//
//

#include "ofxAppParsers.h"
#include "CH_Object.h"
#include "CWRU_Object.h"

ofxAppParsers::ofxAppParsers(){

/////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CWRU ////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////


	// Locate Object List in JSON user lambda //////////////////////////////////////////////////////
	cwru.pointToObjects = [](ofxMtJsonParserThread::JsonStructureData & inOutData){
		ofxJSONElement & jsonRef = *(inOutData.fullJson);
		if(jsonRef["data"].isObject()){
			inOutData.objectArray = (ofxJSONElement*) &(jsonRef["data"]);
		}else{
			ofLogError("ofApp") << "JSON has unexpected format!";
			//if the json is not what we exepcted it to be,
			//let the parser know by filling it the data like this:
			inOutData.objectArray = NULL;
		}
	};

	// Parse Single JSON Object user lambda ////////////////////////////////////////////////////////
	cwru.parseOneObject = [](ofxMtJsonParserThread::SingleObjectParseData & inOutData){

		const ofxJSONElement & jsonRef = *(inOutData.jsonObj); //pointers mess up the json syntax somehow
		string title, description, imgURL, imgSha1;

		try{ //do some parsing - catching exceptions
			title = jsonRef["title"].asString();
			description = jsonRef["description"].asString();
			imgURL = jsonRef["image"]["uri"].asString();
			imgSha1 = jsonRef["image"]["chksum"].asString();
		}catch(Exception exc){
			inOutData.printMutex->lock();
			ofLogError("ofApp") << exc.what() << " " << exc.message() <<
			" " << exc.displayText() << " WHILE PARSING OBJ " << inOutData.objectID;
			inOutData.printMutex->unlock();
		}

		CWRU_Object * o = new CWRU_Object();
		o->title = title;
		o->description = description;
		inOutData.objectID; //in this case we dont need to set the objectID back to the parser
							//bc this json happens to be a dictionary, not a list... so its
							//smart enough to get it from there.

		// ASSET HOLDER SETUP ///////////////////////////////
		// where to download the assets to,
		// our download and usage policies, and what assets do we own
		string assetsDir = ((ofxJSONElement)*(inOutData.userData))["assetsLocation"].asString();
		string assetsPath = assetsDir + "/" + inOutData.objectID;

		ofxAssets::DownloadPolicy assetDownloadPolicy = ofxApp::get().getAssetDownloadPolicy();
		ofxAssets::UsagePolicy assetUsagePolicy = ofxApp::get().getAssetUsagePolicy(); 
		o->AssetHolder::setup(assetsPath, assetUsagePolicy, assetDownloadPolicy);

		if(imgURL.size()){
			o->imagePath = o->addRemoteAsset(imgURL, imgSha1);
		}

		inOutData.object = dynamic_cast<ParsedObject*> (o); //this is how we "return" the object to the parser;
	};


	// Setup Textured Objects User Lambda /////////////////////////////////////////////////////////
	cwru.setupTexturedObject = [](ContentObject * texuredObject){

		CWRU_Object * to = dynamic_cast<CWRU_Object*>(texuredObject); //cast from ContentObject to our native type
		int numImgAssets = to->getNumAssets();	//this will always be 1 for this example, 1 img per object
												//this method is part of AssetHolder

		//the assets are owned by my extended object "AssetHolder"
		to->TexturedObject::setup(numImgAssets, TEXTURE_ORIGINAL); //we only use one tex size, so lets choose ORIGINAL
		to->TexturedObject::setResizeQuality(CV_INTER_AREA); //define resize quality (in case we use mipmaps)
		ofPixels pix;

		for(int i = 0; i < numImgAssets; i++){
			ofxAssets::Descriptor & d = to->getAssetDescAtIndex(i);

			switch (d.type) {
				case ofxAssets::VIDEO: break;
				case ofxAssets::IMAGE:{
					//preload images once, to find out their dimensions that we need to know beforehand;
					//this is a TexturedObject requirement for the Progressive texture loader to alloc b4 loading
					//Ideally the cms provides img dimensions to avoid startup overhead.

					if(ofLoadImage(pix, d.relativePath)){
						to->imgSize = ofVec2f(pix.getWidth(), pix.getHeight());
					}else{
						ofLogError("TexturedObject") << "cant load image at " << d.relativePath;
					}
					break;
				}
				default: break;
			}
		}
	};

/////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CH //////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Locate Object List in JSON user lambda //////////////////////////////////////////////////////
	ch.pointToObjects = [](ofxMtJsonParserThread::JsonStructureData & inOutData){
		ofxJSONElement & jsonRef = *(inOutData.fullJson);
		if(jsonRef.isArray()){
			inOutData.objectArray = (ofxJSONElement*) &(jsonRef);
		}else{
			ofLogError("ofApp") << "JSON has unexpected format!";
			inOutData.objectArray = NULL;
		}
		//inOutData.objectArray = NULL; //make it fail on purpose
	};

	// Parse Single JSON Object user lambda ////////////////////////////////////////////////////////
	ch.parseOneObject = [](ofxMtJsonParserThread::SingleObjectParseData & inOutData){

		const ofxJSONElement & jsonRef = *(inOutData.jsonObj); //pointers mess up the json syntax somehow

		CH_Object * o = new CH_Object();


		map<string, bool> isPrimary; //lets store separately if the img is primary or not - we'll use that later

		try{ //do some parsing - catching exceptions

			o->title = ofxMtJsonParserUtils::initFromJsonString(jsonRef, "title", false, inOutData.printMutex);
			o->description = ofxMtJsonParserUtils::initFromJsonString(jsonRef, "description", false, inOutData.printMutex);
			o->objectID = ofxMtJsonParserUtils::initFromJsonString(jsonRef, "source_id", false, inOutData.printMutex);

			inOutData.objectID = o->objectID; //notice how we feed back the objectID to the parser!

			Json::Value & jsonImagesArray = (Json::Value &)jsonRef["images"];

			for( auto itr = jsonImagesArray.begin(); itr != jsonImagesArray.end(); itr++ ) {

				Json::Value & jsonImage = (Json::Value &)*itr;

				const string imgSize = "z"; //"x", "z", "b" and so on

				if(jsonImage[imgSize].isObject()){
					//if(jsonImage[imgSize]["is_primary"].asString() == "1"){ //only primary images
						CH_Object::CH_Image img;
						img.url = jsonImage[imgSize]["url"].asString();
						img.sha1 = jsonImage[imgSize]["fingerprint"].asString();
						img.imgSize.x = jsonImage[imgSize]["width"].asInt();
						img.imgSize.y = jsonImage[imgSize]["height"].asInt();
						isPrimary[img.url] = (jsonImage[imgSize]["is_primary"].asString() == "1");

						o->images.push_back(img);
					//}
				}
			}

			//imgURL = ofxMtJsonParserUtils::initFromJsonString(jsonRef, "description", false, inOutData.printMutex);
			//imgURL = jsonRef["image"]["uri"].asString();
			//imgSha1 = jsonRef["image"]["chksum"].asString();
		}catch(Exception exc){
			inOutData.printMutex->lock();
			ofLogError("ofApp") << exc.what() << " " << exc.message() << " " << exc.displayText() << " WHILE PARSING OBJ " << inOutData.objectID;
			inOutData.printMutex->unlock();
		}

		if(o->title.size() && o->description.size() && o->images.size() && o->objectID.size()){

			// ASSET HOLDER SETUP //
			//setup our AssetHoler structures - we need to know where to download the assets to,
			//our download and usage policies, and what assets do we own
			string assetsDir = ((ofxJSONElement)*(inOutData.userData))["assetsLocation"].asString();
			string assetsPath = assetsDir + "/" + o->objectID;

			ofxAssets::DownloadPolicy assetDownloadPolicy = ofxApp::get().getAssetDownloadPolicy(); //TODO this is slooow!
			ofxAssets::UsagePolicy assetUsagePolicy = ofxApp::get().getAssetUsagePolicy(); //TODO this is slooow!
			o->AssetHolder::setup(assetsPath, assetUsagePolicy, assetDownloadPolicy);

			for(auto & i : o->images){ //lets add one "Remote Asset" for each image in this object

				ofxAssets::Specs spec;
				spec.width = i.imgSize.x;
				spec.height = i.imgSize.y;

				vector<string> tags;	//note how we can "tag" assets to be able to retrieve them later
										//in this case, we are tagging the "primary image" of each object as so

				if(isPrimary[i.url]){ //only images that are primary get a tag.
					tags.push_back("isPrimary");
				}
				i.localAssetPath = o->addRemoteAsset(i.url, i.sha1, tags, spec);
			}

		}else{ //discard object
			delete o;
			o = nullptr;
		}

		inOutData.object = dynamic_cast<ParsedObject*> (o); //this is how we "return" the object to the parser;
	};


	// Setup Textured Objects User Lambda /////////////////////////////////////////////////////////
	ch.setupTexturedObject = [](ContentObject * texuredObject){

		CH_Object * to = dynamic_cast<CH_Object*>(texuredObject); //cast to our obj type

		int numAssets = to->images.size();

		//assets are owned by my extended object "AssetHolder"
		to->TexturedObject::setup(numAssets, TEXTURE_ORIGINAL); //we only use one tex size, so lets choose original
		to->TexturedObject::setResizeQuality(CV_INTER_AREA); //define resize quality (in case we use mipmaps)
		ofPixels pix;

		for(int i = 0; i < numAssets; i++){
			ofxAssets::Descriptor & d = to->getAssetDescAtIndex(i);

			switch (d.type) {
				case ofxAssets::VIDEO: break; //you might want to handle video thumbnails here?
				case ofxAssets::IMAGE:{
					//preload images once, to find out their dimensions that we need to know beforehand
					//for the Progressive texture loader to alloc b4 loading
					//TODO! cms should provide img dimensions to avoid startup overhead!

					break;
				}
				default: break;
			}
		}
	};
}
