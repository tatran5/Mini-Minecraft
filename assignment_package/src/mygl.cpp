#include "mygl.h"
#include <la.h>

#include <iostream>
#include <QApplication>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QThreadPool>

MyGL::MyGL(QWidget *parent)
    : OpenGLContext(parent),
      mp_geomCube(new Cube(this)), mp_worldAxes(new WorldAxes(this)), mp_cursor(new Cursor(this)),
      mp_progLambert(new ShaderProgram(this)), mp_progFlat(new ShaderProgram(this)),
      mp_progOverlay(new ShaderProgram(this)), m_overlay(this, WATER), m_sky(this, WATER), mp_progSky(new ShaderProgram(this)),
      mp_progStatic(new ShaderProgram(this)), mp_progBlinnPhong(new ShaderProgram(this)),
      mp_progDepth(new ShaderProgram(this)), /*--T*/
      mp_camera(new Camera()), mp_terrain(new Terrain(this)), mp_player(new Player(mp_camera)),
      mp_dateTime(new QDateTime()), ignoreMouseMove(false), m_timeDrawn(0),

      m_fluidOverlay(this, WATER), mp_postProcess(new ShaderProgram(this)), mp_waterPostProcess(new ShaderProgram(this)), mp_secPostProcess(new ShaderProgram(this)),
      m_frameBuffer(-1), m_renderedTexture(-1), m_depthRenderBuffer(-1), m_postOverlay(this, WATER), m_renderMode(NORMAL)
{
    // Connect the timer to a function so that when the timer ticks the function is executed
    connect(&timer, SIGNAL(timeout()), this, SLOT(timerUpdate()));
    // Tell the timer to redraw 60 times per second
    timer.start(16);
    setFocusPolicy(Qt::ClickFocus);

    setMouseTracking(true); // MyGL will track the mouse's movements even if a mouse button is not pressed
    setCursor(Qt::BlankCursor); // Make the cursor invisible
}

MyGL::~MyGL()
{
    makeCurrent();
    glDeleteVertexArrays(1, &vao);
    mp_geomCube->destroy();

    delete mp_geomCube;
    delete mp_worldAxes;
    delete mp_cursor;

    delete mp_progLambert;
    delete mp_progFlat;
    delete mp_progStatic;
    delete mp_progOverlay;

    delete mp_camera;
    delete mp_terrain;
    delete mp_player;

    m_overlay.destroy();
    delete mp_progBlinnPhong;
    delete mp_progSky;
    delete mp_progDepth;

    m_fluidOverlay.destroy();
    delete mp_postProcess;
    delete mp_waterPostProcess;
    delete mp_dateTime;
    delete mp_secPostProcess;
}


void MyGL::MoveMouseToCenter()
{
    QCursor::setPos(this->mapToGlobal(QPoint(width() / 2, height() / 2)));
}

