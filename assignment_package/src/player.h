#ifndef PLAYER_H
#define PLAYER_H

#include <la.h>
#include <QKeyEvent>

#include <scene/terrain.h>
#include "camera.h"

#include <QSound>

/// Comment with "I" means this is straight from the instruction of CIS 460
class Player
{
public:
    //----------------------------------------------------------------------------------------
    // CONSTRUCTORS
    //----------------------------------------------------------------------------------------
    Player(Camera* p_cam);

    //----------------------------------------------------------------------------------------
    // FIELDS
    //----------------------------------------------------------------------------------------
    //to start running, the player must hold the key for more than normalInterval milliseconds
    const int64_t normalInterval = 5000;
    const float maxVel = 5.0;
    const float normalVel = 4.0; //for WASD
    const float flyModeVel = 20.0;
    const float accVal = 2.0; //for WASD
    const float gravityAcceleration = -5.0; //mainly for jump
    const float jumpVel = 4.0;

    //x: right/left
    //y: up/down
    //z: forward/backward
    Camera* p_cam; //a pointer to a camera. Default nullptr
    glm::vec4 pos; //position. Default (0, 0, 0, 0)
    glm::vec4 vel; //velocity. Default (0, 0, 0, 0)
    glm::vec4 accVec; // acceleration vector. Default (0, 0, 0, 0);

    // I:
    // A set of variables to track the relevant inputs from the mouse and keyboard.
    // The Player should know whether or not the W, A, S, D, and Spacebar keys are held down.
    // The Player should also track the the state of its left and right mouse buttons
    // as well as change in the cursor's X and Y coordinates

    // Default: All false and 0
    bool wDown;
    bool aDown;
    bool sDown;
    bool dDown;
    bool spaceDown;

    bool leftDown;
    bool rightDown;

    glm::vec2 cursorOriginal;
    glm::vec2 cursorNew;
    glm::vec2 cursorChange;

    // I:
    // You should enable the character to fly by pressing the F key on the keyboard.
    // This makes the character no longer subject to gravity,
    // and be able to move upwards by pressing E and downwards by pressing Q.
    // Furthermore, the character should no longer be subject to terrain collisions.

    // Default false
    bool fDown; //fly
    bool eDown; //move upwards
    bool qDown; //move downwards

    int64_t intervalSincePress;
    float gravityOnPlayer; // = 0 if on ground or flying; = gravityAcceleration otherwise

    //----------------------------------------------------------------------------------------
    // FUNCTIONS
    //----------------------------------------------------------------------------------------
    // I:
    // A function that takes in a QKeyEvent from MyGL's key press and key release event functions,
    // updates the relevant member variables based on the event.
    void keyPress(QKeyEvent* p_event);
    void keyRelease(QKeyEvent *p_event);

    // I:
    // A function that takes in a QMouseEvent from MyGL's mouse move, mouse press, and mouse release
    // event functions, and updates the relevant member variables based on the event.
    void mouseMove(QMouseEvent* p_event, float x, float y);
    void mousePress(QMouseEvent* p_event);
    void mouseRelease(QMouseEvent* p_event);

    //updates velocity; returns the old velocity
    glm::vec4 updateVeloc(int64_t timeChange);
    //updates position and camera; returns old position
    glm::vec4 updatePosAndCam(int64_t timeChange, glm::vec4 oldVel);

    void handleCollision(glm::vec4 oldPos, Terrain* p_terrain);
    /// helper function for handle collision

    bool hasMoveKeyDown();
    void cursorCamHandle();

    // Handing under liquid movement
    bool m_isUnderLiquid;
    BlockType m_liquidType;

};

#endif // PLAYER_H
