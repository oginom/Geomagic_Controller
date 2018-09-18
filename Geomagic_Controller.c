/*****************************************************************************

Name:	Geomagic_Controller.c
Date:	Sep/11/2018
Author:	Masahiro Ogino
Detail:	https://github.com/oginom/Geomagic_Controller

*******************************************************************************/
#ifdef  _WIN64
#pragma warning (disable:4996)
#endif

#include <stdio.h>
#include <assert.h>

#if defined(WIN32)
# include <windows.h>
# include <conio.h>
#else
# include "conio.h"
# include <ctype.h>
typedef int BOOL;
#endif

#include <HD/hd.h>
#include <HDU/hduVector.h>
#include <HDU/hduError.h>

//for Socket Communication
#pragma  comment(lib,"ws2_32.lib")
int dstSocket;


static HDdouble gMaxAddForce = 0.01; // N
static hduVector3Dd gPosition = { 0, 0, 0 }; // mm
static HDint gButtons = 0;

static hduVector3Dd gTarget = { 0, 0, 0 }; // mm

HDSchedulerCallback gCallbackHandle = 0;

HDCallbackCode HDCALLBACK AddForceCallback(void *pUserData);

HDCallbackCode HDCALLBACK SetTargetPosCallback(void *pUserData);
HDCallbackCode HDCALLBACK GetDevicePosCallback(void *pUserData);
HDCallbackCode HDCALLBACK GetButtonsCallback(void *pUserData);

BOOL initDemo();

int keyboardLoop();
int socketLoop();

int isNGpos(hduVector3Dd p);

/*******************************************************************************
Main function.
*******************************************************************************/
int main(int argc, char* argv[])
{

	HHD hHD;
	HDErrorInfo error;
	int keypress;
	enum KeyType arrowCode;

	hHD = hdInitDevice(HD_DEFAULT_DEVICE);
	if (HD_DEVICE_ERROR(error = hdGetError()))
	{
		hduPrintError(stderr, &error, "Failed to initialize haptic device");
		Sleep(1000);
		return -1;
	}

	gCallbackHandle = hdScheduleAsynchronous(
		AddForceCallback, 0, HD_MAX_SCHEDULER_PRIORITY);
	hdEnable(HD_FORCE_OUTPUT);

	hdStartScheduler();
	if (HD_DEVICE_ERROR(error = hdGetError()))
	{
		hduPrintError(stderr, &error, "Failed to start the scheduler");
		Sleep(1000);
		return -1;
	}

	
	if (!initDemo())
	{
		printf("Demo Initialization failed\n");
		printf("Press any key to exit\n");
		getch();
		return -1;
	}

	hdGetDoublev(HD_NOMINAL_MAX_FORCE, &gMaxAddForce);

	//main loop
	socketLoop();

	hdStopScheduler();
	hdUnschedule(gCallbackHandle);
	hdDisableDevice(hHD);

	printf("wait for finish...\n");
	Sleep(5000);

	return 0;
}

//################################################################
//                  Begin Scheduler callbacks

//#
//# Main callback
//#
HDCallbackCode HDCALLBACK AddForceCallback(void *pUserData)
{
	static const HDdouble K1 = 0.05;

	HDErrorInfo error;
	//HDdouble instRate;
	//static HDdouble timer = 0;

	hdBeginFrame(hdGetCurrentDevice());

	hdGetDoublev(HD_CURRENT_POSITION, gPosition);
	hdGetIntegerv(HD_CURRENT_BUTTONS, &gButtons);

	// Use the reciprocal of the instantaneous rate as a timer.
	//hdGetDoublev(HD_INSTANTANEOUS_UPDATE_RATE, &instRate);
	//timer += 1.0 / instRate;

	// Apply a force with virtual impedance to target position.
	hduVector3Dd force;
	hduVecSubtract(force, gTarget, gPosition);
	hduVecScaleInPlace(force, K1);
	HDdouble forceMag = hduVecMagnitude(force);
	if (forceMag > gMaxAddForce)
	{
		hduVecScaleInPlace(force, gMaxAddForce / forceMag);
	}

	hdSetDoublev(HD_CURRENT_FORCE, force);

	hdEndFrame(hdGetCurrentDevice());

	// Check if an error occurred while attempting to render the force.
	if (HD_DEVICE_ERROR(error = hdGetError()))
	{
		hduPrintError(stderr, &error,
		"Error detected during scheduler callback.\n");
		if (hduIsSchedulerError(&error)) return HD_CALLBACK_DONE;
	}

	return HD_CALLBACK_CONTINUE;
}