void MyGL::initializeGL()
{
    // Create an OpenGL context using Qt's QOpenGLFunctions_3_2_Core class
    // If you were programming in a non-Qt context you might use GLEW (GL Extension Wrangler)instead
    initializeOpenGLFunctions();
    // Print out some information about the current OpenGL context
    debugContextVersion();

    // Set a few settings/modes in OpenGL rendering
    glEnable(GL_DEPTH_TEST);
    //    glEnable(GL_LINE_SMOOTH);
    //    glEnable(GL_POLYGON_SMOOTH);
    // The two belows are for non-opaque shading; enable transparency

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    //    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

    // Set the size with which points should be rendered
    glPointSize(5);
    // Set the color with which the screen is filled at the start of each render call.
    glm::vec4 skyCol = glm::vec4(247.f, 241.f, 241.f, 255.f) / 255.f;

    glClearColor(skyCol.x, skyCol.y, skyCol.z, 1.f);

    printGLErrorLog();

    // Create a Vertex Attribute Object
    glGenVertexArrays(1, &vao);



    //Create the instance of Cube
    mp_geomCube->create();
    mp_worldAxes->create();
    mp_cursor->create();

    // Transparent overlay when underwater or lave
    m_overlay.create();

    // Create shaders-----------------------------------
    mp_progLambert->create(":/glsl/lambert.vert.glsl", ":/glsl/lambert.frag.glsl"); // Create and set up the diffuse shader
    mp_progFlat->create(":/glsl/flat.vert.glsl", ":/glsl/flat.frag.glsl"); // Create and set up the flat lighting shader
    mp_progStatic->create(":/glsl/static.vert.glsl", ":/glsl/static.frag.glsl");  // Create and set up the shading for the cursor
    mp_progBlinnPhong->create(":/glsl/blinnPhongUVAni.vert.glsl", ":/glsl/blinnPhongUVAni.frag.glsl"); // Create and setup the shading for blinnPhong & UV mapping objects
    mp_progOverlay->create(":/glsl/overlay.vert.glsl", ":/glsl/overlay.frag.glsl");
    mp_progSky->create(":/glsl/sky.vert.glsl", ":/glsl/sky.frag.glsl");
    mp_progDepth->create(":/glsl/depth.vert.glsl", ":/glsl/depth.frag.glsl");

    // -------------------------------------------------

    // Set a color with which to draw geometry since you won't have one
    // defined until you implement the Node classes.
    // This makes your geometry render green.
    mp_progLambert->setGeometryColor(skyCol);
    mp_progBlinnPhong->setGeometryColor(skyCol);

    mp_progBlinnPhong->createTexture();
    printGLErrorLog();
    //    mp_progBlinnPhong->createNorTexture();
    printGLErrorLog();
    // We have to have a VAO bound in OpenGL 3.2 Core. But if we're not
    // using multiple VAOs, we can just bind one once.
    //    vao.bind();
    glBindVertexArray(vao);
    printGLErrorLog();


    // mp_terrain->createInitScene(0, 64, 0, 64);

    mp_terrain->createScene(0, 64, 0, 64);
    mp_terrain->setWorldBounds(glm::vec4(0, 64,
                                         0, 64));

    mp_terrain->calcInitPos(0,0);
    printGLErrorLog();



    m_sky.createFar();
    printGLErrorLog();
    m_sky.setAlpha(1.f);
    printGLErrorLog();
    updateChunks();
    printGLErrorLog();

    // createRenderBuffer(); // Create vertex buffer object. Used for shadow mapping --T

    // Post shader stuff
    createFluidRenderBuffers();
    printGLErrorLog();
    createPostPostBuffers();
    printGLErrorLog();
    m_fluidOverlay.createPostProcess();
    printGLErrorLog();
    mp_postProcess->create(":/glsl/passthrough.vert.glsl", ":/glsl/noop.frag.glsl");
    printGLErrorLog();
    mp_secPostProcess->create(":/glsl/passthrough.vert.glsl", ":/glsl/paint.frag.glsl");
    printGLErrorLog();
    mp_waterPostProcess->create(":/glsl/passthrough.vert.glsl", ":/glsl/waterwarp.frag.glsl");
    printGLErrorLog();
    mp_waterPostProcess->createTexture();
}

void MyGL::createRenderBuffer() {
    // Initialize the frame buffers
    glGenFramebuffers(1, &m_depthMapFBO); //x
    glGenTextures(1, &m_depthMap); //x
    // Bind our texture so that all functions that deal with textures will interact with this one
    glBindTexture(GL_TEXTURE_2D, m_depthMap); //x
    // Give an empty image to OpenGL ( the last "0" )
    //    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
    //                 SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                 SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL); //x
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); //x
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); //x
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); //x
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER); //x
    // All coordinates outside the depth map's range have a depth of 1.0 which as a result
    // means these coordinates will never be in shadow. Otherwise weird block of shadows
    /////
    //    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    //    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    //With the generated depth texture we can attach it as the framebuffer's depth buffer:
    glBindFramebuffer(GL_FRAMEBUFFER, m_depthMapFBO); //x
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthMap, 0); //x
    glDrawBuffer(GL_NONE); //x
    glReadBuffer(GL_NONE); //x
    //    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    //    Adam below
    glBindFramebuffer(GL_FRAMEBUFFER, this->defaultFramebufferObject());  // x
}

