#include "qttimer.hpp"

#include <iostream>

using std::cerr;


QtTimer::QtTimer()
{
  connect(&timer,SIGNAL(timeout()),SLOT(timeoutSlot()));
}


void QtTimer::timeoutSlot()
{
  if (callback) {
    callback();
  }
}


void QtTimer::start()
{
  timer.setInterval(interval_in_milliseconds);
  timer.start();
}
