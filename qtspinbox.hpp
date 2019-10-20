#ifndef QTSPINBOX_HPP_
#define QTSPINBOX_HPP_

#include <functional>
#include <QSpinBox>


class QtSpinBox : public QSpinBox {
  Q_OBJECT

  public:
    QtSpinBox();

    std::function<void(int)> value_changed_function;
    void setValue(int);

  public slots:
    void valueChangedSlot(int);

  private:
    bool ignore_signals;

    void wheelEvent(QWheelEvent *) override;
    void focusInEvent(QFocusEvent *) override;
    void focusOutEvent(QFocusEvent *) override;
};

#endif /* QTSPINBOX_HPP_ */