//#
//# Modifies the target position
//#
HDCallbackCode HDCALLBACK SetTargetPosCallback(void *pUserData)
{
	HDdouble *pTarget = (HDdouble *) pUserData;

	gTarget[0] = pTarget[0];
	gTarget[1] = pTarget[1];
	gTarget[2] = pTarget[2];

	return HD_CALLBACK_DONE;
}

//#
//# Gets current device position
//#
HDCallbackCode HDCALLBACK GetDevicePosCallback(void *pUserData)
{
	HDdouble *pPosition = (HDdouble *)pUserData;

	pPosition[0] = gPosition[0];
	pPosition[1] = gPosition[1];
	pPosition[2] = gPosition[2];

	return HD_CALLBACK_DONE;
}

//#
//# Gets current buttons states
//#
HDCallbackCode HDCALLBACK GetButtonsCallback(void *pUserData)
{
	HDint *pButtons = (HDdouble *)pUserData;
	*pButtons = gButtons;
	return HD_CALLBACK_DONE;
}

//                  End Scheduler callbacks
//################################################################

BOOL initDemo(void)
{
	HDErrorInfo error;
	int calibrationStyle;
	printf("Demo Instructions\n");

	hdGetIntegerv(HD_CALIBRATION_STYLE, &calibrationStyle);
	if (calibrationStyle & HD_CALIBRATION_AUTO || calibrationStyle & HD_CALIBRATION_INKWELL)
	{
		printf("Please prepare for starting the demo by \n");
		printf("holding the device handle firmly and\n\n");
		printf("Press any key to continue...\n");
		getch();
		return 1;
	}
	if (calibrationStyle & HD_CALIBRATION_ENCODER_RESET)
	{
		printf("Please prepare for starting the demo by \n");
		printf("holding the device handle firmly and \n\n");
		printf("Press any key to continue...\n");

		getch();

		hdUpdateCalibration(calibrationStyle);
		if (hdCheckCalibration() == HD_CALIBRATION_OK)
		{
			printf("Calibration complete.\n\n");
			return 1;
		}
		if (HD_DEVICE_ERROR(error = hdGetError()))
		{
			hduPrintError(stderr, &error, "Reset encoders reset failed.");
			return 0;
		}
	}
}


int keyboardLoop()
{
	while (_kbhit()) getch(); //flush
	printf("Press 'Q' to quit.\n\n");

	/* Loop until key press. */
	while (HD_TRUE)
	{
		if (_kbhit())
		{
			int keypress = toupper(getch());
			if (keypress == 'D')
			{
				hduVector3Dd newpos;
				newpos[0] = gTarget[0] + 10;
				newpos[1] = gTarget[1];
				newpos[2] = gTarget[2];
				hdScheduleSynchronous(SetTargetPosCallback, &newpos,
					HD_DEFAULT_SCHEDULER_PRIORITY);
			}
			else if (keypress == 'A')
			{
				hduVector3Dd newpos;
				newpos[0] = gTarget[0] - 10;
				newpos[1] = gTarget[1];
				newpos[2] = gTarget[2];
				hdScheduleSynchronous(SetTargetPosCallback, &newpos,
					HD_DEFAULT_SCHEDULER_PRIORITY);
			}
			else if (keypress == 'W')
			{
				hduVector3Dd newpos;
				newpos[0] = gTarget[0];
				newpos[1] = gTarget[1];
				newpos[2] = gTarget[2] - 10;
				hdScheduleSynchronous(SetTargetPosCallback, &newpos,
					HD_DEFAULT_SCHEDULER_PRIORITY);
			}
			else if (keypress == 'S')
			{
				hduVector3Dd newpos;
				newpos[0] = gTarget[0];
				newpos[1] = gTarget[1];
				newpos[2] = gTarget[2] + 10;
				hdScheduleSynchronous(SetTargetPosCallback, &newpos,
					HD_DEFAULT_SCHEDULER_PRIORITY);
			}
			else
			{
				break;
			}
		}
		while (_kbhit()) getch(); //flush

		// print position
		hduVector3Dd position;
		hdScheduleSynchronous(GetDevicePosCallback, position,
			HD_DEFAULT_SCHEDULER_PRIORITY);
		printf("Device position: %.3f %.3f %.3f\n",
			position[0], position[1], position[2]);

		Sleep(1000);
	}
	return 0;
}

