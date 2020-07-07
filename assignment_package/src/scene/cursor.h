#pragma once

#include "drawable.h"
#include <la.h>

#include <QOpenGLContext>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>

class Cursor : public Drawable
{
public:
    Cursor(OpenGLContext* context) : Drawable(context){}
    virtual ~Cursor(){}
    void create();
    GLenum drawMode();
};
