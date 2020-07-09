#pragma once
#include <vector>
#include <iostream>
#include <string>
#include <mutex>
#include <thread>
#include <stdlib.h>
#include "Person.h"
#include "Elevator.h"
using namespace std;
class ElevatorBank
{
  /*
  client message enter, at first there are two elevators, and monitor the inputs.
  This object contains two threads, one thread select the best elevator and add
  the passenger to the request list, and other thrread listen the client inputs
  */
private:
	mutex bankLocker;
	Elevator *elevatorOne;
	Elevator *elevatorTwo;
	void strategeChooseElevator();
public:
	void initElevatorBank();
	vector<Person *> allRequest;
	void getClient();
	void elevatorSelec();
	ElevatorBank();
	~ElevatorBank();
};

