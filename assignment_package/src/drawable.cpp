#include "drawable.h"
#include <la.h>

Drawable::Drawable(OpenGLContext* context)
    : bufIdx(), bufPos(), bufNor(), bufCol(), bufInter(),
      idxBound(false), posBound(false), norBound(false), colBound(false),
      interBound(false), context(context)
{}

Drawable::~Drawable()
{}


void Drawable::destroy()
{
    context->glDeleteBuffers(1, &bufIdx);
    context->glDeleteBuffers(1, &bufPos);
    context->glDeleteBuffers(1, &bufNor);
    context->glDeleteBuffers(1, &bufCol);
    context->glDeleteBuffers(1, &bufInter);

}

GLenum Drawable::drawMode()
{
    // Since we want every three indices in bufIdx to be
    // read to draw our Drawable, we tell that the draw mode
    // of this Drawable is GL_TRIANGLES

    // If we wanted to draw a wireframe, we would return GL_LINES

    return GL_TRIANGLES;
}

int Drawable::elemCount()
{
    return count;
}


int Drawable::transElemCount()
{
    return transCount;
}

void Drawable::generateInterleaved()
{
    interBound = true;
    context -> glGenBuffers(1, &bufInter);
}

void Drawable::generateInterleavedNonOp()
{
    interNonOpBound = true;
    context -> glGenBuffers(1, &bufInterNonOp);
}


void Drawable::generateIdxNonOp()
{
    idxNonOpBound = true;
    // Create a VBO on our GPU and store its handle in bufIdx
    context->glGenBuffers(1, &bufIdxNonOp);
}

void Drawable::generateIdx()
{
    idxBound = true;
    // Create a VBO on our GPU and store its handle in bufIdx
    context->glGenBuffers(1, &bufIdx);
}

void Drawable::generatePos()
{
    posBound = true;
    // Create a VBO on our GPU and store its handle in bufPos
    context->glGenBuffers(1, &bufPos);
}

void Drawable::generateNor()
{
    norBound = true;
    // Create a VBO on our GPU and store its handle in bufNor
    context->glGenBuffers(1, &bufNor);
}

void Drawable::generateCol()
{
    colBound = true;
    // Create a VBO on our GPU and store its handle in bufCol
    context->glGenBuffers(1, &bufCol);
}

void Drawable::generateUV()
{
    uvBound = true;
    // Create a VBO on our GPU and store its handle in bufUV
    context->glGenBuffers(1, &bufUV);
}

void Drawable::generateCosPow()
{
    cosPowBound = true;
    // Create a VBO on our GPU and store its handle in bufCosPow
    context->glGenBuffers(1, &bufCosPow);
}

void Drawable::generateAnimatable()
{
    animatableBound = true;
    // Create a VBO on our GPU and store its handle in bufAnimatable
    context->glGenBuffers(1, &bufAnimatable);
}

bool Drawable::bindIdx()
{
    if(idxBound) {
        context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufIdx);
    }
    return idxBound;
}

bool Drawable::bindIdxNonOp()
{
    if(idxNonOpBound) {
        context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufIdxNonOp);
    }
    return idxNonOpBound;
}

bool Drawable::bindPos()
{
    if(posBound){
        context->glBindBuffer(GL_ARRAY_BUFFER, bufPos);
    }
    return posBound;
}

bool Drawable::bindNor()
{
    if(norBound){
        context->glBindBuffer(GL_ARRAY_BUFFER, bufNor);
    }
    return norBound;
}

bool Drawable::bindCol()
{
    if(colBound){
        context->glBindBuffer(GL_ARRAY_BUFFER, bufCol);
    }
    return colBound;
}

bool Drawable::bindUV()
{
    if(uvBound) {
        context->glBindBuffer(GL_ARRAY_BUFFER, bufUV);
    }
    return uvBound;
}

bool Drawable::bindCosPow()
{
    if(cosPowBound) {
        context->glBindBuffer(GL_ARRAY_BUFFER, bufCosPow);
    }
    return cosPowBound;
}

bool Drawable::bindAnimatable()
{
    if(animatableBound) {
        context->glBindBuffer(GL_ARRAY_BUFFER, bufAnimatable);
    }
    return animatableBound;
}

bool Drawable::bindInterleaved() {
    if(interBound) {
        context -> glBindBuffer(GL_ARRAY_BUFFER, bufInter);
    }
    return interBound;
}


bool Drawable::bindInterleavedNonOp() {
    if(interNonOpBound) {
        context -> glBindBuffer(GL_ARRAY_BUFFER, bufInterNonOp);
    }
    return interNonOpBound;
}


