PACKAGES=openscenegraph QtGui QtOpenGL eigen3
MOC=moc-qt4

CXXFLAGS=-W -Wall -Wundef -pedantic -std=c++14 -MD -MP -g \
  `pkg-config --cflags $(PACKAGES)`

all:
	$(MAKE) run_unit_tests
	$(MAKE) run_guisolver

run_unit_tests: \
  optional_test.pass \
  taggedvalueio_test.pass \
  osgutil_test.pass \
  transform_test.pass \
  markerpredicted_test.pass \
  scenestatetaggedvalue_test.pass \
  scenestateio_test.pass \
  scenesolver_test.pass \
  optimize_test.pass \
  treevalues_test.pass \
  sceneobjects_test.pass

run_guisolver: guisolver
	./guisolver

%_moc.cpp: %.hpp
	$(MOC) $*.hpp -o $@

%.pass: %
	./$*
	touch $@

DEFAULTSCENESTATE=defaultscenestate.o scenestate.o maketransform.o
TAGGEDVALUEIO=taggedvalueio.o printindent.o streamparser.o stringutil.o
SCENESTATETAGGEDVALUE=scenestatetaggedvalue.o scenestate.o

SCENESTATEIO=scenestateio.o taggedvalue.o $(SCENESTATETAGGEDVALUE) \
  $(TAGGEDVALUEIO)

SCENEERROR=sceneerror.o scenestate.o
SCENEOBJECTS=sceneobjects.o maketransform.o settransform.o scenestate.o
OPTIMIZE=optimize.o

guisolver: main.o osgscene.o qttimer.o qttimer_moc.o \
  osgQtGraphicsWindowQt.o osgpickhandler.o osgutil.o $(DEFAULTSCENESTATE) \
  $(SCENEERROR) scenesolver.o $(OPTIMIZE) \
  qttreewidgetitem.o qtcombobox.o qtcombobox_moc.o \
  qtlineedit.o qtlineedit_moc.o qtslider.o qtslider.o \
  qtslider_moc.o qtspinbox.o qtspinbox_moc.o treevalues.o \
  qttreewidget.o qttreewidget_moc.o qtmenu.o qtslot.o qtslot_moc.o \
  mainwindowcontroller.o \
  $(SCENEOBJECTS) intersector.o $(SCENESTATEIO)
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

optional_test: optional_test.o osgutil.o
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

taggedvalueio_test: taggedvalueio_test.o $(TAGGEDVALUEIO)
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

osgutil_test: osgutil_test.o osgutil.o
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

transform_test: transform_test.o maketransform.o
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

markerpredicted_test: markerpredicted_test.o scenestate.o maketransform.o
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

scenestatetaggedvalue_test: scenestatetaggedvalue_test.o \
  $(SCENESTATETAGGEDVALUE) taggedvalue.o $(TAGGEDVALUEIO)
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

scenestateio_test: scenestateio_test.o scenestate.o \
  $(DEFAULTSCENESTATE) $(SCENESTATEIO)
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

scenesolver_test: scenesolver_test.o \
  scenesolver.o maketransform.o $(SCENEERROR) $(OPTIMIZE)
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

optimize_test: optimize_test.o $(OPTIMIZE)
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

treevalues_test: treevalues_test.o $(DEFAULTSCENESTATE) treevalues.o \
  maketransform.o
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

sceneobjects_test: sceneobjects_test.o $(SCENEOBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

clean:
	rm -f *.o *.d guisolver *_moc.cpp

-include *.d
