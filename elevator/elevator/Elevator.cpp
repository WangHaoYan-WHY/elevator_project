#include "stdafx.h"
#include "Elevator.h"
#include "Person.h"

// create the elevator's running thread, there are two lists
// one is request list, and other is the clinets' arrives list
Elevator::Elevator(int inElevatorID, int inCurrentLevel)
{
	elevatorID = inElevatorID;
	currentLevel = inCurrentLevel;
	elevatorStatus = STOP;
	runThread = true;
	elevatorRun = new thread(&Elevator::run,this);
	elevatorRun->detach();
}

Elevator::~Elevator()
{
}
bool Elevator::addArriveList(char *name, int src, int det)
{
	Person *client = new Person(name, src, det);
	if (arriveList.size() == 0)
	{
		arriveList.push_back(client);
	}else {
		for (vector<Person *>::iterator i = arriveList.begin(); i != arriveList.end(); i++)
		{
			if ((*i)->detlevel >= client->detlevel)
			{
				arriveList.insert(i, client);
				break;
			}
		}
	}
	return true;
}

bool Elevator::addRequest(char *name, int src, int det)    
{
	Person *client = new Person(name, src, det);
	if (requestList.size() == 0)
	{
		requestList.push_back(client);
	}else {
		for (vector<Person *>::iterator i = requestList.begin(); i != requestList.end(); i++)
		{
			if ((*i)->srclevel >= client->srclevel)
			{
				requestList.insert(i, client);
				break;
			}
		}
	}

  // test the print
	for (vector<Person *>::iterator i = requestList.begin(); i != requestList.end(); i++)
	{
		cout << "current " << this->elevatorID << " elevator's request client:" << endl;
		cout << (*i)->name << endl;
	}
	return true;
}

void Elevator::openDoor()
{
	cout << this->elevatorID << " elevator arrives " << currentLevel << " level" << endl;
}

void Elevator::clientArrive()	//check whether there are arriving clients
{
	for (vector<Person *>::iterator i = arriveList.begin(); i != arriveList.end();)
	{
		if ((*i)->detlevel == currentLevel)
		{
			openDoor();
			std::cout << (*i)->name << " arrives " << currentLevel << " level. and leaves " <<elevatorID<<" elevator"<< endl;
			delete((*i));
			i = arriveList.erase(i);
			continue;
		}
		if (arriveList.size() == 0) {
			this->elevatorStatus = STOP;
		}
		i++;
	}
}

void Elevator::setElevatorStatus(int state)
{
	this->elevatorStatus = state;
}
void Elevator::run()
{
	while (this->runThread)		
	{
		while (!this->requestList.empty() || !arriveList.empty())
		{
			locker.lock();

			move();

			locker.unlock();
			std::this_thread::sleep_for(std::chrono::milliseconds(3000));  // elevator's running time
		}
		elevatorStatus = STOP;
	}
}
/*
  each level check whether there are passenger want to enter the elevator
  and if yes, delete the passenger from request list and add to arrive list
*/
void Elevator::move()    // modify the move level,  
{
	// show the clients who enter the elevator
	for (vector<Person *>::iterator i = requestList.begin(); i != requestList.end(); )
	{
		if ((*i)->srclevel == currentLevel)
		{
			// if passenger enter the elevator, show that 
			cout <<this->elevatorID<<" elevator open, "<<(*i)->name << "at" << (*i)->srclevel << " level enter " << elevatorID << " elevator" << endl;
			
			if (arriveList.empty()) {
				this->elevatorStatus = (((*i)->detlevel - (*i)->srclevel) > 0 ? UP : DOWN);
			}
			addArriveList((*i)->name, (*i)->srclevel, (*i)->detlevel);	
			delete((*i));
			i = requestList.erase(i);
			continue;
		}
		i++;
	}
	
	if (elevatorStatus == UP)
	{
		currentLevel++;
		if (currentLevel == maxLevel)
			elevatorStatus = STOP;
		if (currentLevel == minLevel)
			elevatorStatus = STOP;
	}
	else if (elevatorStatus == DOWN) {
		currentLevel--;
	}
	cout << elevatorID << " elevator " << (this->elevatorStatus == UP ? " is up " : " is down ") << " arrive level " << currentLevel << endl;
	clientArrive();
}
void Elevator::stopElevator()
{
	this->runThread = false;
}
int Elevator::getElevatorStatus()
{
	return this->elevatorStatus;
}
int Elevator::getElevatorCurrentLevel()
{
	return this->currentLevel;
}