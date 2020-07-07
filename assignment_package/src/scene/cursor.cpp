#include "cursor.h"

void Cursor::create()
{

    std::vector<glm::vec4> pos;

    pos.push_back(glm::vec4(-0.03,0,0,1));
    pos.push_back( glm::vec4(0.03,0,0,1));
    pos.push_back(glm::vec4(0,-0.03,0,1));
    pos.push_back(glm::vec4(0,0.03,0,1));


    GLuint idx[4] = {0, 1, 2, 3};

    glm::vec4 col[4] = {glm::vec4(1,1,1,1), glm::vec4(1,1,1,1),
                        glm::vec4(1,1,1,1), glm::vec4(1,1,1,1)};


    count = 4;

    /*
    generateInterleaved();
    context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufInter);
    context->glBufferData(GL_ARRAY_BUFFER, inter.size() * sizeof(glm::vec4), inter.data(), GL_STATIC_DRAW);
*/
    generateIdx();
    context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufIdx);
    context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, 4 * sizeof(GLuint), idx, GL_STATIC_DRAW);

    generatePos();
    context->glBindBuffer(GL_ARRAY_BUFFER, bufPos);
    context->glBufferData(GL_ARRAY_BUFFER, pos.size() * sizeof(glm::vec4), pos.data(), GL_STATIC_DRAW);
    generateCol();
    context->glBindBuffer(GL_ARRAY_BUFFER, bufCol);
    context->glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(glm::vec4), col, GL_STATIC_DRAW);
}

GLenum Cursor::drawMode()
{
    return GL_LINES;
}
