#include "getnode.h"

#include <iostream>
#include <unistd.h>
#include <ncurses.h>
#include <algorithm>
#include <string>
#include <fstream>
#include <time.h>

#define VERSION "0.0.3"
#define DATE "03.07.2015"
#define DELAY_MS 1000
#define NAME "emoxu3"
#define LONGNAME "Energy Monitoring for Odroid XU3" 

int _guicycles = 1;
int _guicyclescount = 0;
int _loopms = 1000;
int _measureEnergy = 1;
std::string _logfile = "";
std::ofstream _flog;
uint64_t _oldTime = 0;
uint64_t _aTime = 0;
uint64_t _diffTime = 0;
std::string _separator = " ";
double _wattST   = 0.0;
double _wattSA15 = 0.0;
double _wattSA7  = 0.0;
double _wattSGPU = 0.0;
double _wattSMem = 0.0;

const char* getCmdOption(const char ** begin, const char ** end, const std::string & option) {
  auto itr = std::find(begin, end, option);
  std::find(begin, end, option);

  if (itr != end && ++itr != end) {
      return *itr;
    }
  return 0;
}

bool cmdOptionExists(const char** begin, const char** end, const std::string& option) {
  return std::find(begin, end, option) != end;
}

void printHelp() {
  std::cout << LONGNAME << " v" << VERSION << " - " << DATE << std::endl;
  std::cout << "Usage: emoxu3 [options]" << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  --interval,   -i <time in ms> " << "Set the iteration time" << std::endl;
  std::cout << "  --gui-cycles, -g <n>          " << "Interval cycles needed to refresh the GUI" << std::endl;
  std::cout << "  --log,        -l <file>       " << "Log information to a file" << std::endl;
  std::cout << "  --separator,  -s <\"simbols\">  " << "Simbols that separate parameters in the log" << std::endl; 
  std::cout << "  --help,       -h              " << "Show this help" << std::endl;
}

void initNCurses() {
  initscr();
  timeout(0);
  curs_set(0);
  //start_color();
  raw();
  //keypad(stdscr, TRUE);
  noecho();
  //init_pair(1, COLOR_GREEN, COLOR_BLACK);
  //init_pair(2, COLOR_RED, COLOR_BLACK);
}
int getData(GetNode *getNode) {
  int res;

  res  = getNode->GetCPUUsage();
  res += getNode->GetINA231();
  res += getNode->GetCPUCurFreq(-1);
  res += getNode->GetGPUCurFreq();
  res += getNode->GetCPUTemp(-1);
  res += getNode->GetGPUTemp();

  return res;
}

void calculateEnergy(GetNode *getNode) {
  double dt = ((double)_diffTime / 1000.0);
  _wattSMem += (getNode->memuW) * dt;
  _wattSGPU += (getNode->g3duW) * dt;
  _wattSA7  += (getNode->kfcuW) * dt;
  _wattSA15 += (getNode->armuW) * dt;

  _wattST = _wattSMem + _wattSGPU + _wattSA7 + _wattSA15;
}

void calculateTime() {
  struct timespec spec;
  uint64_t newTime = 0;

  clock_gettime(CLOCK_MONOTONIC_RAW, &spec);
  newTime = round(spec.tv_nsec / 1000000) + (spec.tv_sec * 1000);

  if(_oldTime)
    _diffTime = newTime - _oldTime;
  _oldTime = newTime;

  _aTime += _diffTime;
}

