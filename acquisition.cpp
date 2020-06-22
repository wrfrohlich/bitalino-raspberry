//----------------------------------------------------------------------------------------------------------
//
//			Name: Eng. William da Rosa Frohlich
//
//			Project: Acquisition of BITalino Data
//
//			Date: 2020.06.13
//
//----------------------------------------------------------------------------------------------------------
// Libraries

#include "bitalino.h"
#include <iostream>
#include <fstream>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

using namespace std;

//------------------------------------------------------------------------------------

#ifdef _WIN32

#include <conio.h>

bool keypressed(void)
{
	return (_kbhit() != 0);
}

//------------------------------------------------------------------------------------

#else // Linux or Mac OS

#include <sys/select.h>

bool keypressed(void)
{
   fd_set   readfds;
   FD_ZERO(&readfds);
   FD_SET(0, &readfds);

   timeval  readtimeout;
   readtimeout.tv_sec = 0;
   readtimeout.tv_usec = 0;

   return (select(FD_SETSIZE, &readfds, NULL, NULL, &readtimeout) == 1);
}

#endif

//------------------------------------------------------------------------------------

int main()
{
//------------------------------------------------------------------------------------
// Variable
	int sockfd;
	sockfd = socket(AF_INET,SOCK_DGRAM,0);
	struct sockaddr_in serv;
	char msg[44], ECG[5], EDA[5], current_time[20], first_name[10], last_name[10], name[20], new_name[21] ,control[5] = "0RUN";
	
	// Socket
	serv.sin_family = AF_INET;
	serv.sin_port = htons(18002);
	serv.sin_addr.s_addr = inet_addr("127.0.0.1");
	socklen_t m = sizeof(serv);
	
	string file_name;
	string start_name = "Database ";
	string end_name = ".txt";
	string s_day, s_month;
	
	time_t date_time;
	
	time(&date_time);
	
	struct tm*get_date = localtime(&date_time);
	struct tm*get_time = localtime(&date_time);
	
	int i_day = get_date->tm_mday;
	int i_month = get_date->tm_mon + 1;
	int i_year = get_date->tm_year + 1900;
	int i_min = get_time->tm_min;
	int i_hour = get_time->tm_hour;
	int i_sec = get_time->tm_sec;
	
	s_day = std::to_string(i_day);
	s_month = std::to_string(i_month);
	
	file_name = start_name + " " + s_month + "-" + s_day + end_name;
	
//------------------------------------------------------------------------------------

	try
	{
		// Starts the file
		ofstream myfile;
		
		// Read the patient's name
		puts("Please, insert the first name of the patient:");
		scanf("%s", first_name);
		
		puts("Please, insert the last name of the patient:");
		scanf("%s", last_name);
		
		snprintf(name, 20, "%s %s", first_name, last_name);
		
		// Waiting for Connection
		puts("Connecting to device. Please, wait a moment.");
		
		// Set the MAC address
		BITalino dev("20:16:12:22:50:18");  // Device MAC address (Windows and Linux)
		
		// Message about the defice connected
		puts("Device Connected. Press Enter to exit.");
		
		// Create the file
		myfile.open (file_name, ios::app);
		
		// Define the categories of the file
		myfile << "NAME;ECG;EDA;TIME\n";
		
		// Close the file
		myfile.close();
		
		// Get device version and show
		std::string ver = dev.version();
		printf("BITalino version: %s\n", ver.c_str());
		
		// Start acquisition of all channels at 1000 Hz
		dev.start(10, { 0, 1, 2, 3, 4, 5});
		dev.trigger({true, false});
		
//------------------------------------------------------------------------------------
// Start Loop

		// Initialize the frames vector with 1 frames
		BITalino::VFrame frames(1);
		do
		{
			// Get frames
			dev.read(frames);
			const BITalino::Frame &f = frames[0];
			
			// Open the file
			myfile.open (file_name, ios::app);
			
			// Get the current time
			date_time;
			time(&date_time);
			
			tm*get_date = localtime(&date_time);
			tm*get_time = localtime(&date_time);
			
			i_day = get_date->tm_mday;
			i_month = get_date->tm_mon + 1;
			i_year = get_date->tm_year + 1900;
			i_min = get_time->tm_min;
			i_hour = get_time->tm_hour;
			i_sec = get_time->tm_sec;

			// Show in the prompt the frame
			sprintf(msg, "%s;%d;%d;%d/%d/%d %d:%d:%d", name, f.analog[1], f.analog[2], i_day, i_month, i_year, i_hour, i_min, i_sec);
			cout << msg << "\n";			
			

			sprintf(new_name, "%d%s", 1, name);
			sprintf(ECG, "%d%d",2 , f.analog[1]);
			sprintf(EDA, "%d%d", 3, f.analog[2]);
			sprintf(current_time, "%d%d/%d/%d %d:%d:%d", 4, i_day, i_month, i_year, i_hour, i_min, i_sec);
			
			sendto(sockfd,new_name,strlen(new_name),0,(struct sockaddr *)&serv,m);
			sendto(sockfd,ECG,strlen(ECG),0,(struct sockaddr *)&serv,m);
			sendto(sockfd,EDA,strlen(EDA),0,(struct sockaddr *)&serv,m);
			sendto(sockfd,current_time,strlen(current_time),0,(struct sockaddr *)&serv,m);
			sendto(sockfd,control,strlen(control),0,(struct sockaddr *)&serv,m);

			// Save the data in the file
			myfile << msg << "\n";
			
			// Close the file
			myfile.close();
		
		// Until the key is pressed
		} while (!keypressed());
		
		dev.stop();
		
		strcpy(control, "0STOP");
		sendto(sockfd,control,strlen(control),0,(struct sockaddr *)&serv,m);
//------------------------------------------------------------------------------------
// End Loop

	}
	catch(BITalino::Exception &e)
	{
		
		printf("BITalino exception: %s\n", e.getDescription());
		
	}
	return 0;
}
//------------------------------------------------------------------------------------