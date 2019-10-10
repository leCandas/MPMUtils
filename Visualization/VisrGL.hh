/// \file VisrGL.hh OpenGL visualization window driver
// Michael P. Mendenhall, LLNL 2019

#ifndef VISRGL_HH
#define VISRGL_HH

#include "Visr.hh"

#ifdef WITH_OPENGL
#include <GL/freeglut.h>
#include <deque>
#include <pthread.h>

/// OpenGL window visualization driver
class GLVisDriver: public VisDriver {
public:
    /// Destructor
    ~GLVisDriver() { endGlutLoop(); }

    /// start interactive drawing loop thread
    void doGlutLoop();
    /// stop interactive drawing loop thread
    void endGlutLoop();

    /// initialize visualization window
    void initWindow(const std::string& title = "OpenGL Viewer Window");

    /// pause for user interaction
    void pause() override;

    //-/////////////////////////////////
    // actions called in GLUT event loop

    /// Flush queue and redraw display if queue unlocked
    void tryFlush();
    /// Redraw display
    void updateViewWindow();
    void redrawDisplay();
    void reshapeWindow(int width, int height);
    void keypress(unsigned char key, int x, int y);
    void specialKeypress(int key, int x, int y);
    void startMouseTracking(int button, int state, int x, int y);
    void mouseTrackingAction(int x, int y);

    //-///////////////////
    // viewing window info

    float win_c[3];     ///< center of window view
    float ar = 1.0;     ///< (x range)/(y range) window aspect ratio
    float viewrange;    ///< half-height (y) range
    float win_lo[3];    ///< Window lower range
    float win_hi[3];    ///< Window upper range
    int winwidth, winheight;    ///< pixel dimensions

protected:

    /// add to backend commands execution queue
    void pushCommand(const VisCmd& c) override;

    /// start a group of related drawing commands
    void _startRecording(const vector<float>&) override;
    /// end a group of related drawing commands
    void _stopRecording(const vector<float>&) override;
    /// clear output --- automatic on startRecording(true)
    void _clearWindow(const vector<float>&) override;
    /// set color for subsequent draws
    void _setColor(const vector<float>&) override;
    /// draw series of lines between vertices
    void _lines(const vector<float>&) override;
    /// draw ball at location
    void _ball(const vector<float>&) override;
    /// OpenGL teapot
    void _teapot(const vector<float>&) override;

    /// reset rotations/scale to default
    void resetViewTransformation();
    /// get current transformation matrix
    void getMatrix();

    bool pause_display = false; ///< flag to maintain display pause
    bool updated = true;        ///< flag to refresh updated drawing
    int clickx0, clicky0;       ///< click (start) location
    int modifier;               ///< modifier key

    std::deque<VisCmd> commands;    ///< to-be-processed commands
    pthread_mutex_t commandLock;    ///< commands queue lock
    std::vector<GLuint> displaySegs;
};

#else

/// Null visualization driver without OpenGL
class GLVisDriver: public VisDriver {
public:
    /// start interactive drawing loop thread
    void doGlutLoop() { }
    /// stop interactive drawing loop thread
    void endGlutLoop() { }
}

#endif
#endif