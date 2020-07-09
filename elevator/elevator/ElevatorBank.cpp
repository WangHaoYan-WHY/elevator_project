#include "stdafx.h"
#include "ElevatorBank.h"


ElevatorBank::ElevatorBank()
{
	initElevatorBank();
  // create child thread, used to select the best elevator
  // and main thread is to get the requests
	thread th(&ElevatorBank::elevatorSelec, this);
	th.detach();
}

ElevatorBank::~ElevatorBank()
{
	delete(elevatorOne);
	delete(elevatorTwo);
}
void ElevatorBank::getClient()   // get the passenger's request
{
	char name[16];
	int srcLevel;
	int detLevel;
	Person *client;
	while (1) {
		cin >> name >> srcLevel >> detLevel;
		bool effect = 1;
		if (srcLevel > MAXLEVEL ||srcLevel < MINLEVEL) 
			effect = false;
		if (detLevel > MAXLEVEL || detLevel < MINLEVEL) 
			effect = false;
		if (detLevel == srcLevel)
			effect = false;
		if (effect) {					// input the valid the record
			bankLocker.lock();			// use mutex to lock the operate to elevator
			client = new Person(name, srcLevel, detLevel);
			allRequest.push_back(client);
			bankLocker.unlock();		// unlock the mutex
		}else{
			printf("Please assert your enter message: name enter_elevator leave_elevator\n");
		}
	}
}
int JudgeUpOrDown(int src,int det)    //judge client is up or down
{
	int flag;

	if (det - src > 0) {
		flag = UP;
	}else if (det - src < 0) {
		flag = DOWN;
	}

	return flag;
}

void ElevatorBank::elevatorSelec()		   
{
	while (1) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		bankLocker.lock();
		while (!allRequest.empty())
		{
			strategeChooseElevator();
		}
		bankLocker.unlock();
	}
}
void ElevatorBank::initElevatorBank()    // create two elevators
{
	elevatorOne = new Elevator(1, 1);
	elevatorTwo = new Elevator(2, 1);
}
void ElevatorBank::strategeChooseElevator()			
{
	typedef struct choice
	{
		Elevator *elevator;
		int sta;
		int rank;              // select the lower rank elevator to response
	} slec;		
	slec elevatorOneMeg = { elevatorOne,elevatorOne->getElevatorStatus(),1000 };
	slec elevatorTwoMeg = { elevatorTwo,elevatorTwo->getElevatorStatus(),1000 };

	elevatorOne->locker.lock();
	elevatorTwo->locker.lock();

	int num = allRequest.size();
	for (vector<Person *>::iterator i = allRequest.begin(); i < allRequest.end(); i++)
	{
		elevatorOneMeg.rank = 1000;
		elevatorTwoMeg.rank = 1000;
		// judge the 1 elevator's rank
    // sign the client's request is up or down
		int flag = JudgeUpOrDown((*i)->srclevel, (*i)->detlevel);
		if (elevatorOneMeg.sta == flag) {
			if (!elevatorOne->arriveList.empty()) {
				if (flag == UP && elevatorOne->getElevatorCurrentLevel() <= (*i)->srclevel)
				{
					elevatorOneMeg.rank = abs((*i)->srclevel - elevatorOne->getElevatorCurrentLevel());
				}
				else if (DOWN == flag && elevatorOne->getElevatorCurrentLevel() >= (*i)->detlevel) {
					elevatorOneMeg.rank = abs(elevatorOne->getElevatorCurrentLevel() - (*i)->srclevel);
				}
			}
		}else if (elevatorOneMeg.sta == STOP) {
				elevatorOneMeg.rank = abs(elevatorOne->getElevatorCurrentLevel() - (*i)->srclevel);
		}
		// judge the 2 elevator's rank
		if (elevatorTwoMeg.sta == flag) {
      // judge whether current arrive list is NULL
			if (!elevatorTwo->arriveList.empty()) {
				if (flag == UP && elevatorTwo->getElevatorCurrentLevel() <= (*i)->srclevel)
				{
					elevatorTwoMeg.rank = abs((*i)->srclevel - elevatorTwo->getElevatorCurrentLevel());
				}
				else if (DOWN == flag && elevatorTwo->getElevatorCurrentLevel() >= (*i)->detlevel) {
					elevatorTwoMeg.rank = abs(elevatorTwo->getElevatorCurrentLevel() - (*i)->srclevel);
				}
			}
		}else if (elevatorTwoMeg.sta == STOP) {
				elevatorTwoMeg.rank = abs(elevatorTwo->getElevatorCurrentLevel() - (*i)->srclevel);
		}

		// there are no suitable elevator, skip current request and check next time
		if (elevatorOneMeg.rank == 1000 && elevatorTwoMeg.rank == 1000)  
		{
			continue;
		}
	    // select suitable elevator and add the client
		if (elevatorOneMeg.rank <= elevatorTwoMeg.rank)
		{
			elevatorOneMeg.elevator->addRequest((*i)->name, (*i)->srclevel, (*i)->detlevel);
			
			if (elevatorOne->getElevatorStatus() == STOP) {
				int temp = ((*i)->srclevel - elevatorOne->getElevatorCurrentLevel()) > 0 ? UP : DOWN;
				elevatorOne->setElevatorStatus(temp);
			}
			//printf("bank client add to1\n");
		}else if (elevatorOneMeg.rank > elevatorTwoMeg.rank) {
			elevatorTwoMeg.elevator->addRequest((*i)->name, (*i)->srclevel, (*i)->detlevel);

			if (elevatorTwo->getElevatorStatus() == STOP) {
				int temp = ((*i)->srclevel - elevatorTwo->getElevatorCurrentLevel()) > 0 ? UP : DOWN;
				elevatorTwo->setElevatorStatus(temp);
			}
			//printf("bank client add to2\n");
		}
		delete((*i));
		i = allRequest.erase(i);
		num--;
		if (num == 0) break;
	}
	elevatorOne->locker.unlock();
	elevatorTwo->locker.unlock();
}