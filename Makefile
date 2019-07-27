PACKAGES=openscenegraph QtGui QtOpenGL
MOC=moc-qt4
CXXFLAGS=-W -Wall -pedantic -std=c++17 -MD -MP `pkg-config --cflags $(PACKAGES)`

run_guisolver: guisolver
	./guisolver

%_moc.cpp: %.hpp
	$(MOC) $*.hpp -o $@

guisolver: main.o osgscenemanager.o osgscene.o qttimer.o qttimer_moc.o \
  osgQtGraphicsWindowQt.o osgpickhandler.o osgutil.o
	$(CXX) $(LDFLAGS) -o $@ $^ `pkg-config --libs $(PACKAGES)`

clean:
	rm -f *.o *.d guisolver *_moc.cpp

-include *.d
