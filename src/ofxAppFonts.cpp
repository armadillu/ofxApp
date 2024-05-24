//
//  ofxAppFonts.cpp
//
//  Created by Oriol Ferrer Mesià aug/2016
//
//

#include "ofxAppFonts.h"
#include "ofxJsonSettings.h"
#include "ofxAppUtils.h"
#include "ofxFontStashParser.h"

void ofxAppFonts::setup(){

	//load default ofxFontStash fonts
	loadOfxAppFont(getMonoFontPath(), monospaced);
	loadOfxAppFont(getMonoBoldFontPath(), monospacedBold);

	//Setup ofxFontStash2 & add default fonts
	fonts2.pixelDensity = ofxJsonSettings::get().getFloat("Fonts/ofxFontStash2/pixelDensity");
	fonts2.setup();
	fonts2.addFont(monoID, getMonoFontPath());
	fonts2.addFont(monoBoldID, getMonoBoldFontPath());
	fonts2.addFont(regularID, getRegularFontPath());
	fonts2.addFont(boldID, getBoldFontPath());


	loadFontStashFonts();
	loadFontStash2Fonts();
	loadFontStash2Styles();
}

ofxFontStash* ofxAppFonts::getFont(const std::string& name){

	auto search = userFS_Fonts.find(name);
	if (search != userFS_Fonts.end()){
		return search->second;
	}else{
		std::string msg = "Can't find a font with that name! (" + name + ")";
		ofxApp::utils::terminateApp("ofxAppFonts", msg);
		return nullptr;
	}
}

const ofxFontStash2::Style& ofxAppFonts::getFontStyle(const std::string & styleID){

	auto it = fs2Styles.find(styleID);
	if(it != fs2Styles.end()){
		return it->second;
	}
	static ofxFontStash2::Style errStyle = ofxFontStash2::Style(monoID, 22);
	ofLogError("ofxAppFonts") << "can't find an ofxFontStash2 style named \"" << styleID << "\"";
	return errStyle;
}


void ofxAppFonts::loadFontStashFonts(){

	ofxJsonSettings & settings = ofxJsonSettings::get();

	if(settings.exists("Fonts/ofxFontStash")){

		ofJson json = settings.getJson("Fonts/ofxFontStash");

		if(json.is_object()){

			for( auto itr = json.begin(); itr != json.end(); itr++ ) { //walk all user fonts
				std::string fontName = itr.key();
				std::string fontFile = (*itr)["fontFile"];
				int atlasSize = (*itr)["atlasSize"];
				float lineH = (*itr)["lineHeight"];
				bool mipmaps = (*itr)["mipmaps"];
				int mipmapPadding = (*itr)["mipmapPadding"];
				float uiScale = (*itr)["uiScale"];

				ofxApp::utils::assertFileExists(fontFile);
				ofxFontStash * font = new ofxFontStash();

				ofLogNotice("ofxAppFonts") << "Loading User ofxFontStash Font '" << fontName << "' from '" << fontFile << "'";
				font->setup(fontFile,
							lineH, 		//line height
							atlasSize, 		//tex atlas size
							mipmaps,		//mipmaps
							mipmapPadding, 	//atlas extra intra-char padding
							uiScale 	//ui Scale (per-char texture upscale)
						   );

				if ((*itr).contains("fontSize")) {
					bool doKerning = (*itr)["kerning"];
					font->setKerning(doKerning);
				}

				auto search = userFS_Fonts.find(fontName);
				if (search == userFS_Fonts.end()){
					ofLogNotice("ofxAppFonts") << "Loading User ofxFontStash Font '" << fontName << "' from '" << fontFile << "'";
					userFS_Fonts[fontName] = font;
				}else{
					ofxApp::utils::terminateApp("ofxAppFonts", "User ofxFontStash Font with this name already exists! (" + fontName + ")");
				}

				float charSpacing = 0;
				if((*itr).contains("charSpacing")){
					charSpacing = (*itr)["charSpacing"];
					font->setCharacterSpacing(charSpacing);
					ofLogNotice("ofxAppFonts") << "Setting custom Character Spacing for font \""<< fontName << "\" : " << charSpacing;
				}
			}
		}else{
			ofxApp::utils::terminateApp("ofxAppFonts", "User Fonts (\"Fonts/ofxFontStash\")is not a Json Object! Check your \"ofxAppSettings.json\" file!");
		}
	}
}


