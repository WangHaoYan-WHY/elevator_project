#pragma once
#include "Person.h"
#include <vector>
#include <thread>
#include <mutex>
#include <iostream>
#include <Windows.h>
#define UP 0
#define DOWN 1
#define STOP 2
#define MAXLEVEL 30
#define MINLEVEL 1
class Elevator
{
private:
	int elevatorID;
	int currentLevel;
	int maxLevel;
	int minLevel;
	thread *elevatorRun;
	int elevatorStatus;
	bool runThread;      // work or not
	void openDoor();
	void clientArrive();
	void move();
	void run();		    // run the elevator
public:
	std::mutex locker;
	vector<Person *> requestList;
	vector<Person *> arriveList;
	bool addArriveList(char *name, int src, int det);
	bool addRequest(char *name, int src, int det);
	~Elevator();
	Elevator(int inElevatorID, int currentLevel);
	int getElevatorStatus();
	void setElevatorStatus(int state);
	int getElevatorCurrentLevel();
	void stopElevator();
};

