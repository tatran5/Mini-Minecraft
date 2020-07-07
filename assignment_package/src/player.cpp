#include "player.h"
#include <iostream>
#include <string>
#include <QDebug>

Player::Player(Camera *p_cam) :
    p_cam(p_cam),
    pos(glm::vec4(p_cam->eye, 1.f)),
    vel(glm::vec4(0, 0, 0, 0)),
    accVec(accVal, 0, accVal, 0),
    wDown(false), aDown(false), sDown(false), dDown(false), spaceDown(false),
    leftDown(false), rightDown(false),
    cursorOriginal(glm::vec2(0, 0)),  cursorNew(glm::vec2(0, 0)), cursorChange(glm::vec2(0, 0)),
    fDown(true), eDown(false), qDown(false),
    intervalSincePress(0), gravityOnPlayer(0),
    m_isUnderLiquid(false), m_liquidType(GRASS)
{}

/// A function that takes in a QKeyEvent from MyGL's key press and key release event functions,
/// updates the relevant member variables based on the event.
void Player::keyPress(QKeyEvent* p_event) {
    if (p_event->key() == Qt::Key_W && gravityOnPlayer != gravityAcceleration) {
        intervalSincePress = 0;
        wDown = true;
//        QSound::play("/Users/dzungnguyen/Desktop/final-project-mini-minecraft-the-adam-s-famally/assignment_package/sound/foots.wav");
        QSound::play(":/sound/foots.wav");

    }
    else if (p_event->key() == Qt::Key_A && gravityOnPlayer != gravityAcceleration) {
        intervalSincePress = 0;
        aDown = true;
        QSound::play(":/sound/foots.wav");

    }
    else if (p_event->key() == Qt::Key_S && gravityOnPlayer != gravityAcceleration) {
        intervalSincePress = 0;
        sDown = true;
        QSound::play(":/sound/foots.wav");

    }
    else if (p_event->key() == Qt::Key_D && gravityOnPlayer != gravityAcceleration) {
        intervalSincePress = 0;
        dDown = true;
        QSound::play(":/sound/foots.wav");
    }

    /// more "special handling" for fly & related keys: use F to turn on/off fly mode
    else if (p_event->key() == Qt::Key_F) {
        if (fDown) {
            gravityOnPlayer = gravityAcceleration;
            fDown = false;
        }
        else {
            fDown = true;
            gravityOnPlayer = 0;
            vel.y = 0;
        }
    }
    /// the two below only works if fly mode is activated
    else if (p_event->key() == Qt::Key_E && fDown) { eDown = true;}
    else if (p_event->key() == Qt::Key_Q && fDown) { qDown = true;}

    if (p_event->key() == Qt::Key_Space && !fDown && gravityOnPlayer != gravityAcceleration) { ///jump
        vel.y = jumpVel;
        gravityOnPlayer = gravityAcceleration;
        spaceDown = true;
        QSound::play(":/sound/mario.wav");

    }

    ///restart the timer for the movement
    if (hasMoveKeyDown()) {
        intervalSincePress = 0;
    }
}

void Player::keyRelease(QKeyEvent* p_event) {
    intervalSincePress = 0;
    vel.x = 0;
    vel.z = 0;
    if (p_event->key() == Qt::Key_W) { wDown = false; }
    else if (p_event->key() == Qt::Key_A) { aDown = false; }
    else if (p_event->key() == Qt::Key_S) { sDown = false; }
    else if (p_event->key() == Qt::Key_D) { dDown = false; }
    else if (p_event->key() == Qt::Key_Space) { spaceDown = false; }
    else if (p_event->key() == Qt::Key_Left) { leftDown = false; }
    else if (p_event->key() == Qt::Key_Right) { rightDown = false; }
    /// more "special handling" for fly-related keys:
    /// releasing f key will have no effect
    /// the two below only works if fly mode is activated
    else if (p_event->key() == Qt::Key_E) { eDown = false;}
    else if (p_event->key() == Qt::Key_Q) { qDown = false;}
}

