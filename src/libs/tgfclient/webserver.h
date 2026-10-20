/***************************************************************************
                    webserver.h -- Interface file for The Gaming Framework
                             -------------------
    created              : 04/11/2015
    copyright            : (C) 2015 by MadBad
    email                : madbad82@gmail.com
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/** @file
        The Gaming Framework API (client part).
    @author     <a href=mailto:madbad82@gmail.com>MadBad</a>
*/
#ifndef __SD_WEBSERVER_H__
#define __SD_WEBSERVER_H__
#include "tgfclient.h"
#include <vector>
#include <string>
#include <ctime>
#include <curl/curl.h>
#include <car.h>

class NotificationManager;
class TGFCLIENT_API WebServer;

TGFCLIENT_API WebServer& webServer();

TGFCLIENT_API void gfuiInitWebStats();
TGFCLIENT_API void gfuiShutdownWebStats();

struct webRequest_t {
  int id;
  std::string data;
};


class NotificationManager {

	public:
		//a list of notification messages
		std::vector<std::string> msglist;

		std::clock_t animationLastExecTime; //the current time

		//constructor
		NotificationManager();

		//destructor
		~NotificationManager();

		void updateStatus();


	private:
		void startNewNotification();
		void runAnimation();
		void removeOldUi();
		void createUi();
		void updateWebserverStatusUi();

		void* screenHandle;
		void* prevScreenHandle;
		void* menuXMLDescHdle;
		int	notifyUiIdBg;//the bg image uiid
		int notifyUiIdBusyIcon; //the webserver busy icon
		std::vector<int> notifyUiId;//the text lines uiid
		bool busy;
		int textPadding;
		std::clock_t animationStartTime; //when the animation started
		std::clock_t animationRestStartTime; //when the animation started

		float totalAnimationDuration;//how much the animation should take to fully run in one direction
		float animationRestTime; //how much we should wait when we are fully displayed
		int animationDirection;
		int propertyFinalValue;
		std::vector<std::string> messageLines;
		int propertyChangeNeeded;

};



class TGFCLIENT_API WebServer {

	public:
		//local data
		int raceId;

	private:
		const char* username;
		const char* password;
		const char* url;
		bool isWebServerEnabled, raceEndSent;

		NotificationManager notifications;

		//curl
		CURLM* multi_handle;
		int handle_count;
		std::string curlServerReply;

		//dynamic data retrieved with some request to the webserver
		int userId;
		const char* sessionId;

		//configuration readers
		void readConfiguration();
		int readUserConfig(int userId);

		//async requests

		int addAsyncRequest(const std::string &data);
		int addOrderedAsyncRequest(const std::string &data);
		int pendingAsyncRequestId;
		std::vector<webRequest_t> orderedAsyncRequestQueque;

	public:
		int updateAsyncStatus();
		void updateStatus();

		//specific requests
		int sendLogin (int userId);
		int sendLogin (const char* username, const char* password);
		int sendRaceStart (tSkillLevel user_skill, const char *track_id, char *car_id, int type, void *setup, int startposition, const char *sdversion);
		int sendRaceEnd (int race_id, int endposition);
		int sendLap (int race_id, bool valid, double laptime, double fuel, int position, int wettness);

		//constructor
		WebServer();

		//destructor
		~WebServer();
};

#endif //__SD_WEBSERVER_H__
