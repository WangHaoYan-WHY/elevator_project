
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <stdio.h>
#include <Windows.h>
#include <vector>
#include <fstream>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <chrono>
#include <ctime>

#define DOWN -1
#define STOP 0
#define UP 1
#define MAX_LEVEL 30
#define MIN_LEVEL 1
#define ELEVATOR_NUM 3
#define MAX_PEOPLE_NUM 9


using namespace std;

int nScreenWidth = 500;			// Console Screen Size X (columns)
int nScreenHeight = 300;			// Console Screen Size Y (rows)
int nFieldWidth = 80;
int nFieldHeight = 122;
unsigned char *pField = nullptr;
CRITICAL_SECTION g_CS;

std::timed_mutex statistic_mutex;
int total_wait_time;
int total_serve_time;
int total_turnaround_time;
int total_consumers;

class Person {
public:
  Person();
  Person(string n, int s_level, int d_level);
  string name;
  int source;
  int destination;
  chrono::system_clock::time_point start_time;
  chrono::system_clock::time_point service_time;
  void calculate();
  ~Person();
};

Person::Person() {
  name = ""; source = 1; destination = 2;
}
Person::Person(string n, int s_level, int d_level) {
  name = n;
  source = s_level;
  destination = d_level;
}
void Person::calculate() {
  using Ms = std::chrono::milliseconds;
  chrono::system_clock::time_point tend = chrono::system_clock::now();
  std::unique_lock<std::timed_mutex> lock1(statistic_mutex, std::defer_lock);
  if (lock1.try_lock_for(Ms(100))) {
    chrono::system_clock::duration wait_length = service_time - start_time;
    chrono::system_clock::duration service_length = tend - service_time;
    chrono::system_clock::duration turnaround_length = tend - start_time;
    int wait_num = chrono::duration_cast<chrono::milliseconds>(wait_length).count();
    int service_num = chrono::duration_cast<chrono::milliseconds>(service_length).count();
    int turnaround_num = chrono::duration_cast<chrono::milliseconds>(turnaround_length).count();
    total_wait_time += wait_num;
    total_serve_time += service_num;
    total_turnaround_time += turnaround_num;
    total_consumers += 1;
    lock1.unlock();
  }
}
Person::~Person() {
}

class Elevator {
private:
  thread elevator_thread;
  int elevator_ID;
  int current_level;
  int max_level;
  int min_level;
  bool run_flag;
private:
  void move();
  void run();
  void open_door();
  void client_arrive();
public:
  mutex ml;
public:
  vector<Person> request_list;
  vector<Person> arrive_list;
  int status;
  Elevator(int id, int level);
  bool add_arrive_list(string name, int source, int destination, chrono::system_clock::time_point start);
  bool add_request(string name, int source, int destination);
  void stop_e();
  int get_status();
  void set_status(int state);
  int get_current_level();
  ~Elevator();
};

Elevator::Elevator(int id, int level) {
  elevator_thread = thread(&Elevator::run, this);
  elevator_ID = id;
  current_level = level;
  status = STOP;
  run_flag = true;
  elevator_thread.detach();
}

bool Elevator::add_arrive_list(string name, int source, int destination, chrono::system_clock::time_point start) {
  bool fl = false;
  Person one_person = Person(name, source, destination);
  one_person.start_time = start;
  one_person.service_time = chrono::system_clock::now();
  if (arrive_list.size() == 0) {
    arrive_list.push_back(one_person);
  }
  else {
    vector<Person>::iterator iter = arrive_list.begin();
    while (iter != arrive_list.end()) {
      if ((*iter).destination > one_person.destination) {
        arrive_list.insert(iter, one_person);
        fl = true;
        break;
      }
      iter++;
    }
    if (fl == false) {
      arrive_list.push_back(one_person);
    }
  }
  return true;
}

bool Elevator::add_request(string name, int source, int destination) {
  bool fl = false;
  Person one_person = Person(name, source, destination);
  one_person.start_time = chrono::system_clock::now();
  if (request_list.size() == 0) {
    request_list.push_back(one_person);
  }
  else {
    vector<Person>::iterator iter = request_list.begin();
    while (iter != request_list.end()) {
      if ((*iter).source > one_person.source) {
        request_list.insert(iter, one_person);
        fl = true;
        break;
      }
      iter++;
    }
    if (fl == false) {
      request_list.insert(iter, one_person);
    }
  }
  return true;
}