/// A function that takes in a QMouseEvent from MyGL's mouse WWmove, mouse press, and mouse release
/// event functions, and updates the relevant member variables based on the event.
void Player::mouseMove(QMouseEvent* p_event, float centerX, float centerY) {
    cursorNew.x = p_event->x();
    cursorNew.y = p_event->y();
    cursorChange = cursorNew - glm::vec2(centerX, centerY);
    if (cursorChange != glm::vec2(0, 0))
    {
        cursorOriginal = cursorNew;
        p_cam->RotateAboutRight((-1.f) * cursorChange.y * 0.01);
        p_cam->RotateAboutUp((-1.f) * cursorChange.x * 0.01);
        cursorChange = glm::vec2(0, 0);
        p_cam->RecomputeAttributes();
    }
}

void Player::mousePress(QMouseEvent* p_event) {
    cursorOriginal.x = p_event->x();
    cursorOriginal.y = p_event->y();
}

void Player::mouseRelease(QMouseEvent* p_event) {
    cursorNew.x = p_event->x();
    cursorNew.y = p_event->y();
    cursorChange = cursorNew - cursorOriginal;
    if (cursorChange != glm::vec2(0, 0)) {
    }
}

bool Player::hasMoveKeyDown() {
    return (wDown || aDown || sDown || dDown || (fDown && (eDown || qDown)));
}

void Player::cursorCamHandle() {
    if (cursorChange != glm::vec2(0, 0))
    {
        cursorOriginal = cursorNew;
        p_cam->RotateAboutRight((-1.f) * cursorChange.y * 0.01);
        p_cam->RotateAboutUp((-1.f) * cursorChange.x * 0.01);
        cursorChange = glm::vec2(0, 0);
        p_cam->RecomputeAttributes();
    }
}

glm::vec4 Player::updateVeloc(int64_t timeChange) {
    glm::vec4 oldVel = this->vel;
    float timeChangeSec = (float) timeChange / 1000.f;
    float curYVel = vel.y; /// falling down velocity
    intervalSincePress += timeChange;
    /// handling velocity in x and z direction
    if (hasMoveKeyDown()) {

        float numPossibleKeys = 0.f; /// the total number that is possible is 2
        glm::vec4 vel1, vel2;
        if ((qDown || eDown) && fDown) { /// there is no acceleration for q, e
            if (qDown) {vel.y = -flyModeVel;}
            else {vel.y = flyModeVel;}
        } else if (intervalSincePress < normalInterval) { /// no acceleration
            // Move forward
            if (wDown) {
                if(!fDown && !m_isUnderLiquid) {
                    vel1 = (-1.f) * normalVel * glm::vec4(p_cam->look, 0.f);
                } else if (fDown) {
                    vel1 = (-1.f) * flyModeVel * glm::vec4(p_cam->look, 0.f);
                } else if (!fDown && m_isUnderLiquid) {
                    vel1 = (-1.f) * 2.f/3.f * normalVel * glm::vec4(p_cam->look, 0.f);
                }
                numPossibleKeys++;
            // move backward
            } else if (sDown) {
                if(!fDown && !m_isUnderLiquid) {
                    vel1 = normalVel * glm::vec4(p_cam->look, 0.f);
                } else if (fDown){
                    vel1 = flyModeVel * glm::vec4(p_cam->look, 0.f);
                } else if (!fDown && m_isUnderLiquid) {
                    vel1 = normalVel * 2.f/3.f * glm::vec4(p_cam->look, 0.f);
                }
                numPossibleKeys++;
            }
            // Move left
            if (aDown) {
                if(!fDown && !m_isUnderLiquid) {
                    vel2 = (-1.f) * normalVel * glm::vec4(p_cam->right, 0.f);
                } else if (fDown) {
                    vel2 = (-1.f) * flyModeVel * glm::vec4(p_cam->right, 0.f);
                } else if (!fDown && m_isUnderLiquid) {
                    vel2 = (-1.f) * 2.f/3.f * normalVel * glm::vec4(p_cam->right, 0.f);
                }
                numPossibleKeys++;
            // move right
            } else if (dDown) {
                if(!fDown && !m_isUnderLiquid) {
                    vel2 = normalVel * glm::vec4(p_cam->right, 0.f);
                } else  if (fDown) {
                    vel2 = flyModeVel * glm::vec4(p_cam->right, 0.f);
                } else if (!fDown && m_isUnderLiquid) {
                     vel2 = normalVel * 2.f/3.f * glm::vec4(p_cam->right, 0.f);
                }
                numPossibleKeys++;
            }
            vel = (1.f / numPossibleKeys) * (vel1 + vel2);
            vel.y = curYVel;

        } else if (intervalSincePress >= normalInterval) { //has acceleration
            /// use the formula v_final  = v_initial + time * acceleration
            /// also caps the velocity
            if (!m_isUnderLiquid) {
                if (wDown) {
                    vel1 = vel + (-1.f) * timeChangeSec * accVal * glm::vec4(p_cam->look, 0.f);
                    numPossibleKeys ++;
                } else if (sDown) {
                    vel1 = vel + timeChangeSec * accVal * glm::vec4(p_cam->look, 0.f);
                    numPossibleKeys ++;
                }
                if (aDown) {
                    vel2 = vel + (-1.f) * timeChangeSec * accVal * glm::vec4(p_cam->right, 0.f);
                    numPossibleKeys ++;
                } else if (dDown) {
                    vel2 = vel + timeChangeSec * accVal * glm::vec4(p_cam->right, 0.f);
                    numPossibleKeys ++;
                }
            } else if (m_isUnderLiquid) {
                if (wDown) {
                    vel1 = 2.f/3.f * vel + (-1.f) * timeChangeSec * accVal * glm::vec4(p_cam->look, 0.f);
                    numPossibleKeys ++;
                } else if (sDown) {
                    vel1 = 2.f/3.f * vel + timeChangeSec * accVal * glm::vec4(p_cam->look, 0.f);
                    numPossibleKeys ++;
                }
                if (aDown) {
                    vel2 = 2.f/3.f * vel + (-1.f) * timeChangeSec * accVal * glm::vec4(p_cam->right, 0.f);
                    numPossibleKeys ++;
                } else if (dDown) {
                    vel2 = 2.f/3.f * vel + timeChangeSec * accVal * glm::vec4(p_cam->right, 0.f);
                    numPossibleKeys ++;
                }
            }

            vel = (1.f / numPossibleKeys) * (vel1 + vel2);
            if (vel.x > maxVel) {vel.x = maxVel;}
            if (vel.z > maxVel) {vel.z = maxVel;}
            vel.y = curYVel;
        }
    }

    /// handling velocity in y direction
    if (gravityOnPlayer != 0 && !m_isUnderLiquid) {
        /// use the formula v_final  = v_initial + time * accelerations
        vel.y += timeChangeSec * gravityOnPlayer;
    }
    else if (gravityOnPlayer != 0 && m_isUnderLiquid) {
        if (spaceDown) {
            vel.y += 0.01f;
        } else {
            vel.y += timeChangeSec * gravityOnPlayer;
        }
    }
    return oldVel;
}


