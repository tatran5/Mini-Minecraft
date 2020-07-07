#pragma once

#ifndef QUAD_H
#define QUAD_H

#include "drawable.h"
#include <QOpenGLContext>
#include "terrain.h"

class Quad : public Drawable
{
private:
    float alpha;
public:
    BlockType blockType;
    Quad(OpenGLContext* context, BlockType t);
    virtual void create();
    virtual void createFar();
    void createPostProcess();

    void setAlpha(float newAlpha);
};

#endif // QUAD_H
