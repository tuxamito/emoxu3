#include "getnode.h"

#include <iostream>
#include <unistd.h>
#include <ncurses.h>
#include <algorithm>

#define VERSION "0.0.1"
#define DATE "29.06.2015"
#define DELAY_MS 1000
#define NAME "emoxu3"
#define LONGNAME "Energy Monitoring for Odroid XU3" 


int _loopms = 1000;


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
  std::cout << "  --interval, -i <time in ms> " << "Set the iteration time"<< std::endl;
  std::cout << "  --help, -h                  " << "Show this help"<< std::endl;
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

  return res;
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
  printw("A15 POWER: %.3fV, %.3fA, %.3fW\n", getNode->armuV, getNode->armuA, getNode->armuW);
  printw("A7  POWER: %.3fV, %.3fA, %.3fW\n", getNode->kfcuV, getNode->kfcuA, getNode->kfcuW);
  printw("GPU POWER: %.3fV, %.3fA, %.3fW\n", getNode->g3duV, getNode->g3duA, getNode->g3duW);
  printw("Mem POWER: %.3fV, %.3fA, %.3fW\n", getNode->memuV, getNode->memuA, getNode->memuW);
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
}

int main(int argc, const char* argv[]) {
  int ch;
  int loop;
  GetNode *getNode = new GetNode();

  parseArguments(argc, argv);

  initNCurses();

  if (getNode->OpenINA231()) {
    cout << "Error opening sensor" << endl;
    endwin();
    exit(-1);
  } else {
    // open log file
  }

  loop = 1;
  do {
    if(getData(getNode)) {
      endwin();
      exit(-1);
    }

    updateDataScreen(getNode);
    ch = getch();
    if(ch == 'q')
      loop = 0;
    usleep(_loopms * DELAY_MS);
  }while(loop);

  getNode->CloseINA231();
  delete getNode;
  endwin();

  return 0;
}