glm::vec4 Player::updatePosAndCam(int64_t timeChange, glm::vec4 oldVel){
    glm::vec4 oldPos = pos;
    float timeChangeSec = (float) timeChange / 1000.f;
    ///use the formula dist = 0.5 * (v_f + v_i) * t
    glm::vec4 changeInPos = 0.5f * timeChangeSec * (vel + oldVel);
    float changeInPosY = changeInPos.y;
    if (wDown || sDown || aDown || dDown)
    {
        changeInPos.y = 0;
    }
    if (wDown)
    {
        p_cam->TranslateAlongLookXZ(glm::length(changeInPos));
    }
    else if (sDown)
    {
        p_cam->TranslateAlongLookXZ(-glm::length(changeInPos));
    }

    if (aDown)
    {
        p_cam->TranslateAlongRight(-glm::length(changeInPos));
    }
    else if (dDown)
    {
        p_cam->TranslateAlongRight(glm::length(changeInPos));
    }

    if (((qDown || eDown) && fDown) || gravityOnPlayer != 0) {
        p_cam->eye.y += changeInPosY;
        if (((qDown || eDown) && fDown)) { vel.y = 0;}
    }
    p_cam->RecomputeAttributes();
    pos = glm::vec4(p_cam->eye, 1.f);
    return oldPos;
}