void MyGL::resizeGL(int w, int h)
{
    //This code sets the concatenated view and perspective projection matrices used for
    //our scene's camera view.
    *mp_camera = Camera(w, h, mp_terrain->m_playerInitPos,
                        glm::vec3(64, mp_terrain->m_playerInitPos.y, 64),
                        glm::vec3(0,  1,  0));

    mp_player->pos = glm::vec4(mp_camera->eye.x, mp_camera->eye.y, mp_camera->eye.z, 1.f);

    glm::mat4 viewproj = mp_camera->getViewProj();

    // Upload the view-projection matrix to our shaders (i.e. onto the graphics card)

    mp_progLambert->setViewProjMatrix(viewproj);
    mp_progFlat->setViewProjMatrix(viewproj);
    mp_progStatic->setViewProjMatrix(viewproj);
    mp_progBlinnPhong->setViewProjMatrix(viewproj);


    mp_progSky->setViewProjMatrix(glm::inverse(mp_camera->getViewProj()));

    //this->glUniform2i(mp_progSky->unifDimensions, width() * this->devicePixelRatio(), height() * this->devicePixelRatio());


    mp_progLambert->setCamPosVector(mp_camera->eye);
    mp_progFlat->setCamPosVector(mp_camera->eye);
    mp_progBlinnPhong->setCamPosVector(mp_camera->eye);
    mp_progSky -> setDimensions(width() * this->devicePixelRatio(), height() * this->devicePixelRatio());

    // Post process shader stuff
    mp_postProcess->setDimensions(width() * this->devicePixelRatio(), height() * this->devicePixelRatio());
    mp_secPostProcess->setDimensions(width() * this->devicePixelRatio(), height() * this->devicePixelRatio());
    mp_waterPostProcess->setDimensions(width() * this->devicePixelRatio(), height() * this->devicePixelRatio());

    createFluidRenderBuffers();

    createPostPostBuffers();



    printGLErrorLog();
}


// MyGL's constructor links timerUpdate() to a timer that fires 60 times per second.
// We're treating MyGL as our game engine class, so we're going to use timerUpdate
void MyGL::timerUpdate()
{
    int64_t currentTime = mp_dateTime->currentMSecsSinceEpoch();
    int64_t timeElapsed = currentTime - lastUpdateTime;
    lastUpdateTime = currentTime;
    glm::vec4 oldVel = mp_player->updateVeloc(timeElapsed);
    glm::vec4 oldPos = mp_player->updatePosAndCam(timeElapsed, oldVel);
    mp_player->handleCollision(oldPos, mp_terrain);
    ignoreMouseMove = true;
    MoveMouseToCenter();
    ignoreMouseMove = false;


    mp_terrain->mutex.lock();


    if (!mp_terrain->chunkList.empty()) {
        for (Chunk* chunk : mp_terrain->chunkList)
        {
            if(!mp_terrain->chunkMap.contains(chunk->xzCoord)) {
                mp_terrain->chunkMap.insert(chunk->xzCoord, chunk);
                // Setting adjacencies
                mp_terrain->setAdjacencies(chunk);
                mp_terrain->toCreate.insert(chunk);

                mp_terrain->makeTreeAtChunk(chunk);
            }
        }
        mp_terrain->chunkList.clear();
        // mp_terrain->created -= mp_terrain->recreate;
    }



    updateChunks();
    mp_terrain->mutex.unlock();
    update();

}


