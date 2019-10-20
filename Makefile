PACKAGES=openscenegraph QtGui QtOpenGL eigen3
MOC=moc-qt4

CXXFLAGS=-W -Wall -Wundef -pedantic -std=c++17 -MD -MP -g \
  `pkg-config --cflags $(PACKAGES)`

all:
	$(MAKE) run_unit_tests
	$(MAKE) run_guisolver

run_unit_tests: \
  osgutil_test.pass transform_test.pass scenesolver_test.pass optimize_test.pass

run_guisolver: guisolver
	./guisolver

%_moc.cpp: %.hpp
	$(MOC) $*.hpp -o $@

%.pass: %
	./$*
	touch $@

guisolver: main.o osgscene.o qttimer.o qttimer_moc.o \
  osgQtGraphicsWindowQt.o osgpickhandler.o osgutil.o setupscene.o \
  sceneerror.o maketransform.o scenesolver.o optimize.o qttreewidget.o \
  qttreewidgetitem.o qtcombobox.o qtcombobox_moc.o \
  qtlineedit.o qtlineedit_moc.o qtslider.o qtslider.o \
  qtslider_moc.o qtspinbox.o qtspinbox_moc.o filltree.o \
  updatetreevalues.o
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

osgutil_test: osgutil_test.o osgutil.o
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

transform_test: transform_test.o maketransform.o
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

scenesolver_test: scenesolver_test.o \
  scenesolver.o maketransform.o sceneerror.o optimize.o
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

optimize_test: optimize_test.o optimize.o
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

clean:
	rm -f *.o *.d guisolver *_moc.cpp

-include *.d
