/* -*-c++-*- OpenSceneGraph - Copyright (C) 2009 Wang Rui
 *
 * This library is open source and may be redistributed and/or modified under
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OpenSceneGraph Public License for more details.
*/

#ifndef OSGVIEWER_GRAPHICSWINDOWQT
#define OSGVIEWER_GRAPHICSWINDOWQT

#include <osgViewer/GraphicsWindow>
#include "osgQtExport.hpp"

#include <QMutex>
#include <QEvent>
#include <QQueue>
#include <QSet>
#undef GL_GLEXT_VERSION /*workaround conflict between osgViewer/GraphicsWindow and QGLWidget */
#include <QGLWidget>

class QInputEvent;

namespace osgViewer {
    class ViewerBase;
}

namespace osgQt
{

// forward declarations
class GraphicsWindowQt;


class OSGQT_EXPORT GLWidget : public QGLWidget
{
    typedef QGLWidget inherited;

  public:
    QSize sizeHint() const override;

    void setDefaultSize(QSize arg) { _default_size = arg; }

    virtual ~GLWidget();

    bool canSwapBuffers() const { return _canSwapBuffers; }

    void setGraphicsWindow( GraphicsWindowQt* gw ) { _gw = gw; }
    GraphicsWindowQt* getGraphicsWindow() { return _gw; }
    const GraphicsWindowQt* getGraphicsWindow() const { return _gw; }

    bool getForwardKeyEvents() const { return _forwardKeyEvents; }
    virtual void setForwardKeyEvents( bool f ) { _forwardKeyEvents = f; }

    void setKeyboardModifiers( QInputEvent* event );

    void keyPressEvent( QKeyEvent* event ) override;
    void keyReleaseEvent( QKeyEvent* event ) override;
    void mousePressEvent( QMouseEvent* event ) override;
    void mouseReleaseEvent( QMouseEvent* event ) override;
    void mouseDoubleClickEvent( QMouseEvent* event ) override;
    void mouseMoveEvent( QMouseEvent* event ) override;
    void wheelEvent( QWheelEvent* event ) override;

protected:

    int getNumDeferredEvents()
    {
        QMutexLocker lock(&_deferredEventQueueMutex);
        return _deferredEventQueue.count();
    }
    void enqueueDeferredEvent(QEvent::Type eventType, QEvent::Type removeEventType = QEvent::None)
    {
        QMutexLocker lock(&_deferredEventQueueMutex);

        if (removeEventType != QEvent::None)
        {
            if (_deferredEventQueue.removeOne(removeEventType))
                _eventCompressor.remove(eventType);
        }

        if (_eventCompressor.find(eventType) == _eventCompressor.end())
        {
            _deferredEventQueue.enqueue(eventType);
            _eventCompressor.insert(eventType);
        }
    }
    void processDeferredEvents();

    friend class GraphicsWindowQt;
    GraphicsWindowQt* _gw;

    QMutex _deferredEventQueueMutex;
    QQueue<QEvent::Type> _deferredEventQueue;
    QSet<QEvent::Type> _eventCompressor;

    bool _forwardKeyEvents;
    bool _canSwapBuffers;
    QSize _default_size;

    void resizeEvent( QResizeEvent* event ) override;
    void moveEvent( QMoveEvent* event ) override;
    void glDraw() override;
    bool event( QEvent* event ) override;

  private:
    GLWidget(
      QWidget* parent = NULL,
      const QGLWidget* shareWidget = NULL,
      Qt::WindowFlags f = 0,
      bool forwardKeyEvents = false
    );

    GLWidget(
      QGLContext* context,
      QWidget* parent = NULL,
      const QGLWidget* shareWidget = NULL,
      Qt::WindowFlags f = 0,
      bool forwardKeyEvents = false
    );

    GLWidget(
      const QGLFormat& format,
      QWidget* parent = NULL,
      const QGLWidget* shareWidget = NULL,
      Qt::WindowFlags f = 0,
      bool forwardKeyEvents = false
    );
};


class OSGQT_EXPORT GraphicsWindowQt : public osgViewer::GraphicsWindow
{
public:
    GraphicsWindowQt( osg::GraphicsContext::Traits* traits, QWidget* parent = NULL, const QGLWidget* shareWidget = NULL, Qt::WindowFlags f = 0 );
    GraphicsWindowQt( GLWidget* widget );
    virtual ~GraphicsWindowQt();

    inline GLWidget* getGLWidget() { return _widget; }
    inline const GLWidget* getGLWidget() const { return _widget; }

    /// deprecated
    inline GLWidget* getGraphWidget() { return _widget; }
    /// deprecated
    inline const GLWidget* getGraphWidget() const { return _widget; }

    struct WindowData : public osg::Referenced
    {
        WindowData( GLWidget* widget = NULL, QWidget* parent = NULL ): _widget(widget), _parent(parent) {}
        GLWidget* _widget;
        QWidget* _parent;
    };

    bool init( QWidget* parent, const QGLWidget* shareWidget, Qt::WindowFlags f );

    static QGLFormat traits2qglFormat( const osg::GraphicsContext::Traits* traits );
    static void qglFormat2traits( const QGLFormat& format, osg::GraphicsContext::Traits* traits );
    static osg::GraphicsContext::Traits* createTraits( const QGLWidget* widget );

    virtual bool setWindowRectangleImplementation( int x, int y, int width, int height );
    virtual void getWindowRectangle( int& x, int& y, int& width, int& height );
    virtual bool setWindowDecorationImplementation( bool windowDecoration );
    virtual bool getWindowDecoration() const;
    virtual void grabFocus();
    virtual void grabFocusIfPointerInWindow();
    virtual void raiseWindow();
    virtual void setWindowName( const std::string& name );
    virtual std::string getWindowName();
    virtual void useCursor( bool cursorOn );
    virtual void setCursor( MouseCursor cursor );

    virtual bool valid() const;
    virtual bool realizeImplementation();
    virtual bool isRealizedImplementation() const;
    virtual void closeImplementation();
    virtual bool makeCurrentImplementation();
    virtual bool releaseContextImplementation();
    virtual void swapBuffersImplementation();
    virtual void runOperations();

    virtual void requestWarpPointer( float x, float y );

protected:

    friend class GLWidget;
    GLWidget* _widget;
    bool _ownsWidget;
    QCursor _currentCursor;
    bool _realized;
};

}

#endif