// This function is called whenever update() is called.
// MyGL's constructor links update() to a timer that fires 60 times per second,
// so paintGL() called at a rate of 60 frames per second.
void MyGL::paintGL()
{
    //    // Render to our framebuffer rather than the viewportr
    glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer); //<------------ THIS IS NOT WORKING
    //    // Render on the whole framebuffer, complete from the lower left corner to the upper right
    glViewport(0,0,this->width() * this->devicePixelRatio(), this->height() * this->devicePixelRatio());
    // Clear the screen so that we only see newly drawn images

    //     if (mp_player->m_isUnderLiquid)
    //     {
    //         //    // Render to our framebuffer rather than the viewportr
    //             glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);
    //         //    // Render on the whole framebuffer, complete from the lower left corner to the upper right
    //             glViewport(0,0,this->width() * this->devicePixelRatio(), this->height() * this->devicePixelRatio());
    //     }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mp_progSky->setTime(m_timeDrawn);
    //mp_progSky->setCamPosVector(mp_camera->eye);
    mp_progSky->setViewProjMatrix(glm::inverse(mp_camera->getViewProj()));

    mp_progSky->drawNotInter(m_sky);


    mp_progFlat->setViewProjMatrix(mp_camera->getViewProj());
    mp_progLambert->setViewProjMatrix(mp_camera->getViewProj());
    mp_progStatic->setViewProjMatrix(mp_camera->getViewProj());
    mp_progBlinnPhong->setViewProjMatrix(mp_camera->getViewProj());

    mp_progBlinnPhong->setTime(m_timeDrawn);
    m_timeDrawn++;

    mp_progLambert->setCamPosVector(mp_camera->eye);
    mp_progFlat->setCamPosVector(mp_camera->eye);
    mp_progBlinnPhong->setCamPosVector(mp_camera->eye);

    GLDrawScene();

    glDisable(GL_DEPTH_TEST);
    mp_progFlat->setModelMatrix(glm::mat4());
    mp_progFlat->draw(*mp_worldAxes);
    mp_progStatic->setModelMatrix(glm::scale(glm::mat4(),glm::vec3((float) mp_camera -> height / (float) mp_camera -> width,
                                                                   1, 1.f)));


    m_overlay.blockType = mp_player->m_liquidType;

    m_overlay.setAlpha(0.3f);
    mp_postProcess->setTime(m_timeDrawn);
    performPostprocessRenderPass();



    mp_progOverlay->drawNotInter(m_overlay);

    mp_progStatic->drawNotInter(*mp_cursor);


    mp_postProcess->setTime(m_timeDrawn);

    glEnable(GL_DEPTH_TEST);
    performPostprocessRenderPass();

    printGLErrorLog();

}

void MyGL::updateChunks() {
    for(auto p = mp_terrain -> chunkMap.begin(); p != mp_terrain -> chunkMap.end(); p++) {

        if(mp_terrain->toCreate.contains(p.value())) {
            //want to create adjacents but not repeat anything
            p.value() -> destroy();
            p.value() -> create();

            if(p.value()->left != nullptr) {
                p.value()->left->destroy();
                p.value()->left->create();
            }

            if(p.value()->right != nullptr) {
                p.value()->right->destroy();
                p.value()->right->create();

            }

            if(p.value()->front != nullptr) {
                p.value()->front->destroy();
                p.value()->front->create();
            }

            if(p.value()->back != nullptr) {
                p.value()->back->destroy();
                p.value()->back->create();
            }
            mp_terrain->toCreate.remove(p.value());


        }

    }



    /*
    for(auto p = mp_terrain -> chunkMap.begin(); p != mp_terrain -> chunkMap.end(); p++) {

        if(p.value()->blockAt(4,0,0) == EMPTY) {
            std::cout << "BLANK" << "\n";
        }

        QSet<Chunk*> tempCreated;

        //want to create adjacents but not repeat anything
        if(!mp_terrain->created.contains(p.value())) {

            if(!tempCreated.contains(p.value())) {
                p.value() -> create();

            }
            if(p.value()->left != nullptr) {
                p.value()->left->destroy();
                p.value()->left->create();
                tempCreated.insert(p.value()->left);
            }

            if(p.second->back != nullptr) {
                p.second -> back -> destroy();
                p.second -> back -> create();

            if(p.value()->back != nullptr) {
                p.value()->back->destroy();
                p.value()->back->create();
                tempCreated.insert(p.value()->back);
            }
            mp_terrain->created.insert(p.value());


        }


    }
    */


}

void MyGL::GLDrawScene()
{

    GLDrawSceneOpaque();
    GLDrawSceneNonOpaque();
}

void MyGL::GLDrawSceneDepth() {
    for(auto p = mp_terrain -> chunkMap.begin(); p != mp_terrain -> chunkMap.end(); p++) {
        //translate the chunk to its world position
        mp_progDepth->setModelMatrix(glm::translate(glm::mat4(),
                                                    glm::vec3((int) (p.key() >> 32),
                                                              0,
                                                              (int) ((p.key()) & 0x00000000FFFFFFFF))));

        // draw the opaque chunks
        p.value()->bindOpaquePart();
        mp_progDepth->draw(*p.value());
    }
}