void Elevator::open_door() {
  //EnterCriticalSection(&g_CS);
  //cout << elevator_ID << " elevator arrives at level: " << current_level << endl;
  //LeaveCriticalSection(&g_CS);
}

void Elevator::client_arrive() {
  vector<Person>::iterator iter = arrive_list.begin();
  while (iter != arrive_list.end()) {
    if ((*iter).destination == current_level) {
      open_door();
      (*iter).calculate();
      //EnterCriticalSection(&g_CS);
      //cout << (*iter)->name << " has arrives level: " << current_level << " and leaves elevator: " << elevator_ID << endl;
      //LeaveCriticalSection(&g_CS);
      iter = arrive_list.erase(iter);
      continue;
    }
    if (arrive_list.size() == 0) {
      status = STOP;
    }
    iter++;
  }
  if (arrive_list.size() == 0) {
    status = STOP;
  }
}

void Elevator::set_status(int state) {
  status = state;
}

void Elevator::run() {
  while (1) {
    while (!request_list.empty() || !arrive_list.empty()) {
      unique_lock<mutex> ul(ml);
      move();
      ul.unlock();
      this_thread::sleep_for(std::chrono::milliseconds(1000));

    }
    status = STOP;
  }
}

void Elevator::move() {
  if (arrive_list.size() != 0) {
    client_arrive();
  }
  vector<Person>::iterator iter = request_list.begin();
  while (iter != request_list.end()) {
    int direction;
    if ((*iter).source - (*iter).destination > 0) {
      direction = DOWN;
    }
    else {
      direction = UP;
    }
    if ((*iter).source == current_level && direction == status && arrive_list.size() < MAX_PEOPLE_NUM) {
      //EnterCriticalSection(&g_CS);
      //cout << "passenger: " << ((*iter)->name) << " enter elevator: " << elevator_ID << " at level: " << (*iter)->source << endl;
      //LeaveCriticalSection(&g_CS);
      add_arrive_list((*iter).name, (*iter).source, (*iter).destination, (*iter).start_time);
      // delete(*iter);
      iter = request_list.erase(iter);
      continue;
    }
    else if (status == STOP) {
      if ((*iter).source == current_level) {
        status = (((*iter).destination - (*iter).source) > 0 ? UP : DOWN);
        add_arrive_list((*iter).name, (*iter).source, (*iter).destination, (*iter).start_time);
        // delete(*iter);
        iter = request_list.erase(iter);
        continue;
      }
      else {
        int temp = ((*iter).source - current_level) > 0 ? UP : DOWN;
        status = temp;
      }
    }
    iter++;
  }
  //EnterCriticalSection(&g_CS);
  //cout << "elevator " << elevator_ID << " now is at level: " << current_level;
  if (status == UP) {
    current_level++;
    if (current_level == MAX_LEVEL) {
      status = STOP;
    }
  }
  else if (status == DOWN) {
    current_level--;
    if (current_level == MIN_LEVEL) {
      status = STOP;
    }
  }
  if (status == STOP) {
    //cout << " and stop" << endl;
  }
  else {
    //cout << " and will arrive level: " << current_level << endl;
  }
  //LeaveCriticalSection(&g_CS);
}

void Elevator::stop_e() {
  run_flag = false;
}

int Elevator::get_status() {
  return status;
}

int Elevator::get_current_level() {
  return current_level;
}

Elevator::~Elevator() {
}
class Elevator_Bank {
private:
  void select_stratege();
public:
  mutex ml;
  vector<Person> requests;
  vector<Elevator*> elevators;
  Elevator_Bank();
  void init();
  void get_client();
  void elevator_select();
  ~Elevator_Bank();
};

int judge(int source, int destination) {
  if (source - destination < 0) {
    return UP;
  }
  else {
    return DOWN;
  }
}

Elevator_Bank::Elevator_Bank() {
  init();
  thread th(&Elevator_Bank::elevator_select, this);
  th.detach();
}

