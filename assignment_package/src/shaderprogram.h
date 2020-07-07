#ifndef SHADERPROGRAM_H
#define SHADERPROGRAM_H

#include <openglcontext.h>
#include <la.h>
#include <glm/glm.hpp>

#include "drawable.h"

class ShaderProgram
{
public:
    GLuint vertShader; // A handle for the vertex shader stored in this shader program
    GLuint fragShader; // A handle for the fragment shader stored in this shader program
    GLuint prog;       // A handle for the linked shader program stored in this class
    GLuint textureHandle;
    GLuint norTextureHandle;

    int attrPos; // A handle for the "in" vec4 representing vertex position in the vertex shader
    int attrNor; // A handle for the "in" vec4 representing vertex normal in the vertex shader
    int attrCol; // A handle for the "in" vec4 representing vertex color in the vertex shader
    int attrUV; // A handle for the "in" vec2 representng vertex UV in the vertex shader
    int attrCosPow; // A handle for the "in" float representing the cosine power in the vertex shader
    int attrAnimatable; // A handle for the "in" float which is != 0 if the object is animatable
//    int attrPosLightSpace; // A handle for the "in"  vec4 which tells where a vertex position is from the light's perspective, for shadow mapping.

    int unifModel; // A handle for the "uniform" mat4 representing model matrix in the vertex shader
    int unifModelInvTr; // A handle for the "uniform" mat4 representing inverse transpose of the model matrix in the vertex shader
    int unifViewProj; // A handle for the "uniform" mat4 representing combined projection and view matrices in the vertex shader
    int unifColor; // A handle for the "uniform" vec4 representing color of geometry in the vertex shader
    int unifSampler2DTexture; // A handle to the "uniform" sampler2D that will be used to read the texture containing the scene render
    int unifSampler2DNormal; // A handle to the "uniform" sampler2D that will be used to read the texture of normals containing the scene render
    int unifTime; // A handle for the "uniform" float representing time in the shader
    int unifCamPos; // A handle for the "uniform" vec4 representing the world position of the camera
    int unifDimensions; // A handle for the "uniform" vec4 representing the world position of the camera
    int unifLightSpaceMatrix;


public:
    ShaderProgram(OpenGLContext* context);
    // Sets up the requisite GL data and shaders from the given .glsl files
    void create(const char *vertfile, const char *fragfile);
    void createTexture();
    void createNorTexture();
//    void createDepthTexture();

    void setCamPosVector(const glm::vec3 &cam);

    // Draw the given object to our screen using this ShaderProgram's shaders
    void draw(Drawable &d);
    void drawNotInter(Drawable& d); // Draw not interleaved
    void drawNonOpaque(Drawable& d);
    void drawDepth(Drawable& d);

    // Utility function that prints any shader compilation errors to the console
    void printShaderInfoLog(int shader);
    // Utility function that prints any shader linking errors to the console
    void printLinkInfoLog(int prog);
    // Pass the given color to this shader on the GPU
    void setGeometryColor(glm::vec4 color);
    // Pass the given model matrix to this shader on the GPU
    void setModelMatrix(const glm::mat4 &model);
    // Pass the given Projection * View matrix to this shader on the GPU
    void setViewProjMatrix(const glm::mat4 &vp);
    // Pass the time to this shader on the GPU - helpful for animatable objectss
    void setTime(int t);
    // Pass light space matrix to convert a vertex's position into that from light's perspective for shadow mapping
    void setLightSpaceMatrix(const glm::mat4& lightSpaceMx); //--T
    // Pass the dimensions to sky
    void setDimensions(int width, int height);

    // Utility function used in create()
    char* textFileRead(const char*);
    // Tells our OpenGL context to use this shader to draw things
    void useMe();

    QString qTextFileRead(const char*);

    // Post process overlay
    void drawOverlay(Drawable& d, int textureSlot);

private:
    OpenGLContext* context;   // Since Qt's OpenGL support is done through classes like QOpenGLFunctions_3_2_Core,
    // we need to pass our OpenGL context to the Drawable in order to call GL functions
    // from within this class.
};


#endif // SHADERPROGRAM_H