void MyGL::GLDrawSceneOpaque() {
    for(auto p = mp_terrain -> chunkMap.begin(); p != mp_terrain -> chunkMap.end(); p++) {
        //translate the chunk to its world position

        float xCoord = (int) (p.key() >> 32);
        float zCoord = (int) ((p.key()) & 0x00000000FFFFFFFF);

        if(glm::distance(glm::vec2(xCoord,zCoord), glm::vec2(mp_player -> pos.x, mp_player -> pos.z)) < 100) {

            mp_progBlinnPhong->setModelMatrix(glm::translate(glm::mat4(),
                                                             glm::vec3(xCoord,
                                                                       0,
                                                                       zCoord)));

            // draw the opaque chunks
            p.value() -> bindOpaquePart();
            mp_progBlinnPhong->draw(*p.value());
        }
    }

    //mp_terrain->chunkMap = nearMap;
}

void MyGL::GLDrawSceneNonOpaque() {
    for(auto p = mp_terrain -> chunkMap.begin(); p != mp_terrain -> chunkMap.end(); p++) {
        //translate the chunk to its world position

        float xCoord = (int) (p.key() >> 32);
        float zCoord = (int) ((p.key()) & 0x00000000FFFFFFFF);

        if(glm::distance(glm::vec2(xCoord,zCoord), glm::vec2(mp_player -> pos.x, mp_player -> pos.z)) < 100) {



            mp_progBlinnPhong->setModelMatrix(glm::translate(glm::mat4(),
                                                             glm::vec3(xCoord,
                                                                       0,
                                                                       zCoord)));

            p.value() -> bindNonOpaquePart();
            mp_progBlinnPhong->draw(*p.value());
        }

    }
}

void MyGL::keyPressEvent(QKeyEvent *e)
{
    if(e->isAutoRepeat() ) {
        e->ignore();
    } else {
        e->accept();
        float amount = 2.0f;
        if(e->modifiers() & Qt::ShiftModifier){
            amount = 10.0f;
        }
        // http://doc.qt.io/qt-5/qt.html#Key-enum
        // This could all be much more efficient if a switch
        // statement were used, but I really dislike their
        // syntax so I chose to be lazy and use a long
        // chain of if statements instead

        if (e->type() == QEvent::KeyPress) {
            if (e->key() == Qt::Key_Escape) {
                QApplication::quit();
            } else if (e->key() == Qt::Key_Right || e->key() == Qt::Key_Left ||
                       e->key() == Qt::Key_W || e->key() == Qt::Key_S ||
                       e->key() == Qt::Key_D || e->key() == Qt::Key_A ||
                       e->key() == Qt::Key_Q || e->key() == Qt::Key_E ||
                       e->key() == Qt::Key_F || e->key() == Qt::Key_Space) {
                mp_player->keyPress(e);
            }else if (e->key() == Qt::Key_Up) {
                mp_camera->RotateAboutRight(-amount);
            } else if (e->key() == Qt::Key_Down) {
                mp_camera->RotateAboutRight(amount);
            } else if (e->key() == Qt::Key_1) {
                mp_camera->fovy += amount;
            } else if (e->key() == Qt::Key_2) {
                mp_camera->fovy -= amount;
            } else if (e->key() == Qt::Key_R) {
                *mp_camera = Camera(this->width(), this->height());
            } else if (e->key() == Qt::Key_M) {
                if(m_renderMode == NORMAL) {
                    m_renderMode = PAINT;
                } else {
                    m_renderMode = NORMAL;

                }
            }
        }
        mp_camera->RecomputeAttributes();
    }
    expand();
}

void MyGL::keyReleaseEvent(QKeyEvent* p_event) {
    // KeyReleaseEvent is constantly called by the system even if the key is being held down
    // The if statement is to get rid of this situation
    if(p_event->isAutoRepeat() ) {
        p_event->ignore();
    } else {
        p_event->accept();
        mp_player->keyRelease(p_event);
        mp_camera->RecomputeAttributes();
    }
}

void MyGL::mouseMoveEvent(QMouseEvent *p_event) {
    if (!ignoreMouseMove) {
        mp_player->mouseMove(p_event, this->width() * 0.5 + this->pos().x(), this->height() * 0.5 + this->pos().y());
    }
}

