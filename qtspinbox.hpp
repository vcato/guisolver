#ifndef QTSPINBOX_HPP_
#define QTSPINBOX_HPP_

#include <functional>
#include <QSpinBox>


class QtSpinBox : public QSpinBox {
  Q_OBJECT

  public:
    QtSpinBox();

    std::function<void(int)> value_changed_function;

  public slots:
    void valueChangedSlot(int);

  private:
    bool ignore_signals;
};

#endif /* QTSPINBOX_HPP_ */
