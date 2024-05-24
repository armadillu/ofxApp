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


	// Locate Object List in JSON user lambda //////////////////////////////////////////////////////////////////

	cwru.pointToObjects = [](ofxMtJsonParserThread::JsonStructureData & inOutData){
		ofJson & jsonRef = *(inOutData.fullJson);

		if(jsonRef["data"].is_object()){
			inOutData.objectArray = (ofJson*) &(jsonRef["data"]);
		}else{
			ofLogError("ofApp") << "JSON has unexpected format!";
			//if the json is not what we exepcted it to be,
			//let the parser know by filling it the data like this:
			inOutData.objectArray = NULL;
		}
	};



	// Parse Single JSON Object user lambda //////////////////////////////////////////////////////////////////////

	cwru.parseOneObject = [](ofxMtJsonParserThread::SingleObjectParseData & inOutData){

		//pointers mess up the json syntax somehow
		const ofJson & jsonRef = *(inOutData.jsonObj);
		string title, description, imgURL, imgSha1;

		try{ //do some parsing - catching exceptions
			title = jsonRef["title"];
			description = jsonRef["description"];
			imgURL = jsonRef["image"]["uri"];
			imgSha1 = jsonRef["image"]["chksum"];
		}catch(exception exc){
			inOutData.printMutex->lock();
			ofLogError("ofApp") << exc.what() << " WHILE PARSING OBJ " << inOutData.objectID;
			inOutData.printMutex->unlock();
		}

		CWRU_Object * o = new CWRU_Object();
		o->title = title;
		o->description = description;
		o->imgURL = imgURL;
		o->imgSha1 = imgSha1;
		inOutData.objectID;
		//in this case we dont need to set the objectID back to the parser
		//bc this json happens to be a dictionary, not a list... so its
		//smart enough to get it from there.

		//this is how we "return" the object to the parser;
		inOutData.object = dynamic_cast<ParsedObject*> (o);
	};



	// Define the assets that a single object contains /////////////////////////////////////////////////////////////

	cwru.defineObjectAssets = [](ofxApp::CatalogAssetsData & data){

		//cast from ContentObject to our native type
		CWRU_Object * cwru = dynamic_cast<CWRU_Object*>(data.object);
		string assetsPath = data.assetsLocation + "/" + cwru->getObjectUUID();
		cwru->AssetHolder::setup(assetsPath, data.assetUsagePolicy, data.assetDownloadPolicy);
		if(cwru->imgURL.size()){
			cwru->imagePath = cwru->AssetHolder::addRemoteAsset(cwru->imgURL, cwru->imgSha1, ofxChecksum::Type::SHA1);
		}
	};



	// Setup Textured Objects User Lambda /////////////////////////////////////////////////////////////////////////

	cwru.setupTexturedObject = [](ContentObject * texuredObject){

		CWRU_Object * cwruO = dynamic_cast<CWRU_Object*>(texuredObject); //cast from ContentObject to our native type
		int numImgAssets = cwruO->AssetHolder::getNumAssets();	//this will always be 1 for this example, 1 img per object

		//the assets are owned by my extended object "AssetHolder"
		cwruO->TexturedObject::setup(numImgAssets, TEXTURE_ORIGINAL); //we only use one tex size, so lets choose ORIGINAL
		cwruO->TexturedObject::setResizeQuality(CV_INTER_AREA); //define resize quality (in case we use mipmaps)

		//this example show how to tackle certain problems; would not usually be necessary
		//but we need to check the pixel size of each image for TextureLoader to be able to work;
		//so we do that here.
		for(int i = 0; i < numImgAssets; i++){
			ofxAssets::Descriptor & d = cwruO->AssetHolder::getAssetDescAtIndex(i);

			switch (d.type) {
				case ofxAssets::VIDEO: break;
				case ofxAssets::IMAGE:{
					auto info = ofxApp::utils::getImageDimensions(d.relativePath);
					if(info.valid){
						cwruO->imgSize = ofVec2f(info.width, info.height);
					}else{
						ofLogError("TexturedObject") << "cant get metadata image info at " << d.relativePath;
					}break;
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
		ofJson & jsonRef = *(inOutData.fullJson);
		if(jsonRef.is_array()){
			inOutData.objectArray = (ofJson*) &(jsonRef);
		}else{
			ofLogError("ofApp") << "JSON has unexpected format!";
			inOutData.objectArray = NULL;
		}
		//inOutData.objectArray = NULL; //make it fail on purpose
	};


	// Parse Single JSON Object user lambda ////////////////////////////////////////////////////////

	ch.parseOneObject = [](ofxMtJsonParserThread::SingleObjectParseData & inOutData){

		const ofJson & jsonRef = *(inOutData.jsonObj); //pointers mess up the json syntax somehow

		CH_Object * o = new CH_Object();

		try{ //do some parsing - catching exceptions

			o->title = ofxMtJsonParserUtils::initFromJsonString(jsonRef, "title", false, inOutData.printMutex);
			o->description = ofxMtJsonParserUtils::initFromJsonString(jsonRef, "description", false, inOutData.printMutex);
			o->objectID = ofxMtJsonParserUtils::initFromJsonString(jsonRef, "source_id", false, inOutData.printMutex);

			inOutData.objectID = o->objectID; //notice how we feed back the objectID to the parser!

			const ofJson & jsonImagesArray = jsonRef["images"];

			for( auto itr = jsonImagesArray.begin(); itr != jsonImagesArray.end(); itr++ ) {

				const ofJson & jsonImage = *itr;

				const string imgSize = "z"; //"x", "z", "b", "k" and so on

				if(jsonImage.contains(imgSize) ){
					CH_Object::CH_Image img;
					img.url = jsonImage[imgSize]["url"];
					img.sha1 = jsonImage[imgSize]["fingerprint"];
					img.imgSize.x = jsonImage[imgSize]["width"];
					img.imgSize.y = jsonImage[imgSize]["height"];
					img.isPrimary = (jsonImage[imgSize]["is_primary"] == "1");

					o->images.push_back(img);
				}
			}
		}catch(exception exc){
			inOutData.printMutex->lock();
			ofLogError("ofApp") << exc.what() << " WHILE PARSING OBJ " << inOutData.objectID;
			inOutData.printMutex->unlock();
		}

		if(!o->title.size() || !o->description.size() || !o->images.size() || !o->objectID.size()){
			delete o;
			o = nullptr; //discard object; by retruning a null object, ofxApp knows this object should be ignored
		}

		inOutData.object = dynamic_cast<ParsedObject*> (o); //this is how we "return" the object to the parser;
	};




	// Define the assets that a single object contains /////////////////////////////////////////////////////////////

	ch.defineObjectAssets = [](ofxApp::CatalogAssetsData & data){

		CH_Object * cho = dynamic_cast<CH_Object*>(data.object); //cast from ContentObject to our native type

		string assetsPath = data.assetsLocation + "/" + cho->ParsedObject::getObjectUUID();
		cho->AssetHolder::setup(assetsPath, data.assetUsagePolicy, data.assetDownloadPolicy);

		for(auto & i : cho->images){ //lets add one "Remote Asset" for each image in this object

			ofxAssets::Specs spec;
			spec.width = i.imgSize.x;
			spec.height = i.imgSize.y;

			vector<string> tags;	//note how we can "tag" assets to be able to retrieve them later
									//in this case, we are tagging the "primary image" of each object

			if(i.isPrimary){ //only images that are primary get a tag.
				tags.push_back("isPrimary");
			}

			//add N Remote Assets to the object - one per each image it contains
			i.localAssetPath = cho->AssetHolder::addRemoteAsset(i.url, i.sha1, ofxChecksum::Type::SHA1, tags, spec);
		}
	};



	// Setup Textured Objects User Lambda /////////////////////////////////////////////////////////////////////////

	ch.setupTexturedObject = [](ContentObject * texuredObject){

		CH_Object * to = dynamic_cast<CH_Object*>(texuredObject); //cast to our obj type

		int numAssets = to->images.size();

		//assets are owned by my extended object "AssetHolder"
		to->TexturedObject::setup(numAssets, TEXTURE_ORIGINAL); //we only use one tex size, so lets choose original
		to->TexturedObject::setResizeQuality(CV_INTER_AREA); //define resize quality (in case we use mipmaps)
	};
}