// Mouse press handling removal and addition of blocks
void MyGL::mousePressEvent(QMouseEvent *p_event)
{
    if (p_event->button() == Qt::LeftButton)
    {
        removeBlock(6.f);
        updateChunks();

    }
    else if (p_event->button() == Qt::RightButton)
    {
        addBlock(GUM, 4.f);
        updateChunks();

    }
}


// Check if the player is near the current bounds of the world.
// If true then expand and redraw accordingly
void MyGL::expand()
{
    int offset = 15.f;
    int addition = 64.f;

    glm::vec4 playerPos = mp_player->pos;
    glm::vec4 currWorldBounds = mp_terrain->m_currBounds;
    float worldMinX = currWorldBounds.x;
    float worldMaxX = currWorldBounds.y;
    float worldMinZ = currWorldBounds.z;
    float worldMaxZ = currWorldBounds.w;

    // Close to top edge
    if (playerPos.z > worldMaxZ - offset && playerPos.z < worldMaxZ)
    {
        mp_terrain->createScene(worldMinX, worldMaxX, worldMaxZ, worldMaxZ + addition);
        mp_terrain->setWorldBounds(glm::vec4(worldMinX, worldMaxX,
                                             worldMinZ, worldMaxZ + addition));

    }
    // Close to bottom edge
    else if (playerPos.z > worldMinZ && playerPos.z < worldMinZ + offset)
    {
        mp_terrain->createScene(worldMinX, worldMaxX, worldMinZ - addition, worldMinZ);
        mp_terrain->setWorldBounds(glm::vec4(worldMinX, worldMaxX,
                                             worldMinZ - addition, worldMaxZ));


    }
    // Close to left edge
    else if (playerPos.x > worldMinX && playerPos.x < worldMinX + offset)
    {
        mp_terrain->createScene(worldMinX - addition, worldMinX, worldMinZ, worldMaxZ);
        mp_terrain->setWorldBounds(glm::vec4(worldMinX - addition, worldMaxX,
                                             worldMinZ, worldMaxZ));


    }
    // Close to right edge
    else if (playerPos.x > worldMaxX - offset && playerPos.x < worldMaxX)
    {
        mp_terrain->createScene(worldMaxX, worldMaxX + addition, worldMinZ, worldMaxZ);
        mp_terrain->setWorldBounds(glm::vec4(worldMinX, worldMaxX + addition,
                                             worldMinZ, worldMaxZ));


    }
}

// Switches the value of two flaots
void MyGL::flip(float &a, float &b)
{
    float temp = a;
    a = b;
    b = temp;
}

// Check if the ray is hitting a block that's not of type empty
// If it is, then remove the block
void MyGL::removeBlock(float reach)
{
    glm::vec4 origin = glm::vec4(mp_camera->eye.x, mp_camera->eye.y, mp_camera->eye.z, 1.f);
    glm::vec4 direction(glm::normalize(glm::vec3(mp_camera->look)), 0);

    for (float t = 1; t < reach; t++) {
        glm::vec4 pointOnRay = origin + t * direction;

        float xMin = floor(pointOnRay.x);
        float yMin = floor(pointOnRay.y);
        float zMin = floor(pointOnRay.z);

        glm::vec4 currWorldBounds = mp_terrain->m_currBounds;

        // check to make sure the block is within the bounds
        if (xMin < currWorldBounds.x || xMin > currWorldBounds.y
                || yMin < 0 || yMin > 256
                || zMin < currWorldBounds.z || zMin > currWorldBounds.w)
        {
            // If it's out of bounds then we don't do anything
            break;
        }

        // Remove the closest one and break out of loop, if not then
        // keep marching along the ray
        if (mp_terrain->getBlockAt(xMin, yMin, zMin) != EMPTY){
            mp_terrain->setBlockAt(xMin, yMin, zMin, EMPTY);
            std::cout << xMin << ", " << yMin << ", " << zMin << '\n';
            mp_terrain->created.remove(mp_terrain->getChunkAt(xMin, zMin));

            break;
        }
    }
}

