#ifndef MYGL_H
#define MYGL_H

#include <openglcontext.h>
#include <utils.h>
#include <shaderprogram.h>
#include <scene/cube.h>
#include <scene/worldaxes.h>
#include "camera.h"
#include <scene/terrain.h>

#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>

#include <QDateTime>
#include "player.h"
#include <scene/cursor.h>
#include <qstack.h>
#include <iostream>
#include <scene/quad.h>

#include <scene/worker.h>

#include <QThread>

enum RenderMode : unsigned char
{
    NORMAL, PAINT
};

class MyGL : public OpenGLContext
{
    Q_OBJECT
private:


    Cube* mp_geomCube;// The instance of a unit cube we can use to render any cube. Should NOT be used in final version of your project.
    WorldAxes* mp_worldAxes; // A wireframe representation of the world axes. It is hard-coded to sit centered at (32, 128, 32).

    Cursor* mp_cursor;
    ShaderProgram* mp_progLambert;// A shader program that uses lambertian reflection
    ShaderProgram* mp_progFlat;// A shader program that uses "flat" reflection (no shadowing at all)
    ShaderProgram* mp_progStatic;// A shader program that uses static vertex data (doesn't change w/ perspective)
    ShaderProgram* mp_progBlinnPhong; // A shader program that applies blinn phong shading, uv mapping and animation to objects
    ShaderProgram* mp_progSky; // A shader program that applies blinn phong shading, uv mapping and animation to objects
    ShaderProgram* mp_progOverlay;
    //--T
    ShaderProgram* mp_progDepth; //A shader that only considers z coord of vertices in order to create depth map. Render from light's ortho view

    const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024; //resolution of shadow map

    GLuint m_depthMap;
    GLuint m_depthMapFBO; //depth map frame buffer, used for shadow mapping
    //--ET
    GLuint vao; // A handle for our vertex array object. This will store the VBOs created in our geometry classes.
    // Don't worry too much about this. Just know it is necessary in order to render geometry.

    Camera* mp_camera;
    Terrain* mp_terrain;

    /// Timers
    QTimer timer; /// Fires approx. 60 times per second. Timer linked to timerUpdate()
    QDateTime* mp_dateTime;    /// Timer linked to timerUpdate()
    int m_timeDrawn; //numner of time that paintGL is called. helps in animating UV

    Player* mp_player;

    int64_t lastUpdateTime;
    bool ignoreMouseMove;

    void MoveMouseToCenter(); // Forces the mouse position to the screen's center. You should call this
    // from within a mouse move event after reading the mouse movement so that
    // your mouse stays within the screen bounds and is always read.

    void flip(float &a, float &b);


    RenderMode m_renderMode;
    // Transparent overlay
    Quad m_overlay;
    Quad m_sky;

  //  Quad m_postQuad;

    //  RiverWorker* mp_riverWorker;

    // Post Process overlay
    Quad m_fluidOverlay;
    Quad m_postOverlay;
    ShaderProgram* mp_postProcess;

    ShaderProgram* mp_secPostProcess;
    ShaderProgram* mp_waterPostProcess;


    // A collection of handles to the five frame buffers we've given
    // ourselves to perform render passes. The 0th frame buffer is always
    // written to by the render pass that uses the currently bound surface shader.
    GLuint m_frameBuffer;
    GLuint m_frameBuffer2;


    // A collection of handles to the textures used by the frame buffers.
    // m_frameBuffers[i] writes to m_renderedTextures[i].
    GLuint m_renderedTexture;
    GLuint m_renderedTexture2;

    // A collection of handles to the depth buffers used by our frame buffers.
    // m_frameBuffers[i] writes to m_depthRenderBuffers[i].
    GLuint m_depthRenderBuffer;




public:
    explicit MyGL(QWidget *parent = 0);
    ~MyGL();

    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();

    void GLDrawScene();
    void GLDrawSceneOpaque();
    void GLDrawSceneNonOpaque();
    void GLDrawSceneDepth(); // Only draw opaque blocks and only consider vertex position //--T

    void expand(); // Check if player is near the bounds of the current world

    // Cast a ray from the origin (eye/player position) in the direction of length reach.
    // This is for determining which cube it's intersecting
    void addBlock(BlockType newBlockType, float reach);
    void removeBlock(float reach);

    void updateChunks();
    void performPostprocessRenderPass();

    //--T
    void createRenderBuffer(); //for shadow mapping
    //--ET


    //Post process overlay
    // Sets up the arrays of frame buffers
    // used to store render passes. Invoked
    // once in initializeGL().
    void createFluidRenderBuffers();

    void createPostPostBuffers();
    // A helper function that iterates through
    // each of the render passes required by the
    // currently bound post-process shader and
    // invokes them.
    //void performPostprocessRenderPass();

    void performSecondPostprocessRenderPass();
    void performDepthPass();


protected:
    void keyPressEvent(QKeyEvent *p_event);
    void keyReleaseEvent(QKeyEvent *p_event);
    void mouseMoveEvent(QMouseEvent *p_event);
    void mousePressEvent(QMouseEvent *p_event); // Mouse press handling removal of blocks

private slots:
    /// Slot that gets called ~60 times per second
    void timerUpdate();
};

#endif // MYGL_H
