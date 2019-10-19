#ifndef QTSLIDER_HPP_
#define QTSLIDER_HPP_

#include <QSlider>
#include <functional>

struct QtLineEdit;


struct QtSlider : QWidget
{
  Q_OBJECT

  public:
    QtSlider();

    std::function<void(int)> value_changed_function;

    void setMinimum(int);
    void setMaximum(int);
    void setValue(int);

  private slots:
    void sliderValueChangedSlot(int);

  private:
    QSlider *slider_ptr = nullptr;
    QtLineEdit *line_edit_ptr = nullptr;
    bool ignore_slider_signal = false;

    QSlider &slider();
    const QSlider &slider() const;
    QtLineEdit &lineEdit();
    void updateLineEditFromSlider();
    void setLineEditValue(int);
    int sliderValue() const;
    void setSliderValue(int);
    void lineEditValueChanged(const std::string &);
};


#endif /* QTSLIDER_HPP_ */
