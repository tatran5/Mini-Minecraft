#pragma once

#include <openglcontext.h>
#include <la.h>
#include <iostream>

//This defines a class which can be rendered by our shader program.
//Make any geometry a subclass of ShaderProgram::Drawable in order to render it with the ShaderProgram class.
class Drawable
{
protected:
    int count;     // The number of indices stored in bufIdx.

    int transCount;     // The number of indices stored in bufIdx.

    GLuint bufIdx; // A Vertex Buffer Object that we will use to store triangle indices (GLuints)
    GLuint bufPos; // A Vertex Buffer Object that we will use to store mesh vertices (vec4s)
    GLuint bufNor; // A Vertex Buffer Object that we will use to store mesh normals (vec4s)
    GLuint bufCol; // Can be used to pass per-vertex color information to the shader, but is currently unused.
                   // Instead, we use a uniform vec4 in the shader to set an overall color for the geometry
    GLuint bufUV; // A Vertex Buffer Object that we will use to store mesh UV for texture mapping (vec2s)
    GLuint bufCosPow; // A Vertex Buffer Object that we will use to store cosine power for how shiny the object is
    GLuint bufAnimatable; //A Vertex Buffer Object that we will use to store whether we have to animate the texture

    GLuint bufInter;
    GLuint bufIdxNonOp;
    GLuint bufInterNonOp; // for the nonopaque geometry

    bool idxBound; // Set to TRUE by generateIdx(), returned by bindIdx().
    bool posBound;
    bool norBound;
    bool colBound;
    bool uvBound;
    bool cosPowBound;
    bool animatableBound;
    bool interBound;
    bool interNonOpBound; // for the nonopaque geometry

    bool idxNonOpBound;
    OpenGLContext* context; // Since Qt's OpenGL support is done through classes like QOpenGLFunctions_3_2_Core,
                          // we need to pass our OpenGL context to the Drawable in order to call GL functions
                          // from within this class.

public:
    Drawable(OpenGLContext* context);
    virtual ~Drawable();

    virtual void create() = 0; // To be implemented by subclasses. Populates the VBOs of the Drawable.
    void destroy(); // Frees the VBOs of the Drawable.

    // Getter functions for various GL data
    virtual GLenum drawMode();
    int elemCount();

    int transElemCount();


    // Call these functions when you want to call glGenBuffers on the buffers stored in the Drawable
    // These will properly set the values of idxBound etc. which need to be checked in ShaderProgram::draw()
    void generateIdx();
    void generatePos();
    void generateNor();
    void generateCol();
    void generateUV();
    void generateCosPow();
    void generateAnimatable();
    void generateInterleaved();

    void generateIdxNonOp();
    void generateInterleavedNonOp();

    bool bindIdx();
    bool bindPos();
    bool bindNor();
    bool bindCol();
    bool bindUV();
    bool bindCosPow();
    bool bindAnimatable();
    bool bindInterleaved();

    bool bindIdxNonOp();
    bool bindInterleavedNonOp();

};
