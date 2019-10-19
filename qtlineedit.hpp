#include <QLineEdit>
#include <functional>


class QtLineEdit : public QLineEdit {
  Q_OBJECT

  public:
    QtLineEdit();

    std::function<void(std::string)> text_changed_function;

    void setText(const std::string &);

  private slots:
    void textChangedSlot(const QString &);

  private:
    bool ignore_signals = false;
};