int socketLoop()
{

	char *destination = "127.0.0.1"; //localhost
	unsigned short port = 54321;

	WSADATA     wsaData;
	if (WSAStartup(MAKEWORD(2, 0), &wsaData))
	{
		printf("initialization of winsock failed.");
		getch();
		return -1;
	}

	SOCKET sock0 = socket(AF_INET, SOCK_STREAM, 0);
	if (sock0 == INVALID_SOCKET)
	{
		printf("socket : %d\n", WSAGetLastError());
		getch();
		return -1;
	}

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_port = htons(port);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(destination);
	//addr.sin_addr.S_un.S_addr = INADDR_ANY;
	if (bind(sock0, (struct sockaddr *)&addr, sizeof(addr)) != 0)
	{
		printf("bind : %d\n", WSAGetLastError());
		getch();
		return -1;
	}

	if (listen(sock0, 5) != 0)
	{
		printf("listen : %d\n", WSAGetLastError());
		getch();
		return -1;
	}

	struct sockaddr_in client;
	char buf[1024];
	int scann;
	double x0, x1, x2;
	hduVector3Dd pos0;
	int b0;
	char msg[40];
	int n;
	while (HD_TRUE)
	{
		printf("waiting for Client connects...\n");
		int len = sizeof(client);
		SOCKET sockw = accept(sock0, (struct sockaddr *)&client, &len);
		if (sockw == INVALID_SOCKET)
		{
			printf("accept : %d\n", WSAGetLastError());
			break;
		}

		while (HD_TRUE)
		{
			// 送られてきたメッセージ(COMMAND)を受け取ります
			memset(buf, 0, 1024);
			recv(sockw, buf, 1024, 0);
			if (buf[0] == '\0')   strcpy(buf, "NULL");
			printf("recv : %s\n", buf);

			switch (buf[0])
			{
			case 't': // set target position
				scann = sscanf(buf, "t %lf,%lf,%lf", &x0, &x1, &x2);
				assert(scann == 3);
				pos0[0] = x0;
				pos0[1] = x1;
				pos0[2] = x2;
				if (isNGpos(pos0))
				{
					strcpy(msg, "NG");
				}
				else
				{
					printf("new target: %lf, %lf, %lf\n", pos0[0], pos0[1], pos0[2]);
					hdScheduleSynchronous(SetTargetPosCallback, &pos0,
						HD_DEFAULT_SCHEDULER_PRIORITY);
					strcpy(msg, "OK");
				}
				break;
			case 'p': // get position
				hdScheduleSynchronous(GetDevicePosCallback, &pos0,
					HD_DEFAULT_SCHEDULER_PRIORITY);
				sprintf(msg, "%.3f,%.3f,%.3f", pos0[0], pos0[1], pos0[2]);
				break;
			case 'w': // set weight
				// TODO
				break;
			case 'v': // set vibration
				// TODO
				break;
			case 'b': // get buttons state
				hdScheduleSynchronous(GetButtonsCallback, &b0,
					HD_DEFAULT_SCHEDULER_PRIORITY);
				sprintf(msg, "%d", b0);
				break;
			default:
				strcpy(msg, "ERROR");
			}

			n = send(sockw, msg, strlen(msg), 0);
			if (n < 1)
			{
				printf("send : %d\n", WSAGetLastError());
				break;
			}
		}
		closesocket(sockw);
	}

	closesocket(sock0);
	WSACleanup();
	return 0;
}

int isNGpos(hduVector3Dd p)
{
	return p[0] < -60 || p[0] > 60 || p[1] < -50 || p[1] > 70 || p[2] < -70 || p[2] > 50;
}