void writeDataToLog(GetNode *getNode) {
  /* Time in ms since starting execution */
  _flog << _aTime;

  /* GPU Frequency */
  _flog << _separator << getNode->gpuFreq;

  /* CPU Frequency */
  _flog << _separator << getNode->cpuFreq[0];
  _flog << _separator << getNode->cpuFreq[1];
  _flog << _separator << getNode->cpuFreq[2];
  _flog << _separator << getNode->cpuFreq[3];
  _flog << _separator << getNode->cpuFreq[4];
  _flog << _separator << getNode->cpuFreq[5];
  _flog << _separator << getNode->cpuFreq[6];
  _flog << _separator << getNode->cpuFreq[7];

  /* CPU Usage */
  _flog << _separator << getNode->usage[0];
  _flog << _separator << getNode->usage[1];
  _flog << _separator << getNode->usage[2];
  _flog << _separator << getNode->usage[3];
  _flog << _separator << getNode->usage[4];
  _flog << _separator << getNode->usage[5];
  _flog << _separator << getNode->usage[6];
  _flog << _separator << getNode->usage[7];

  /* Temperatures */
  _flog << _separator << getNode->gpuTemp;
  _flog << _separator << getNode->cpuTemp[0];
  _flog << _separator << getNode->cpuTemp[1];
  _flog << _separator << getNode->cpuTemp[2];
  _flog << _separator << getNode->cpuTemp[3];

  /* A15 CPU Block */
  _flog << _separator << getNode->armuV;
  _flog << _separator << getNode->armuA;
  _flog << _separator << getNode->armuW;

  /* A7 CPU Block */
  _flog << _separator << getNode->kfcuV;
  _flog << _separator << getNode->kfcuA;
  _flog << _separator << getNode->kfcuW;

  /* GPU Block */
  _flog << _separator << getNode->g3duV;
  _flog << _separator << getNode->g3duA;
  _flog << _separator << getNode->g3duW;

  /* Memory Block */
  _flog << _separator << getNode->memuV;
  _flog << _separator << getNode->memuA;
  _flog << _separator << getNode->memuW;

  /* Log Energy if Enabled */
  if(_measureEnergy) {
    _flog << _separator << _wattSA15;
    _flog << _separator << _wattSA7;
    _flog << _separator << _wattSGPU;
    _flog << _separator << _wattSMem;
    _flog << _separator << _wattST;
  }

  _flog << std::endl;

}

void printNLoading() {
  clear();
  printw("Loading...");
  refresh();
}

void updateDataScreen(GetNode *getNode) {
  clear();
  printw("GPU : %dMHz %d", getNode->gpuFreq, getNode->gpuTemp);
  addch(ACS_DEGREE); printw("C\n");
  printw("cpu0: %dMHz, %d\%\n", getNode->cpuFreq[0], getNode->usage[0]);
  printw("cpu1: %dMHz, %d\%\n", getNode->cpuFreq[1], getNode->usage[1]);
  printw("cpu2: %dMHz, %d\%\n", getNode->cpuFreq[2], getNode->usage[2]);
  printw("cpu3: %dMHz, %d\%\n", getNode->cpuFreq[3], getNode->usage[3]);
  printw("CPU4: %dMHz, %d\%% %d", getNode->cpuFreq[4], getNode->usage[4], getNode->cpuTemp[0]);
  addch(ACS_DEGREE); printw("C\n");
  printw("CPU5: %dMHz, %d\%% %d", getNode->cpuFreq[5], getNode->usage[5], getNode->cpuTemp[1]);
  addch(ACS_DEGREE); printw("C\n");
  printw("CPU6: %dMHz, %d\%% %d", getNode->cpuFreq[6], getNode->usage[6], getNode->cpuTemp[2]);
  addch(ACS_DEGREE); printw("C\n");
  printw("CPU7: %dMHz, %d\%% %d", getNode->cpuFreq[7], getNode->usage[7], getNode->cpuTemp[3]);
  addch(ACS_DEGREE); printw("C\n");

  if(_measureEnergy) {
    printw("A15 POWER: %.3fV, %.3fA, %.3fW - %.4fWs\n", getNode->armuV, getNode->armuA, getNode->armuW, _wattSA15);
    printw("A7  POWER: %.3fV, %.3fA, %.3fW - %.4fWs\n", getNode->kfcuV, getNode->kfcuA, getNode->kfcuW, _wattSA7);
    printw("GPU POWER: %.3fV, %.3fA, %.3fW - %.4fWs\n", getNode->g3duV, getNode->g3duA, getNode->g3duW, _wattSGPU);
    printw("Mem POWER: %.3fV, %.3fA, %.3fW - %.4fWs\n", getNode->memuV, getNode->memuA, getNode->memuW, _wattSMem);
    printw("Total Energy: %.4f Ws - %4f Wh\n", _wattST, _wattST/3600.0);
  }
  else {
    printw("A15 POWER: %.3fV, %.3fA, %.3fW\n", getNode->armuV, getNode->armuA, getNode->armuW);
    printw("A7  POWER: %.3fV, %.3fA, %.3fW\n", getNode->kfcuV, getNode->kfcuA, getNode->kfcuW);
    printw("GPU POWER: %.3fV, %.3fA, %.3fW\n", getNode->g3duV, getNode->g3duA, getNode->g3duW);
    printw("Mem POWER: %.3fV, %.3fA, %.3fW\n", getNode->memuV, getNode->memuA, getNode->memuW);
  }

  refresh();
}

