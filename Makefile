PACKAGES=openscenegraph QtGui QtOpenGL eigen3
MOC=moc-qt4
OPTIMIZATION=-g
#OPTIMIZATION=-O3 -DNDEBUG

CXXFLAGS=-W -Wall -Wundef -pedantic -std=c++14 -MD -MP $(OPTIMIZATION) \
  `pkg-config --cflags $(PACKAGES)`

all:
	$(MAKE) run_unit_tests
	$(MAKE) build_manual_tests
	$(MAKE) run_guisolver

run_unit_tests: \
  optional_test.pass \
  readobj_test.pass \
  solveflags_test.pass \
  taggedvalueio_test.pass \
  osgutil_test.pass \
  transform_test.pass \
  scenestate_test.pass \
  expressionparser_test.pass \
  evaluateexpression_test.pass \
  globaltransform_test.pass \
  scenestatetaggedvalue_test.pass \
  scenestateio_test.pass \
  scenesolver_test.pass \
  optimize_test.pass \
  treevalues_test.pass \
  sceneobjects_test.pass \
  observedscene_test.pass

build_manual_tests: \
  qttreewidget_manualtest \
  osgscene_manualtest

run_guisolver: guisolver
	./guisolver

%_moc.cpp: %.hpp
	$(MOC) $*.hpp -o $@

%.pass: %
	./$*
	touch $@

RANDOMPOINT=randompoint.o randomvec3.o
SCENESTATE=scenestate.o nextunusedname.o
DEFAULTSCENESTATE=defaultscenestate.o $(SCENESTATE) maketransform.o
TAGGEDVALUEIO=taggedvalueio.o printindent.o streamparser.o stringutil.o
SCENESTATETAGGEDVALUE=scenestatetaggedvalue.o $(SCENESTATE) taggedvalue.o
SCENESTATEIO=scenestateio.o $(SCENESTATETAGGEDVALUE) $(TAGGEDVALUEIO)

SCENEERROR=sceneerror.o $(SCENESTATE)
SCENEOBJECTS=sceneobjects.o maketransform.o settransform.o $(SCENESTATE)
OPTIMIZE=optimize.o

OBSERVEDSCENE=observedscene.o \
  treevalues.o scenestatetransform.o \
  $(EVALUATEEXPRESSION) $(SCENESTATETAGGEDVALUE) $(SCENEOBJECTS)

EVALUATEEXPRESSION=evaluateexpression.o expressionparser.o parsedouble.o

MAINWINDOWCONTROLLER=mainwindowcontroller.o \
  $(EVALUATEEXPRESSION) $(OBSERVEDSCENE)

QTSPINBOX=qtspinbox.o qtspinbox_moc.o parsedouble.o
RANDOMTRANSFORM=randomtransform.o randomcoordinateaxes.o

QTTREEWIDGET=\
  qttreewidget.o qttreewidget_moc.o \
  qtcombobox.o qtcombobox_moc.o \
  qtlineedit.o qtlineedit_moc.o \
  qtslider.o qtslider_moc.o \
  qtslot.o qtslot_moc.o \
  qtcheckbox.o qtcheckbox_moc.o \
  qttreewidgetitem.o \
  qtmenu.o \
  $(QTSPINBOX)

optional_test: optional_test.o osgutil.o
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

readobj_test: readobj_test.o readobj.o
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

solveflags_test: solveflags_test.o
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

taggedvalueio_test: taggedvalueio_test.o $(TAGGEDVALUEIO)
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

osgutil_test: osgutil_test.o osgutil.o assertnearfloat.o
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

transform_test: transform_test.o maketransform.o
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

scenestate_test: scenestate_test.o $(SCENESTATE)
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

expressionparser_test: expressionparser_test.o expressionparser.o
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

evaluateexpression_test: evaluateexpression_test.o $(EVALUATEEXPRESSION)
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

globaltransform_test: globaltransform_test.o $(SCENESTATE) maketransform.o \
  assertnearfloat.o $(RANDOMTRANSFORM) $(RANDOMPOINT)
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

scenestatetaggedvalue_test: scenestatetaggedvalue_test.o \
  $(SCENESTATETAGGEDVALUE) $(TAGGEDVALUEIO)
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

scenestateio_test: scenestateio_test.o $(SCENESTATE) \
  $(DEFAULTSCENESTATE) $(SCENESTATEIO)
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

scenesolver_test: scenesolver_test.o \
  scenesolver.o maketransform.o $(RANDOMTRANSFORM) $(RANDOMPOINT) \
  assertnearfloat.o randomtransform3.o randompoint3.o transform3util.o \
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
  fakescene.o checktree.o $(SCENESTATEIO) assertnearfloat.o
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

guisolver: main.o qtmainwindow.o osgscene.o qttimer.o qttimer_moc.o \
  osgQtGraphicsWindowQt.o osgpickhandler.o osgutil.o $(DEFAULTSCENESTATE) \
  $(SCENEERROR) scenesolver.o $(OPTIMIZE) \
  $(QTSPINBOX) treevalues.o \
  $(QTTREEWIDGET) \
  $(MAINWINDOWCONTROLLER) \
  $(SCENEOBJECTS) intersector.o $(SCENESTATEIO)
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

qttreewidget_manualtest: qttreewidget_manualtest.o $(QTTREEWIDGET)
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

osgscene_manualtest: osgscene_manualtest.o osgscene.o osgQtGraphicsWindowQt.o \
  osgpickhandler.o osgutil.o qttimer.o qttimer_moc.o intersector.o
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

clean:
	rm -f *.o *.d guisolver *_moc.cpp

-include *.d