void MyGL::addBlock(BlockType newBlockType, float reach)
{
    glm::vec4 origin = glm::vec4(mp_camera->eye.x, mp_camera->eye.y, mp_camera->eye.z, 1.f);
    glm::vec4 direction(glm::normalize(glm::vec3(mp_camera->look)), 0);

    float t_near = -INFINITY;
    ////    float t_far = INFINITY;

    for (float t = 1; t < reach; t++) {
        glm::vec4 pointOnRay = origin + t * direction;

        int xMin = floor(pointOnRay.x);
        int xMax = xMin + 1;
        int yMin = floor(pointOnRay.y);
        float yMax = yMin + 1;
        int zMin = floor(pointOnRay.z);
        int zMax = zMin + 1;

        glm::vec4 currWorldBounds = mp_terrain->m_currBounds;
        // check to make sure the block is within the bounds
        if (xMin < currWorldBounds.x || xMin > currWorldBounds.y
                || yMin < 0 || yMin > 256
                || zMin < currWorldBounds.z || zMin > currWorldBounds.w)
        {
            // If it's out of bounds then we don't do anything
            break;
        }

        if (mp_terrain->getBlockAt(xMin, yMin, zMin) != EMPTY){
            // Find where the ray intersects the planes defined by each face of the cube
            float sideToAddX = xMin;
            float sideToAddY = yMin;
            float sideToAddZ = zMin;
            float finalSideToAdd = 0;

            float t_0x = (xMin - origin.x) / direction.x;
            float t_1x = (xMax - origin.x) / direction.x;
            float t_0y = (yMin - origin.y) / direction.y;
            float t_1y = (yMax - origin.y) / direction.y;
            float t_0z = (zMin - origin.z) / direction.z;
            float t_1z = (zMax - origin.z) / direction.z;

            if (t_0x > t_1x)
            {
                flip(t_0x, t_1x);
                sideToAddX = xMax;
            }
            if (t_0y > t_1y)
            {
                flip(t_0y, t_1y);
                sideToAddY = yMax;
            }
            if (t_0z > t_1z)
            {
                flip(t_0z, t_1z);
                sideToAddZ = zMax;
            }

            float maxTNear = std::max({t_0x, t_0y, t_0z});
            float minTFar = std::min({t_1x, t_1y, t_1z});

            if (maxTNear > minTFar)
            {
                break;
            }

            glm::vec3 newBlockOffset(0.f);

            if (t_0x > t_near)
            {
                t_near = t_0x;
                finalSideToAdd = sideToAddX;
                if (sideToAddX == xMin)
                {
                    newBlockOffset = glm::vec3(-1.f, 0.f, 0.f);
                }
                else
                {
                    newBlockOffset = glm::vec3(1.f, 0.f, 0.f);
                }
            }

            if (t_0y > t_near)
            {
                t_near = t_0y;
                finalSideToAdd = sideToAddY;
                if (sideToAddY == yMin)
                {
                    newBlockOffset = glm::vec3(0.f, -1.f, 0.f);
                }
                else
                {
                    newBlockOffset = glm::vec3(0.f, 1.f, 0.f);
                }
            }

            if (t_0z > t_near)
            {
                t_near = t_0z;
                finalSideToAdd = sideToAddZ;
                if (sideToAddZ == zMin)
                {
                    newBlockOffset = glm::vec3(0.f, 0.f, -1.f);
                }
                else
                {
                    newBlockOffset = glm::vec3(0.f, 0.f, 1.f);
                }
            }

            glm::vec3 newBlockPos(glm::vec3(xMin, yMin, zMin) + newBlockOffset);
            mp_terrain->setBlockAt(newBlockPos.x, newBlockPos.y, newBlockPos.z, newBlockType);
            mp_terrain->created.remove(mp_terrain->getChunkAt(newBlockPos.x, newBlockPos.z));
            break;
        }
    }
}


