//
//  ofxAppErrorReporter.h
//  PosterPlayback
//
//  Created by Oriol Ferrer Mesià on 10/17/16.
//
//

#pragma once
#include "ofMain.h"
#include "ofxAppUtils.h"
#include "ofxSuperLog.h"

class ofxSensu;

class ofxAppErrorReporter{

public:
	
	const std::string indent = " + ";

	void setup(std::string host, int port, std::string email, bool shouldReportErrors,
		std::string hostName, std::string hostIP, std::string binaryName, bool attachGitStatus);
	
	void setup(std::string host, int port, const vector<std::string>& emails, bool shouldReportErrors,
		std::string hostName, std::string hostIP, std::string binaryName, bool attachGitStatus);
	
	void send(const std::string& alertName, const std::string& msg, int level02, const std::string& filePath = "");
	void send(const std::string& alertName, const std::string& msg, int level02, const vector<std::string>& filePaths);
	
	
	
	void sendBlocking(const std::string& alertName, const std::string& msg, int level02, const std::string& filePath = "");
	void sendBlocking(const std::string& alertName, const std::string& msg, int level02, const vector<std::string>& filePaths);
	

protected:
	
	void logSend(const std::string & alertName, const std::string & msg, int level02, bool blocking);
	std::string addContext(const std::string& msg);

	vector<std::string> emails;
	ofxSensu * sensu = nullptr;
	bool enabled;
	std::string hostInfo;
	std::string gitRev;
	std::string gitStatus;
};

