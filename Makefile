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
  scenestate_test.pass \
  globaltransform_test.pass \
  scenestatetaggedvalue_test.pass \
  scenestateio_test.pass \
  scenesolver_test.pass \
  optimize_test.pass \
  treevalues_test.pass \
  sceneobjects_test.pass \
  observedscene_test.pass

run_guisolver: guisolver
	./guisolver

%_moc.cpp: %.hpp
	$(MOC) $*.hpp -o $@

%.pass: %
	./$*
	touch $@

DEFAULTSCENESTATE=defaultscenestate.o scenestate.o maketransform.o
TAGGEDVALUEIO=taggedvalueio.o printindent.o streamparser.o stringutil.o
SCENESTATETAGGEDVALUE=scenestatetaggedvalue.o scenestate.o taggedvalue.o
SCENESTATEIO=scenestateio.o $(SCENESTATETAGGEDVALUE) $(TAGGEDVALUEIO)

SCENEERROR=sceneerror.o scenestate.o
SCENEOBJECTS=sceneobjects.o maketransform.o settransform.o scenestate.o
OPTIMIZE=optimize.o
MAINWINDOWCONTROLLER=mainwindowcontroller.o observedscene.o
OBSERVEDSCENE=observedscene.o \
  $(SCENESTATETAGGEDVALUE) treevalues.o $(SCENEOBJECTS)

guisolver: main.o osgscene.o qttimer.o qttimer_moc.o \
  osgQtGraphicsWindowQt.o osgpickhandler.o osgutil.o $(DEFAULTSCENESTATE) \
  $(SCENEERROR) scenesolver.o $(OPTIMIZE) \
  qttreewidgetitem.o qtcombobox.o qtcombobox_moc.o \
  qtlineedit.o qtlineedit_moc.o qtslider.o qtslider.o \
  qtslider_moc.o qtspinbox.o qtspinbox_moc.o treevalues.o \
  qttreewidget.o qttreewidget_moc.o qtmenu.o qtslot.o qtslot_moc.o \
  $(MAINWINDOWCONTROLLER) \
  $(SCENEOBJECTS) intersector.o $(SCENESTATEIO)
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

optional_test: optional_test.o osgutil.o
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

taggedvalueio_test: taggedvalueio_test.o $(TAGGEDVALUEIO)
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

osgutil_test: osgutil_test.o osgutil.o assertnearfloat.o
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

transform_test: transform_test.o maketransform.o
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

scenestate_test: scenestate_test.o scenestate.o
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

globaltransform_test: globaltransform_test.o scenestate.o maketransform.o \
  assertnearfloat.o randomtransform.o randompoint.o
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

scenestatetaggedvalue_test: scenestatetaggedvalue_test.o \
  $(SCENESTATETAGGEDVALUE) $(TAGGEDVALUEIO)
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

scenestateio_test: scenestateio_test.o scenestate.o \
  $(DEFAULTSCENESTATE) $(SCENESTATEIO)
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

scenesolver_test: scenesolver_test.o \
  scenesolver.o maketransform.o randomtransform.o randompoint.o \
  $(SCENEERROR) $(OPTIMIZE)
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

optimize_test: optimize_test.o $(OPTIMIZE)
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

treevalues_test: treevalues_test.o faketreewidget.o \
  $(DEFAULTSCENESTATE) treevalues.o maketransform.o checktree.o
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

sceneobjects_test: sceneobjects_test.o $(SCENEOBJECTS) fakescene.o
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

observedscene_test: observedscene_test.o $(OBSERVEDSCENE) faketreewidget.o \
  fakescene.o checktree.o
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

clean:
	rm -f *.o *.d guisolver *_moc.cpp

-include *.d