void ofxAppFonts::loadFontStash2Fonts(){

	ofxJsonSettings & settings = ofxJsonSettings::get();

	if(settings.exists("Fonts/ofxFontStash2/fonts")){

		ofJson json = settings.getJson("Fonts/ofxFontStash2/fonts");

		if(json.is_object()){
			for( auto itr = json.begin(); itr != json.end(); itr++ ) { //walk all user fonts
				std::string fontName = itr.key();
				std::string fontFile = (*itr);
				ofxApp::utils::assertFileExists(fontFile);
				bool ok = fonts2.addFont(fontName, fontFile);
				if(!ok){
					ofLogError("ofxAppFonts") << "failed to load ofxFontStash2 Font \"" << fontName << "\" from file at \"" << fontFile << "\"";
				}
			}

			ofLogNotice("ofxAppFonts") << ofxApp::utils::getAsciiHeader("ofxFontStash2 available fonts", '#', 4, 140);
			auto ids = fonts2.getFontIDs();
			for(auto id: ids){
				ofLogNotice("ofxAppFonts") << "# \"" << id << "\"";
			}
		}else{
			ofxApp::utils::terminateApp("ofxAppFonts", "User ofxFontStash2 Fonts (\"Fonts/ofxFontStash2/fonts\")is not a Json Object! Check your \"ofxAppSettings.json\" file!");
		}
	}
}

NVGalign ofxAppFonts::getAlignmentFromString(const std::string & str){
	if(str == "NVG_ALIGN_TOP") return NVG_ALIGN_TOP;
	if(str == "NVG_ALIGN_MIDDLE") return NVG_ALIGN_MIDDLE;
	if(str == "NVG_ALIGN_BOTTOM") return NVG_ALIGN_BOTTOM;
	if(str == "NVG_ALIGN_BASELINE") return NVG_ALIGN_BASELINE;
	return (NVGalign)(NVG_ALIGN_BASELINE);
}


void ofxAppFonts::reloadFontStash2Styles(){
	loadFontStash2Styles();
}

void ofxAppFonts::loadFontStash2Styles(){

	ofxJsonSettings & settings = ofxJsonSettings::get();

	if(settings.exists("Fonts/ofxFontStash2/styles")){

		ofJson json = settings.getJson("Fonts/ofxFontStash2/styles");

		if(json.is_object()){

			for( auto itr = json.begin(); itr != json.end(); itr++ ) { //walk all user styles

				std::string styleName = itr.key();
				ofJson styleObj = (*itr);
				ofxFontStash2::Style style;
				bool ok = true;

				if(styleObj.contains("fontID")){
					style.fontID = styleObj["fontID"];
				}else{
					ofLogError("ofxAppFonts") << " style '" << styleName << "' is missing a \"fontID\"! Ignoring that style!"; ok = false;
				}

				if(styleObj.contains("fontSize")){
					style.fontSize = styleObj["fontSize"];
				}else{
					ofLogError("ofxAppFonts") << " style '" << styleName << "' is missing a \"fontSize\"! Ignoring that style!"; ok = false;
				}

				if(styleObj.contains("color")) style.color = ofxFontStash2::Parser::colorFromHex(styleObj["color"]);
				if(styleObj.contains("blur")) style.blur = styleObj["blur"];
				if(styleObj.contains("lineHeightMult")) style.lineHeightMult = styleObj["lineHeightMult"];
				if(styleObj.contains("alignV")) style.alignmentV = getAlignmentFromString(styleObj["alignV"]);

				if(ok){
					fonts2.addStyle(styleName, style);
					fs2Styles[styleName] = style;
				}
			}

			ofLogNotice("ofxAppFonts") << ofxApp::utils::getAsciiHeader("ofxFontStash2 available Styles", '#', 4, 140);
			auto styles = fonts2.getStyles();
			for(auto st : styles){
				ofLogNotice("ofxAppFonts") << "# \"" << st.first << "\": \"" << st.second.toString() << "\"";
			}
			ofLogNotice("ofxAppFonts") << string(140,'#');
		}else{
			ofxApp::utils::terminateApp("ofxAppFonts", "User ofxFontStash2 styles (\"Fonts/ofxFontStash2/styles\")is not a Json Object! Check your \"ofxAppSettings.json\" file!");
		}
	}
}


void ofxAppFonts::loadOfxAppFont(const std::string & file, ofxFontStash & font){

	std::string fontFile = file;
	int atlasSize = 512;
	float lineH = 1.2;
	bool mipmaps = false;
	int mipMapPadding = 0;
	float retinaScale = 1.0;

	ofxApp::utils::assertFileExists(fontFile);

	ofLogNotice("ofxAppFonts") << "Loading ofxApp Font '" << fontFile << "'";
	font.setup(	fontFile,
				lineH, 		//line height
				atlasSize, 		//tex atlas size
				mipmaps,		//mipmaps
				mipMapPadding, 	//atlas extra intra-char padding
				retinaScale 	//ui Scale (per-char texture upscale)
				);

}
