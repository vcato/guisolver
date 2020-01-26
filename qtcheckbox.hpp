#include <functional>
#include <QCheckBox>


class QtCheckBox : public QCheckBox {
  Q_OBJECT

  public:
    std::function<void(bool)> value_changed_function;

    QtCheckBox();

    void setValue(bool);

  private slots:
    void stateChangedSlot(int);

  private:
    bool ignore_state_changed_signal = false;
};