void Player::handleCollision(glm::vec4 oldPos, Terrain* p_terrain) {
    /// If not in fly mode & there is no block at the player's feet, set the gravity acting on player to be gravityAcce
    /// only handle collision if
    /// - NOT fly mode currently
    /// - There is some change in position
    if (!fDown &&
            (glm::abs(pos.x - oldPos.x) > 0.00001 ||
             glm::abs(pos.y - oldPos.y) > 0.00001 ||
             glm::abs(pos.z - oldPos.z) > 0.00001)) {
        glm::vec3 rayOrigin = glm::vec3(oldPos.x, oldPos.y, oldPos.z);
        glm::vec3 rayDirection = glm::normalize(glm::vec3(pos.x, pos.y, pos.z) - rayOrigin);

        float t = 0.1;
        glm::vec3 lastArbPoint = rayOrigin;
        glm::vec3 posWithNoCollision = glm::vec3(pos.x, pos.y, pos.z);
        glm::vec3 arbPoint = rayOrigin + t * rayDirection;

        while (t <= 1.8f && glm::length(posWithNoCollision - arbPoint) < 0.2) {
            arbPoint = rayOrigin + t * rayDirection; /// arbitrary point on the ray - player/cam's position

            ///get the bounding box of the person at this arbitrary point
            float minXPerson = glm::floor(arbPoint.x - 0.4f);
            float maxXPerson = glm::floor(arbPoint.x + 0.4f);
            float minYPerson = glm::floor(arbPoint.y - 0.9f);
            float maxYPerson = glm::floor(arbPoint.y + 0.9f);
            float minZPerson = glm::floor(arbPoint.z - 0.4f);
            float maxZPerson = glm::floor(arbPoint.z + 0.4f);
            float x = minXPerson;
            float y = minYPerson;
            float z = minZPerson;
            bool breakXYZLoop = false;

            while (x < maxXPerson + 1) {
                while (y < maxYPerson + 1) {
                    while (z < maxZPerson + 1) {
                        /// check if there are blocks overlapping the person
                        BlockType block = p_terrain->getBlockAt(x, y, z);

                        if (block == WATER || block == LAVA)
                        {
                            m_isUnderLiquid = true;

                            m_liquidType = block;
                        }
                        else if (block != EMPTY && (block != WATER || block != LAVA))
                        {
                            breakXYZLoop = true;
                            wDown = false;
                            sDown = false;
                            aDown = false;
                            dDown = false;
                            vel = glm::vec4(0, 0, 0, 0);
                            p_cam->eye = lastArbPoint;
                            pos = glm::vec4(p_cam->eye, 1.f);
                        }
                        else if (block == EMPTY)
                        {

                            m_isUnderLiquid = false;
                            m_liquidType = block;
                        }

                        /// check if the person is on the ground. The if statement makes sure that
                        /// we are not checking the block right below the player's head instead
                        /// dealing with gravity
                        if (y == minYPerson) {
                            BlockType blockAtFeet = p_terrain->getBlockAt(x, y - 1, z);
                            // Player is at surface of water
                            if ((blockAtFeet == WATER || blockAtFeet == LAVA) && !m_isUnderLiquid && gravityOnPlayer != 0 &&
                                    vel.y <= 0 && glm::abs(arbPoint.y - y - 1) <= 0.2f)
                            {
                                gravityOnPlayer = gravityAcceleration * 2.f/3.f;
                            }
                            // Player is underwater
                            else if (m_isUnderLiquid)
                            {
                                gravityOnPlayer = gravityAcceleration * 2.f/3.f;
                            }
                            // Player is at surface of solid block (under or not under water)
                            else if (blockAtFeet != EMPTY && gravityOnPlayer != 0 &&
                                    vel.y <= 0 && glm::abs(arbPoint.y - y - 1) <= 0.2f)
                            {
                                gravityOnPlayer = 0;
                                vel.y = 0;
                                p_cam->eye.y = (int)y + 1;
                                pos = glm::vec4(p_cam->eye, 1.f);
                                breakXYZLoop = true;
                            }
                            // Player is in the air
                            else if (blockAtFeet == EMPTY){
                                gravityOnPlayer = gravityAcceleration;
                            }
                        }

                        if (breakXYZLoop) {break;}
                        z += 1.f;
                    }

                    if (breakXYZLoop) {break;}
                    z = minZPerson;
                    y += 1.f;
                }
                if (breakXYZLoop) {break;}
                z = minZPerson;
                y = minYPerson;
                x += 1.f;
            }
            if (breakXYZLoop) {break;}
            /// update the variables for the loop
            lastArbPoint = arbPoint;
            t += 0.1;
        }
    }
}
