#ifndef QTTIMER_HPP_
#define QTTIMER_HPP_

#include <functional>
#include <QTimer>


class QtTimer : public QObject {
  Q_OBJECT

  public:
    QtTimer();
    std::function<void()> callback;
    int interval_in_milliseconds = 1000;
    void start();

  private:
    QTimer timer;

  private slots:
    void timeoutSlot();
};


#endif /* QTTIMER_HPP_ */