void MyGL::createFluidRenderBuffers()
{
    // Initialize the frame buffers and render textures
    glGenFramebuffers(1, &m_frameBuffer);
    glGenTextures(1, &m_renderedTexture);
    glGenRenderbuffers(1, &m_depthRenderBuffer);

    glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);
    // Bind our texture so that all functions that deal with textures will interact with this one
    glBindTexture(GL_TEXTURE_2D, m_renderedTexture);
    // Give an empty image to OpenGL ( the last "0" )
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, this->width() * this->devicePixelRatio(), this->height() * this->devicePixelRatio(), 0, GL_RGB, GL_UNSIGNED_BYTE, (void*)0);

    // Set the render settings for the texture we've just created.
    // Essentially zero filtering on the "texture" so it appears exactly as rendered
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    // Clamp the colors at the edge of our texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Initialize our depth buffer
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthRenderBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, this->width() * this->devicePixelRatio(), this->height() * this->devicePixelRatio());
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthRenderBuffer);

    // Set m_renderedTexture as the color output of our frame buffer
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_renderedTexture, 0);

    // Sets the color output of the fragment shader to be stored in GL_COLOR_ATTACHMENT0, which we previously set to m_renderedTextures[i]
    GLenum drawBuffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, drawBuffers); // "1" is the size of drawBuffers

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "Frame buffer did not initialize correctly..." << std::endl;
        printGLErrorLog();
    }
}


void MyGL::createPostPostBuffers()
{
    glGenTextures(1, &m_renderedTexture2);
    glBindTexture(GL_TEXTURE_2D, m_renderedTexture2);
    printGLErrorLog();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, this->width() * this->devicePixelRatio(), this->height() * this->devicePixelRatio(), 0, GL_RGB, GL_UNSIGNED_BYTE, (void*)0);
    printGLErrorLog();

    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &m_frameBuffer2);
    glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer2);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_renderedTexture2, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    printGLErrorLog();

    //    GLenum drawBuffers[1] = {GL_COLOR_ATTACHMENT0};
    //    printGLErrorLog();
    //    glDrawBuffers(1, drawBuffers); // "1" is the size of drawBuffers
    //    printGLErrorLog();
    //     mp_secPostProcess->createTexture();

}

void MyGL:: performDepthPass() {

    // Tell OpenGL to render to the viewport's frame buffer
    glBindFramebuffer(GL_FRAMEBUFFER, m_depthMapFBO);

    // Render on the whole framebuffer, complete from the lower left corner to the upper right
    glViewport(0,0,this->width() * this->devicePixelRatio(), this->height() * this->devicePixelRatio());
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Bind our texture in Texture Unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_depthMap);

    mp_progBlinnPhong->draw(m_fluidOverlay);

}


void MyGL::performPostprocessRenderPass()
{

    // 1. first render to depth map -----------------------------------------------------
    //configure shader and matrices, send it into the VBO
    // x
    glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f,
                                           mp_camera->near_clip,
                                           mp_camera->far_clip);
    // x
    glm::mat4 lightView = glm::lookAt(glm::vec3(-10.0f, 20.0f, -10.0f) /*light source position*/,
                                      glm::vec3(100.0f, -20.0f, 10.0f) /*a point which light source directs to*/,
                                      glm::vec3(0.0f, 1.0f, 0.0f)); /*the up vector of world*/
    // x
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;
    // x
    mp_progDepth->setLightSpaceMatrix(lightSpaceMatrix);



    // Render the frame buffer as a texture on a screen-size quad

    // Tell OpenGL to render to the viewport's frame buffer
    glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer2);

    // Render on the whole framebuffer, complete from the lower left corner to the upper right

    glViewport(0,0,this->width() * this->devicePixelRatio(), this->height() * this->devicePixelRatio());

    //glViewport(0,0,this->width() * this->devicePixelRatio(), this->height() * this->devicePixelRatio());
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Bind our texture in Texture Unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_renderedTexture);
    //mp_postProcess->drawOverlay(m_fluidOverlay, 0);
    if(mp_player->m_isUnderLiquid) {
        mp_waterPostProcess->drawOverlay(m_fluidOverlay, 0);

        //draw m_rendered texture, from framebuffer 1, to framebuffer2
    } else {
        mp_postProcess->drawOverlay(m_fluidOverlay, 0);

    }

    glBindFramebuffer(GL_FRAMEBUFFER, this->defaultFramebufferObject());
    // Render on the whole framebuffer, complete from the lower left corner to the upper right
    glViewport(0,0,this->width() * this->devicePixelRatio(), this->height() * this->devicePixelRatio());
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Bind our texture in Texture Unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_renderedTexture2);

    if(m_renderMode == PAINT) {

        mp_secPostProcess->drawOverlay(m_fluidOverlay, 0);
    } else {
        mp_postProcess->drawOverlay(m_fluidOverlay, 0);

    }

}