void parseArguments(int argc, const char* argv[]) {
  if(cmdOptionExists(argv, argv+argc, "-h") || cmdOptionExists(argv, argv+argc, "--help")) {
    printHelp();
    exit(0);
  }

  const char *_loopms1 = getCmdOption(argv, argv + argc, "-i");
  if (_loopms1) {
    _loopms = atoi(_loopms1);
  }
  const char *_loopms2 = getCmdOption(argv, argv + argc, "--interval");
  if (_loopms2) {
    _loopms = atoi(_loopms2);
  }

  const char *_logfile1 = getCmdOption(argv, argv + argc, "-l");
  if (_logfile1) {
    _logfile = _logfile1;
  }
  const char *_logfile2 = getCmdOption(argv, argv + argc, "--log");
  if (_logfile2) {
    _logfile = _logfile2;
  }

  if(!_logfile.empty()) {
    _flog.open(_logfile);
    if(!_flog) {
      std::cerr << "Failed to open log file: " << _logfile << std::endl;
    }
  }

  const char *_guicycles1 = getCmdOption(argv, argv + argc, "-g");
  if (_guicycles1) {
    _guicycles = atoi(_guicycles1);
  }
  const char *_guicycles2 = getCmdOption(argv, argv + argc, "--gui-cycles");
  if (_guicycles2) {
    _guicycles = atoi(_guicycles2);
  }

  const char *_separator1 = getCmdOption(argv, argv + argc, "-s");
  if (_separator1) {
    _separator = _separator1;
  }
  const char *_separator2 = getCmdOption(argv, argv + argc, "--separator");
  if (_separator2) {
    _separator = _separator2;
  }
}

void clearAll() {
  if(_flog.is_open())
    _flog.close();

  endwin();
}

int main(int argc, const char* argv[]) {
  int ch;
  int loop;
  GetNode *getNode = new GetNode();

  parseArguments(argc, argv);

  initNCurses();
  printNLoading();

  if (getNode->OpenINA231()) {
    clearAll();
    cout << "Error opening sensor" << endl;
    exit(-1);
  }

  loop = 1;

  getData(getNode);
  sleep(1);

  do {
    if(getData(getNode)) {
      clearAll();
      std::cout << "Problem reading data" << std::endl;
      exit(-1);
    }
    calculateTime();

    if(_measureEnergy) {
      calculateEnergy(getNode);
    }
    if(_flog) {
      writeDataToLog(getNode);
    }

    if(_guicycles){
      if(++_guicyclescount == _guicycles) {
	updateDataScreen(getNode);
	_guicyclescount = 0;
      }
    }

    ch = getch();
    if(ch == 'q')
      loop = 0;
    usleep(_loopms * DELAY_MS);
  }while(loop);

  getNode->CloseINA231();
  delete getNode;
  clearAll();

  return 0;
}
