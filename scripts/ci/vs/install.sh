#!/usr/bin/env bash
set -e

echo "Executing ci/vs/install.sh"

export OF_ROOT="$APPVEYOR_BUILD_FOLDER/../openFrameworks"
export OF_ADDONS=$OF_ROOT/addons

pwd;
echo "OF_ADDONS: $OF_ADDONS"

ADDONS="armadillu/ofxFontStash armadillu/ofxFontStash2 armadillu/ofxTuio armadillu/ofxJSON armadillu/ofxSuperLog armadillu/ofxAutoTexture armadillu/ofxMTJsonParser armadillu/ofxSimpleHttp armadillu/ofxTagSystem armadillu/ofxStateMachine armadillu/ofxAssets armadillu/ofxThreadSafeLog armadillu/ofxProgressiveTextureLoad armadillu/ofxTexturedObject armadillu/ofxMullion armadillu/ofxJsonSettings armadillu/ofxTimeMeasurements armadillu/ofxRemoteUI armadillu/ofxHistoryPlot armadillu/ofxAnimatable armadillu/ofxScreenSetup armadillu/ofxSensu armadillu/ofxGoogleAnalytics armadillu/ofxInterface"


cd $OF_ADDONS

for ADDON in $ADDONS
do
  echo "Cloning addon '$ADDON' to " `pwd`
  git clone --depth=1  https://github.com/$ADDON.git
done

cd -