void Elevator_Bank::get_client() {
  string name = "";
  int source = 1;
  int destination = 2;
  Person one_person;
  vector<string> names;
  vector<int> sources;
  vector<int> destinations;
  ifstream file("consumer_list.txt");
  string content;
  if (!file.is_open())
  {
    cout << "Something wrong in openning the consumer_list.txt" << endl;
  }
  while (getline(file, content))
  {
    string temp;
    temp = "";
    int count = 0;
    for (int i = 0; i < content.size(); i++) {
      if (content[i] != ' ' && (i != content.size() - 1)) {
        temp.push_back(content[i]);
      }
      else {
        if (count == 0) {
          names.push_back(temp);
        }
        else if (count == 1) {
          sources.push_back(atoi(temp.c_str()));
        }
        else if (count == 2) {
          temp.push_back(content[i]);
          destinations.push_back(atoi(temp.c_str()));
        }
        temp = "";
        count++;
      }
    }
  }
  file.close();

  int index = 0;
  while (1) {
    if (index == names.size()) index = 0;
    if (index >= names.size() || index >= sources.size() || index >= destinations.size()) {
      continue;
    }
    cout << "name: " << index << " " << names[index] << endl;
    name = names[index];
    cout << "source: " << index << " " << sources[index] << endl;
    source = sources[index];
    cout << "destination: " << index << " " << destinations[index] << endl;
    destination = destinations[index];
    bool effect = true;
    if (source > MAX_LEVEL || source < MIN_LEVEL) effect = false;
    if (destination > MAX_LEVEL || destination < MIN_LEVEL) effect = false;
    if (destination == source) effect = false;
    if (effect) {
      unique_lock<mutex> ul(ml);
      one_person = Person(name, source, destination);
      requests.push_back(one_person);
      ul.unlock();
    }
    this_thread::sleep_for(1000ms);
    index++;
  }
}

void Elevator_Bank::elevator_select() {
  while (1) {
    this_thread::sleep_for(std::chrono::milliseconds(2000));
    while (!requests.empty()) {
      unique_lock<mutex> ul(ml);
      select_stratege();
      ul.unlock();
    }
  }
}

void Elevator_Bank::init() {
  for (int i = 0; i < ELEVATOR_NUM; i++) {
    elevators.push_back(new Elevator(i, 1));
  }
}

class node {
public:
  Elevator *elevator;
  int state;
  int rank;
  node(Elevator *el, int st, int r) {
    elevator = el;
    state = st;
    rank = r;
  }
};

void Elevator_Bank::select_stratege() {
  vector<node> elevators_message;
  for (int i = 0; i < elevators.size(); i++) {
    elevators_message.push_back(node(elevators[i], elevators[i]->get_status(), 1000));
  }
  vector<Person>::iterator iter = requests.begin();
  int num = (int)requests.size();
  while (iter != requests.end()) {
    for (int i = 0; i < elevators_message.size(); i++) {
      elevators_message[i].rank = 1000;
    }
    int flag = judge((*iter).source, (*iter).destination);
    for (int i = 0; i < elevators_message.size(); i++) {
      unique_lock<mutex> eul(elevators[i]->ml);
      elevators_message[i].elevator = elevators[i];
      elevators_message[i].state = elevators[i]->get_status();
      if (elevators_message[i].elevator->get_status() == flag) {
        if (flag == UP && elevators_message[i].elevator->get_current_level() <= (*iter).source) {
          elevators_message[i].rank = abs((*iter).source - elevators_message[i].elevator->get_current_level());
        }
        else if (flag == DOWN && elevators_message[i].elevator->get_current_level() >= (*iter).source) {
          elevators_message[i].rank = abs((*iter).source - elevators_message[i].elevator->get_current_level());
        }
      }
      else if (elevators_message[i].elevator->get_status() == STOP) {
        elevators_message[i].rank = abs((*iter).source - elevators_message[i].elevator->get_current_level());
      }
      eul.unlock();
    }
    Elevator *suitable_one = nullptr;
    int suitable_rank = 1000;
    int index = -1;
    for (int i = 0; i < elevators_message.size(); i++) {
      if (elevators_message[i].rank < suitable_rank) {
        suitable_one = elevators_message[i].elevator;
        suitable_rank = elevators_message[i].rank;
        index = i;
      }
    }
    if (index != -1) {
      unique_lock<mutex> ul(suitable_one->ml);
      suitable_one->add_request((*iter).name, (*iter).source, (*iter).destination);
      ul.unlock();
    }
    else {
      continue;
    }
    //delete((*iter));
    iter = requests.erase(iter);
    num--;
    if (num == 0) break;
  }
}

