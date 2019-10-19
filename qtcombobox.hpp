#ifndef QTCOMBOBOX_HPP_
#define QTCOMBOBOX_HPP_

#include <functional>
#include <QComboBox>


class QtComboBox : public QComboBox {
  Q_OBJECT

  public:
    QtComboBox();

    void setItems(const std::vector<std::string> &names);
    void setIndex(int);

    std::function<void(int)> current_index_changed_function;

  private slots:
    void currentIndexChangedSlot(int);

  private:
    void addItems(const std::vector<std::string> &enumeration_names);

    bool ignore_signals;
};

#endif /* QTCOMBOBOX_HPP_ */
