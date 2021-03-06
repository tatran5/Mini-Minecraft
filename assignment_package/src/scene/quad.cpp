#include "quad.h"
#include <QDebug>

Quad::Quad(OpenGLContext* context, BlockType t) : Drawable(context), blockType(t), alpha(0.f)
{}

void Quad::setAlpha(float newAlpha)
{
    alpha = newAlpha;
}

void Quad::create()
{
    GLuint idx[6]{0, 1, 2, 0, 2, 3};

    glm::vec4 vert_pos[4] {glm::vec4(-1.f, -1.f, 0.000001f, 1.f),
                           glm::vec4(1.f, -1.f, 0.000001f, 1.f),
                           glm::vec4(1.f, 1.f, 0.000001f, 1.f),
                           glm::vec4(-1.f, 1.f, 0.000001f, 1.f)};


    glm::vec4 vert_col[4];

    if (blockType == LAVA)
    {
        vert_col[0] = glm::vec4(1, 0, 0, alpha);
        vert_col[1] = glm::vec4(1, 0, 0, alpha);
        vert_col[2] = glm::vec4(1, 0, 0, alpha);
        vert_col[3] = glm::vec4(1, 0, 0, alpha);
    }
    else if (blockType == WATER)
    {
        vert_col[0] = glm::vec4(0, 0, 1, alpha);
        vert_col[1] = glm::vec4(0, 0, 1, alpha);
        vert_col[2] = glm::vec4(0, 0, 1, alpha);
        vert_col[3] = glm::vec4(0, 0, 1, alpha);
    }

    count = 6;
    generateIdx();
    context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufIdx);
    context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLuint), idx, GL_STATIC_DRAW);
    generatePos();
    context->glBindBuffer(GL_ARRAY_BUFFER, bufPos);
    context->glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(glm::vec4), vert_pos, GL_STATIC_DRAW);
    generateCol();
    context->glBindBuffer(GL_ARRAY_BUFFER, bufCol);
    context->glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(glm::vec4), vert_col, GL_STATIC_DRAW);
}

void Quad::createFar()
{
    GLuint idx[6]{0, 1, 2, 0, 2, 3};

    glm::vec4 vert_pos[4] {glm::vec4(-1.f, -1.f, 0.999999f, 1.f),
                           glm::vec4(1.f, -1.f, 0.999999f, 1.f),
                           glm::vec4(1.f, 1.f, 0.999999f, 1.f),
                           glm::vec4(-1.f, 1.f, 0.999999f, 1.f)};

    glm::vec4 vert_col[4];

    if (blockType == LAVA)
    {
        qDebug() << "LAVA!";
        vert_col[0] = glm::vec4(1, 0, 0, alpha);
        vert_col[1] = glm::vec4(1, 0, 0, alpha);
        vert_col[2] = glm::vec4(1, 0, 0, alpha);
        vert_col[3] = glm::vec4(1, 0, 0, alpha);
    }
    else if (blockType == WATER)
    {
        qDebug() << "WATER!";
        vert_col[0] = glm::vec4(0, 0, 1, alpha);
        vert_col[1] = glm::vec4(0, 0, 1, alpha);
        vert_col[2] = glm::vec4(0, 0, 1, alpha);
        vert_col[3] = glm::vec4(0, 0, 1, alpha);
    }

    count = 6;
    generateIdx();
    context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufIdx);
    context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLuint), idx, GL_STATIC_DRAW);
    generatePos();
    context->glBindBuffer(GL_ARRAY_BUFFER, bufPos);
    context->glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(glm::vec4), vert_pos, GL_STATIC_DRAW);
    generateCol();
    context->glBindBuffer(GL_ARRAY_BUFFER, bufCol);
    context->glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(glm::vec4), vert_col, GL_STATIC_DRAW);
}


void Quad::createPostProcess()
{
    GLuint idx[6]{0, 1, 2, 0, 2, 3};
    glm::vec4 vert_pos[4] {glm::vec4(-1.f, -1.f, 0.99f, 1.f),
                           glm::vec4(1.f, -1.f, 0.99f, 1.f),
                           glm::vec4(1.f, 1.f, 0.99f, 1.f),
                           glm::vec4(-1.f, 1.f, 0.99f, 1.f)};

    glm::vec2 vert_UV[4] {glm::vec2(0.f, 0.f),
                          glm::vec2(1.f, 0.f),
                          glm::vec2(1.f, 1.f),
                          glm::vec2(0.f, 1.f)};

    count = 6;

    // Create a VBO on our GPU and store its handle in bufIdx
    generateIdx();
    // Tell OpenGL that we want to perform subsequent operations on the VBO referred to by bufIdx
    // and that it will be treated as an element array buffer (since it will contain triangle indices)
    context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufIdx);
    // Pass the data stored in cyl_idx into the bound buffer, reading a number of bytes equal to
    // CYL_IDX_COUNT multiplied by the size of a GLuint. This data is sent to the GPU to be read by shader programs.
    context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLuint), idx, GL_STATIC_DRAW);

    // The next few sets of function calls are basically the same as above, except bufPos and bufNor are
    // array buffers rather than element array buffers, as they store vertex attributes like position.
    generatePos();
    context->glBindBuffer(GL_ARRAY_BUFFER, bufPos);
    context->glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(glm::vec4), vert_pos, GL_STATIC_DRAW);
    generateUV();
    context->glBindBuffer(GL_ARRAY_BUFFER, bufUV);
    context->glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(glm::vec2), vert_UV, GL_STATIC_DRAW);

}