Elevator_Bank::~Elevator_Bank() {
  for (int i = 0; i < elevators.size(); i++) {
    delete(elevators[i]);
  }
}

class info_node {
public:
  int level;
  int num;
  int status;
  vector<Person> temp_request_list;
  info_node(int i, int j, int k) {
    level = i; num = j; status = k;
  }
};

void show(Elevator_Bank *bank) {
  // Create Screen Buffer
  char *screen = new char[nScreenWidth*nScreenHeight];
  for (int i = 0; i < nScreenWidth*nScreenHeight; i++) screen[i] = ' ';
  HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
  COORD size = { 500, 300 };

  //SetConsoleScreenBufferSize(hConsole, size);
  //SetConsoleActiveScreenBuffer(hConsole);
  DWORD dwBytesWritten = 0;

  pField = new unsigned char[nFieldWidth*nFieldHeight]; // Create play field buffer
  for (int x = 0; x < nFieldWidth; x++) // Board Boundary
    for (int y = 0; y < nFieldHeight; y++)
      pField[y*nFieldWidth + x] = (x == 0 || x == nFieldWidth - 1 || y == nFieldHeight - 1) ? 2 : 0;

  vector<info_node> info;
  for (int i = 0; i < ELEVATOR_NUM; i++) {
    info.push_back(info_node(bank->elevators[i]->get_current_level(), 0, STOP));
  }


  mutex mt;
  vector<Person> temp_bank_request_list;

  while (1)
  {

    this_thread::sleep_for(100ms);
    // Draw Field

    vector<int> up_number(MAX_LEVEL + 1, 0);
    vector<int> down_number(MAX_LEVEL + 1, 0);

    for (int i = 0; i < ELEVATOR_NUM; i++) {
      info[i].level = bank->elevators[i]->get_current_level();
      info[i].num = bank->elevators[i]->arrive_list.size();
      info[i].status = bank->elevators[i]->status;

      int lock_result = try_lock(bank->elevators[i]->ml, mt);
      if (lock_result == -1) {
        info[i].temp_request_list = bank->elevators[i]->request_list;
        bank->elevators[i]->ml.unlock();
        mt.unlock();
      }

      for (int j = 0; j < info[i].temp_request_list.size(); j++) {
        if (info[i].temp_request_list[j].source - info[i].temp_request_list[j].destination > 0) {
          down_number[info[i].temp_request_list[j].source]++;
        }
        else {
          up_number[info[i].temp_request_list[j].source]++;
        }
      }
    }

    int lock_result = try_lock(bank->ml, mt);
    if (lock_result == -1) {
      temp_bank_request_list = bank->requests;
      mt.unlock();
      bank->ml.unlock();
    }
    for (int i = 0; i < temp_bank_request_list.size(); i++) {
      if (temp_bank_request_list[i].source - temp_bank_request_list[i].destination > 0) {
        down_number[temp_bank_request_list[i].source]++;
      }
      else {
        up_number[temp_bank_request_list[i].source]++;
      }
    }

    for (int x = 0; x < nFieldWidth; x++)
      for (int y = 0; y < nFieldHeight; y++)
        screen[(y)*nScreenWidth + (x)] = " *#"[pField[y*nFieldWidth + x]];


    for (int x = 0; x < ELEVATOR_NUM; x++) {
      screen[x * nScreenWidth + nFieldWidth + 1] = 'E';
      screen[x * nScreenWidth + nFieldWidth + 2] = 'l';
      screen[x * nScreenWidth + nFieldWidth + 3] = 'e';
      screen[x * nScreenWidth + nFieldWidth + 4] = 'v';
      screen[x * nScreenWidth + nFieldWidth + 5] = 'a';
      screen[x * nScreenWidth + nFieldWidth + 6] = 't';
      screen[x * nScreenWidth + nFieldWidth + 7] = 'o';
      screen[x * nScreenWidth + nFieldWidth + 8] = 'r';
      screen[x * nScreenWidth + nFieldWidth + 9] = ' ';
      screen[x * nScreenWidth + nFieldWidth + 10] = '1' + x;
      screen[x * nScreenWidth + nFieldWidth + 11] = ':';

      if (info[x].status == UP) {
        screen[x * nScreenWidth + nFieldWidth + 12] = ' ';
        screen[x * nScreenWidth + nFieldWidth + 13] = ' ';
        screen[x * nScreenWidth + nFieldWidth + 14] = 'U';
        screen[x * nScreenWidth + nFieldWidth + 15] = 'P';
      }
      else if (info[x].status == DOWN) {
        screen[x * nScreenWidth + nFieldWidth + 12] = 'D';
        screen[x * nScreenWidth + nFieldWidth + 13] = 'O';
        screen[x * nScreenWidth + nFieldWidth + 14] = 'W';
        screen[x * nScreenWidth + nFieldWidth + 15] = 'N';
      }
      else {
        screen[x * nScreenWidth + nFieldWidth + 12] = 'S';
        screen[x * nScreenWidth + nFieldWidth + 13] = 'T';
        screen[x * nScreenWidth + nFieldWidth + 14] = 'O';
        screen[x * nScreenWidth + nFieldWidth + 15] = 'P';
      }
    }

    screen[4 * nScreenWidth + nFieldWidth + 1] = 'A';
    screen[4 * nScreenWidth + nFieldWidth + 2] = 'v';
    screen[4 * nScreenWidth + nFieldWidth + 3] = 'e';
    screen[4 * nScreenWidth + nFieldWidth + 4] = 'r';
    screen[4 * nScreenWidth + nFieldWidth + 5] = 'a';
    screen[4 * nScreenWidth + nFieldWidth + 6] = 'g';
    screen[4 * nScreenWidth + nFieldWidth + 7] = 'e';
    screen[4 * nScreenWidth + nFieldWidth + 8] = ' ';
    screen[4 * nScreenWidth + nFieldWidth + 9] = 'w';
    screen[4 * nScreenWidth + nFieldWidth + 10] = 'a';
    screen[4 * nScreenWidth + nFieldWidth + 11] = 'i';
    screen[4 * nScreenWidth + nFieldWidth + 12] = 't';
    screen[4 * nScreenWidth + nFieldWidth + 13] = ' ';
    screen[4 * nScreenWidth + nFieldWidth + 14] = 't';
    screen[4 * nScreenWidth + nFieldWidth + 15] = 'i';
    screen[4 * nScreenWidth + nFieldWidth + 16] = 'm';
    screen[4 * nScreenWidth + nFieldWidth + 17] = 'e';
    screen[4 * nScreenWidth + nFieldWidth + 18] = ':';
    int num1 = 0;
    if (total_consumers != 0) {
      num1 = total_wait_time / total_consumers;
    }
    screen[4 * nScreenWidth + nFieldWidth + 19] = '0' + num1 / 100000;
    num1 = num1 % 100000;
    screen[4 * nScreenWidth + nFieldWidth + 20] = '0' + num1 / 10000;
    num1 = num1 % 10000;
    screen[4 * nScreenWidth + nFieldWidth + 21] = '0' + num1 / 1000;
    num1 = num1 % 1000;
    screen[4 * nScreenWidth + nFieldWidth + 22] = '0' + num1 / 100;
    num1 = num1 % 100;
    screen[4 * nScreenWidth + nFieldWidth + 23] = '0' + num1 / 10;
    num1 = num1 % 10;
    screen[4 * nScreenWidth + nFieldWidth + 24] = '0' + num1;
    screen[4 * nScreenWidth + nFieldWidth + 25] = 'm';
    screen[4 * nScreenWidth + nFieldWidth + 26] = 's';

    screen[5 * nScreenWidth + nFieldWidth + 1] = 'A';
    screen[5 * nScreenWidth + nFieldWidth + 2] = 'v';
    screen[5 * nScreenWidth + nFieldWidth + 3] = 'e';
    screen[5 * nScreenWidth + nFieldWidth + 4] = 'r';
    screen[5 * nScreenWidth + nFieldWidth + 5] = 'a';
    screen[5 * nScreenWidth + nFieldWidth + 6] = 'g';
    screen[5 * nScreenWidth + nFieldWidth + 7] = 'e';
    screen[5 * nScreenWidth + nFieldWidth + 8] = ' ';
    screen[5 * nScreenWidth + nFieldWidth + 9] = 's';
    screen[5 * nScreenWidth + nFieldWidth + 10] = 'e';
    screen[5 * nScreenWidth + nFieldWidth + 11] = 'r';
    screen[5 * nScreenWidth + nFieldWidth + 12] = 'v';
    screen[5 * nScreenWidth + nFieldWidth + 13] = 'i';
    screen[5 * nScreenWidth + nFieldWidth + 14] = 'c';
    screen[5 * nScreenWidth + nFieldWidth + 15] = 'e';
    screen[5 * nScreenWidth + nFieldWidth + 16] = ' ';
    screen[5 * nScreenWidth + nFieldWidth + 17] = 't';
    screen[5 * nScreenWidth + nFieldWidth + 18] = 'i';
    screen[5 * nScreenWidth + nFieldWidth + 19] = 'm';
    screen[5 * nScreenWidth + nFieldWidth + 20] = 'e';
    screen[5 * nScreenWidth + nFieldWidth + 21] = ':';
    int num2 = 0;
    if (total_consumers != 0) {
      num2 = total_serve_time / total_consumers;
    }
    screen[5 * nScreenWidth + nFieldWidth + 22] = '0' + num2 / 100000;
    num2 = num2 % 100000;
    screen[5 * nScreenWidth + nFieldWidth + 23] = '0' + num2 / 10000;
    num2 = num2 % 10000;
    screen[5 * nScreenWidth + nFieldWidth + 24] = '0' + num2 / 1000;
    num2 = num2 % 1000;
    screen[5 * nScreenWidth + nFieldWidth + 25] = '0' + num2 / 100;
    num2 = num2 % 100;
    screen[5 * nScreenWidth + nFieldWidth + 26] = '0' + num2 / 10;
    num2 = num2 % 10;
    screen[5 * nScreenWidth + nFieldWidth + 27] = '0' + num2;
    screen[5 * nScreenWidth + nFieldWidth + 28] = 'm';
    screen[5 * nScreenWidth + nFieldWidth + 29] = 's';

    screen[6 * nScreenWidth + nFieldWidth + 1] = 'A';
    screen[6 * nScreenWidth + nFieldWidth + 2] = 'v';
    screen[6 * nScreenWidth + nFieldWidth + 3] = 'e';
    screen[6 * nScreenWidth + nFieldWidth + 4] = 'r';
    screen[6 * nScreenWidth + nFieldWidth + 5] = 'a';
    screen[6 * nScreenWidth + nFieldWidth + 6] = 'g';
    screen[6 * nScreenWidth + nFieldWidth + 7] = 'e';
    screen[6 * nScreenWidth + nFieldWidth + 8] = ' ';
    screen[6 * nScreenWidth + nFieldWidth + 9] = 't';
    screen[6 * nScreenWidth + nFieldWidth + 10] = 'u';
    screen[6 * nScreenWidth + nFieldWidth + 11] = 'r';
    screen[6 * nScreenWidth + nFieldWidth + 12] = 'n';
    screen[6 * nScreenWidth + nFieldWidth + 13] = 'a';
    screen[6 * nScreenWidth + nFieldWidth + 14] = 'r';
    screen[6 * nScreenWidth + nFieldWidth + 15] = 'o';
    screen[6 * nScreenWidth + nFieldWidth + 16] = 'u';
    screen[6 * nScreenWidth + nFieldWidth + 17] = 'n';
    screen[6 * nScreenWidth + nFieldWidth + 18] = 'd';
    screen[6 * nScreenWidth + nFieldWidth + 19] = ' ';
    screen[6 * nScreenWidth + nFieldWidth + 20] = 't';
    screen[6 * nScreenWidth + nFieldWidth + 21] = 'i';
    screen[6 * nScreenWidth + nFieldWidth + 22] = 'm';
    screen[6 * nScreenWidth + nFieldWidth + 23] = 'e';
    screen[6 * nScreenWidth + nFieldWidth + 24] = ':';
    int num3 = 0;
    if (total_consumers != 0) {
      num3 = total_turnaround_time / total_consumers;
    }
    screen[6 * nScreenWidth + nFieldWidth + 25] = '0' + num3 / 100000;
    num3 = num3 % 100000;
    screen[6 * nScreenWidth + nFieldWidth + 26] = '0' + num3 / 10000;
    num3 = num3 % 10000;
    screen[6 * nScreenWidth + nFieldWidth + 27] = '0' + num3 / 1000;
    num3 = num3 % 1000;
    screen[6 * nScreenWidth + nFieldWidth + 28] = '0' + num3 / 100;
    num3 = num3 % 100;
    screen[6 * nScreenWidth + nFieldWidth + 29] = '0' + num3 / 10;
    num3 = num3 % 10;
    screen[6 * nScreenWidth + nFieldWidth + 30] = '0' + num3;
    screen[6 * nScreenWidth + nFieldWidth + 31] = 'm';
    screen[6 * nScreenWidth + nFieldWidth + 32] = 's';

    int level_n = MAX_LEVEL;
    for (int y = 0; y < nFieldHeight; y++) {
      if ((y - 2) % 4 == 0 && level_n > 0) {
        screen[(y)*nScreenWidth + 2] = 'L';
        screen[(y)*nScreenWidth + 3] = ' ';
        if (level_n >= 10) {
          int n1 = level_n / 10; int n2 = level_n % 10;
          screen[(y)*nScreenWidth + 4] = '0' + n1;
          screen[(y)*nScreenWidth + 5] = '0' + n2;
        }
        else {
          screen[(y)*nScreenWidth + 4] = '0' + level_n;
        }
        screen[(y)*nScreenWidth + 7] = 'U';
        screen[(y)*nScreenWidth + 8] = 'P';
        screen[(y)*nScreenWidth + 9] = ':';
        screen[(y)*nScreenWidth + 10] = '0' + up_number[level_n] / 100;
        screen[(y)*nScreenWidth + 11] = '0' + (up_number[level_n] % 100) / 10;
        screen[(y)*nScreenWidth + 12] = '0' + up_number[level_n] % 10;

        screen[(y)*nScreenWidth + 13] = ' ';
        screen[(y)*nScreenWidth + 14] = 'D';
        screen[(y)*nScreenWidth + 15] = 'O';
        screen[(y)*nScreenWidth + 16] = 'W';
        screen[(y)*nScreenWidth + 17] = 'N';
        screen[(y)*nScreenWidth + 18] = ':';
        screen[(y)*nScreenWidth + 19] = '0' + down_number[level_n] / 100;
        screen[(y)*nScreenWidth + 20] = '0' + (down_number[level_n] % 100) / 10;
        screen[(y)*nScreenWidth + 21] = '0' + down_number[level_n] % 10;
        level_n--;
      }
    }

    int dist = 30;
    for (int i = 0; i < info.size(); i++) {
      int l = info[i].level; int n = info[i].num;
      if (l > MAX_LEVEL) l = MAX_LEVEL;
      else if (l < 1) l = 1;
      for (int j = 0; j < 4; j++) {
        if (j == 0 || j == 3) {
          for (int k = 0; k < 7; k++) {
            screen[(nFieldHeight - j - 4 * l) * nScreenWidth + i * 5 + k + dist] = '*';
          }
        }
        else {
          screen[(nFieldHeight - j - 4 * l) * nScreenWidth + i * 5 + dist] = '*';
          screen[(nFieldHeight - j - 4 * l) * nScreenWidth + i * 5 + 6 + dist] = '*';
          if (j == 1) {
            screen[(nFieldHeight - j - 4 * l) * nScreenWidth + i * 5 + 3 + dist] = '0' + n;
          }
        }
      }
      dist += 8;
    }

    // Display Frame
    //WriteConsoleOutputCharacter(hConsole, screen, nScreenWidth * nScreenHeight, { 0,0 }, &dwBytesWritten);

  }
}

int main(int argc, const char * argv[]) {
  total_wait_time = 0;
  total_serve_time = 0;
  total_turnaround_time = 0;
  total_consumers = 0;

  InitializeCriticalSection(&g_CS);
  Elevator_Bank *bank = new Elevator_Bank();
  thread t(show, bank);
  bank->get_client();
  return 0;
}
