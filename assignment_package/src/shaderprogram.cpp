#include "shaderprogram.h"
#include <QFile>
#include <QStringBuilder>
#include <QTextStream>
#include <QDebug>


ShaderProgram::ShaderProgram(OpenGLContext *context)
    : vertShader(), fragShader(), prog(),
      attrPos(-1), attrNor(-1), attrCol(-1),
      attrUV(-1), attrCosPow(-1), attrAnimatable(-1),
      /*attrPosLightSpace(-1),*/
      unifModel(-1), unifModelInvTr(-1), unifViewProj(-1),
      unifColor(-1), unifLightSpaceMatrix(-1),
      unifSampler2DTexture(-1), unifSampler2DNormal(-1),
      unifTime(-1), unifCamPos(-1), context(context), unifDimensions(-1)
{}

void ShaderProgram::create(const char *vertfile, const char *fragfile)
{
    // Allocate space on our GPU for a vertex shader and a fragment shader and a shader program to manage the two
    vertShader = context->glCreateShader(GL_VERTEX_SHADER);
    fragShader = context->glCreateShader(GL_FRAGMENT_SHADER);
    prog = context->glCreateProgram();
    // Get the body of text stored in our two .glsl files
    QString qVertSource = qTextFileRead(vertfile);
    QString qFragSource = qTextFileRead(fragfile);

    char* vertSource = new char[qVertSource.size()+1];
    strcpy(vertSource, qVertSource.toStdString().c_str());
    char* fragSource = new char[qFragSource.size()+1];
    strcpy(fragSource, qFragSource.toStdString().c_str());


    // Send the shader text to OpenGL and store it in the shaders specified by the handles vertShader and fragShader
    context->glShaderSource(vertShader, 1, &vertSource, 0);
    context->glShaderSource(fragShader, 1, &fragSource, 0);
    // Tell OpenGL to compile the shader text stored above
    context->glCompileShader(vertShader);
    context->glCompileShader(fragShader);
    // Check if everything compiled OK
    GLint compiled;
    context->glGetShaderiv(vertShader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        printShaderInfoLog(vertShader);
    }
    context->glGetShaderiv(fragShader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        printShaderInfoLog(fragShader);
    }

    // Tell prog that it manages these particular vertex and fragment shaders
    context->glAttachShader(prog, vertShader);
    context->glAttachShader(prog, fragShader);
    context->glLinkProgram(prog);

    // Check for linking success
    GLint linked;
    context->glGetProgramiv(prog, GL_LINK_STATUS, &linked);
    if (!linked) {
        printLinkInfoLog(prog);
    }

    // Get the handles to the variables stored in our shaders
    // See shaderprogram.h for more information about these variables

    attrPos = context->glGetAttribLocation(prog, "vs_Pos");
    attrNor = context->glGetAttribLocation(prog, "vs_Nor");
    attrCol = context->glGetAttribLocation(prog, "vs_Col");
    attrUV = context->glGetAttribLocation(prog, "vs_UV");
    attrCosPow = context->glGetAttribLocation(prog, "vs_CosPow");
    attrAnimatable = context->glGetAttribLocation(prog, "vs_Animatable");
//    attrPosLightSpace = context->glGetAttribLocation(prog, "vs_PosLightSpace");

    unifModel      = context->glGetUniformLocation(prog, "u_Model");
    unifModelInvTr = context->glGetUniformLocation(prog, "u_ModelInvTr");
    unifViewProj   = context->glGetUniformLocation(prog, "u_ViewProj");
    unifColor      = context->glGetUniformLocation(prog, "u_Color");
    unifTime = context->glGetUniformLocation(prog, "u_Time");
    unifCamPos = context->glGetUniformLocation(prog, "u_CameraPos");
    unifDimensions = context->glGetUniformLocation(prog, "u_Dimensions");
    unifLightSpaceMatrix = context->glGetUniformLocation(prog, "u_LightSpaceMatrix");
}



void ShaderProgram::setDimensions(int width, int height) {
    useMe();

    if(unifDimensions != -1) {
        context -> glUniform2i(unifDimensions, width, height);
    }
}


void ShaderProgram::createTexture() {
    useMe();
    // setting up texture in OpenGL
    context->glGenTextures(1, &textureHandle);
    context->glActiveTexture(GL_TEXTURE0); //GL supports up to 32 different active textures at once(0 - 31)
    context->glBindTexture(GL_TEXTURE_2D, textureHandle);

    // set the texture wrapping/filtering options (on the currently bound texture object)

    context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    context->printGLErrorLog();

    // load image
    QImage textureImage(":/textures/minecraft_textures_all.png");
    textureImage.convertToFormat(QImage::Format_ARGB32);
    //    textureImage = textureImage.mirrored(); // Flip the image upside-down to conform to OpenGL
    context->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                          textureImage.width(), textureImage.height(),
                          0, GL_BGRA, GL_UNSIGNED_BYTE, textureImage.bits());
    context->printGLErrorLog();

    // passing the texture into the GPU
    unifSampler2DTexture  = context->glGetUniformLocation(prog, "u_Sampler2DTexture");
    context->glUniform1i(unifSampler2DTexture, 0);
}

void ShaderProgram::createNorTexture() {
    useMe();
    // setting up texture in OpenGL
    context->glGenTextures(1, &norTextureHandle);
    context->glActiveTexture(GL_TEXTURE0); //GL supports up to 32 different active textures at once(0 - 31)
    context->glBindTexture(GL_TEXTURE_2D, norTextureHandle);

    // set the texture wrapping/filtering options (on the currently bound texture object)
    context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    context->printGLErrorLog();

    // load image
    QImage textureImage(":/textures/minecraft_normals_all.png");
    textureImage.convertToFormat(QImage::Format_ARGB32);
    textureImage = textureImage.mirrored(); // Flip the image upside-down to conform to OpenGL
    context->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                          textureImage.width(), textureImage.height(),
                          0, GL_BGRA, GL_UNSIGNED_BYTE, textureImage.bits());
    context->printGLErrorLog();

    // passing the normaltexture into the GPU
    unifSampler2DNormal  = context->glGetUniformLocation(prog, "u_Sampler2DNormal");
    context->glUniform1i(unifSampler2DNormal, 0);
}

void ShaderProgram::setCamPosVector(const glm::vec3 &cam) {
    useMe();

    if(unifCamPos != -1) {
        context -> glUniform3fv(unifCamPos, 1, &cam[0]);
    }

    context->printGLErrorLog();

}

void ShaderProgram::setViewProjMatrix(const glm::mat4 &vp)
{
    // Tell OpenGL to use this shader program for subsequent function calls
    useMe();

    if(unifViewProj != -1) {
        // Pass a 4x4 matrix into a uniform variable in our shader
        // Handle to the matrix variable on the GPU
        context->glUniformMatrix4fv(unifViewProj,
                                    // How many matrices to pass
                                    1,
                                    // Transpose the matrix? OpenGL uses column-major, so no.
                                    GL_FALSE,
                                    // Pointer to the first element of the matrix
                                    &vp[0][0]);
    }
}

void ShaderProgram::drawNotInter(Drawable &d) {
    useMe();

    // Each of the following blocks checks that:
    //   * This shader has this attribute, and
    //   * This Drawable has a vertex buffer for this attribute.
    // If so, it binds the appropriate buffers to each attribute.

    // Remember, by calling bindPos(), we call
    // glBindBuffer on the Drawable's VBO for vertex position,
    // meaning that glVertexAttribPointer associates vs_Pos
    // (referred to by attrPos) with that VBO
    if (attrPos != -1 && d.bindPos()) {
        context->glEnableVertexAttribArray(attrPos);
        context->glVertexAttribPointer(attrPos, 4, GL_FLOAT, false, 0, NULL);
    }

    if (attrNor != -1 && d.bindNor()) {
        context->glEnableVertexAttribArray(attrNor);
        context->glVertexAttribPointer(attrNor, 4, GL_FLOAT, false, 0, NULL);
    }

    if (attrCol != -1 && d.bindCol()) {
        context->glEnableVertexAttribArray(attrCol);
        context->glVertexAttribPointer(attrCol, 4, GL_FLOAT, false, 0, NULL);
    }

    // Bind the index buffer and then draw shapes from it.
    // This invokes the shader program, which accesses the vertex buffers.
    d.bindIdx();
    context->glDrawElements(d.drawMode(), d.elemCount(), GL_UNSIGNED_INT, 0);

    if (attrPos != -1) context->glDisableVertexAttribArray(attrPos);
    if (attrNor != -1) context->glDisableVertexAttribArray(attrNor);
    if (attrCol != -1) context->glDisableVertexAttribArray(attrCol);

    context->printGLErrorLog();
}

void ShaderProgram::drawDepth(Drawable &d) {
    useMe();

    if(d.bindInterleaved()) {
        if (attrPos != -1) {
            context->glEnableVertexAttribArray(attrPos);
            context->glVertexAttribPointer(attrPos, 4, GL_FLOAT, false,
                                           3 * sizeof(glm::vec4),
                                           (void*) 0);
        }

        d.bindIdx();
        context->glDrawElements(d.drawMode(), d.elemCount(), GL_UNSIGNED_INT, 0);

        if (attrPos != -1) { context->glDisableVertexAttribArray(attrPos); }
    }

    context->printGLErrorLog();
}

//This function, as its name implies, uses the passed in GL widget
void ShaderProgram::draw(Drawable &d)
{
    useMe();
    if (unifSampler2DTexture != -1) {
        context->glActiveTexture(GL_TEXTURE0); //GL supports up to 32 different active textures at once(0 - 31)
        context->glBindTexture(GL_TEXTURE_2D, textureHandle);
        context->glUniform1i(unifSampler2DTexture, 0);
    }
    if (unifSampler2DNormal != -1) {
        context->glUniform1i(unifSampler2DNormal, 0);
    }
    /*// Each of the following blocks checks that:
    //   * This shader has this attribute, and
    //   * This Drawable has a vertex buffer for this attribute.
    // If so, it binds the appropriate buffers to each attribute.

    // Remember, by calling bindPos(), we call
    // glBindBuffer on the Drawable's VBO for vertex position,
    // meaning that glVertexAttribPointer associates vs_Pos
    // (referred to by attrPos) with that VBO

    // The reason for stride and the offset (the last 2 argument of glVertexAttribPointer:
    // 0       |1       |2       |3
    // pos.x   |pos.y   |pos.z   |pos.w

    // 4       |5       |6       |7
    // nor.x   |nor.y   |nor.z   |nor.w

    // 8       |9       |10      |11
    // uv.x    |uv.y    |cosPow  |animatable

    // 12 slots/floats, 3 vec4s*/

    if(d.bindInterleaved()) {
        if (attrPos != -1) {
            context->glEnableVertexAttribArray(attrPos);
            context->glVertexAttribPointer(attrPos, 4, GL_FLOAT, false,
                                           3 * sizeof(glm::vec4),
                                           (void*) 0);
        }

        if (attrNor != -1) {
            context->glEnableVertexAttribArray(attrNor);
            context->glVertexAttribPointer(attrNor, 4, GL_FLOAT, false,
                                           3 * sizeof(glm::vec4),
                                           (void*) sizeof(glm::vec4));
        }


        if (attrUV != -1) {
            context->glEnableVertexAttribArray(attrUV);
            context->glVertexAttribPointer(attrUV, 2, GL_FLOAT, false,
                                           3 * sizeof(glm::vec4),
                                           (void*) (2 * sizeof(glm::vec4)));
        }

        if (attrCosPow != -1) {
            context->glEnableVertexAttribArray(attrCosPow);
            context->glVertexAttribPointer(attrCosPow, 1, GL_FLOAT, false,
                                           3 * sizeof(glm::vec4),
                                           (void*) (2 * (sizeof(glm::vec4))
                                                    + sizeof(glm::vec2))); /*UV memory*/
        }

        if (attrAnimatable != -1) {
            context->glEnableVertexAttribArray(attrAnimatable);
            context->glVertexAttribPointer(attrAnimatable, 1, GL_FLOAT, false,
                                           3 * sizeof(glm::vec4),
                                           (void*) (2 * sizeof(glm::vec4)
                                                    + sizeof(glm::vec2) /*UV memory*/
                                                    + sizeof(float))); /*cosine power*/
        }
    }

    // Bind the index buffer and then draw shapes from it.
    // This invokes the shader program, which accesses the vertex buffers.
    d.bindIdx();
    context->glDrawElements(d.drawMode(), d.elemCount(), GL_UNSIGNED_INT, 0);

    if (attrPos != -1) context->glDisableVertexAttribArray(attrPos);
    if (attrNor != -1) context->glDisableVertexAttribArray(attrNor);
    if (attrUV != -1) context->glDisableVertexAttribArray(attrUV);
    if (attrCosPow != -1) context->glDisableVertexAttribArray(attrCosPow);
    if (attrAnimatable != -1) context->glDisableVertexAttribArray(attrAnimatable);

    context->printGLErrorLog();
}

//This function, as its name implies, uses the passed in GL widget
void ShaderProgram::drawNonOpaque(Drawable &d)
{
    useMe();

    if(d.bindInterleavedNonOp()) {
        if (attrPos != -1) {
            context->glEnableVertexAttribArray(attrPos);
            context->glVertexAttribPointer(attrPos, 4, GL_FLOAT, false,
                                           4 * sizeof(glm::vec4),
                                           (void*) 0);
        }

        if (attrNor != -1) {
            context->glEnableVertexAttribArray(attrNor);
            context->glVertexAttribPointer(attrNor, 4, GL_FLOAT, false,
                                           4 * sizeof(glm::vec4),
                                           (void*) sizeof(glm::vec4));
        }

        if (attrCol != -1) {
            context->glEnableVertexAttribArray(attrCol);
            context->glVertexAttribPointer(attrCol, 4, GL_FLOAT, false,
                                           4 * sizeof(glm::vec4),
                                           (void*) (2 * sizeof(glm::vec4)));
        }

        if (attrUV != -1) {
            context->glEnableVertexAttribArray(attrUV);
            context->glVertexAttribPointer(attrUV, 2, GL_FLOAT, false,
                                           4 * sizeof(glm::vec4),
                                           (void*) (3 * sizeof(glm::vec4)));
        }

        if (attrCosPow != -1) {
            context->glEnableVertexAttribArray(attrCosPow);
            context->glVertexAttribPointer(attrCosPow, 1, GL_FLOAT, false,
                                           4 * sizeof(glm::vec4),
                                           (void*) (3 * (sizeof(glm::vec4))
                                                         + sizeof(glm::vec2))); /*UV memory*/
        }

        if (attrAnimatable != -1) {
            context->glEnableVertexAttribArray(attrAnimatable);
            context->glVertexAttribPointer(attrAnimatable, 1, GL_FLOAT, false,
                                           4 * sizeof(glm::vec4),
                                           (void*) (3 * sizeof(glm::vec4)
                                                    + sizeof(glm::vec2) /*UV memory*/
                                                    + sizeof(float))); /*cosine power*/
        }
    }

    // Bind the index buffer and then draw shapes from it.
    // This invokes the shader program, which accesses the vertex buffers.
    d.bindIdxNonOp();
    context->glDrawElements(d.drawMode(), d.transElemCount(), GL_UNSIGNED_INT, 0);

    if (attrPos != -1) context->glDisableVertexAttribArray(attrPos);
    if (attrNor != -1) context->glDisableVertexAttribArray(attrNor);
    if (attrCol != -1) context->glDisableVertexAttribArray(attrCol);
    if (attrUV != -1) context->glDisableVertexAttribArray(attrUV);
    if (attrCosPow != -1) context->glDisableVertexAttribArray(attrCosPow);
    if (attrAnimatable != -1) context->glDisableVertexAttribArray(attrAnimatable);

    context->printGLErrorLog();
}

void ShaderProgram::drawOverlay(Drawable &d, int textureSlot)
{
    useMe();

    // Set our "renderedTexture" sampler to user Texture Unit 0
    context->glUniform1i(unifSampler2DTexture, textureSlot);

    // Each of the following blocks checks that:
    //   * This shader has this attribute, and
    //   * This Drawable has a vertex buffer for this attribute.
    // If so, it binds the appropriate buffers to each attribute.

    if (attrPos != -1 && d.bindPos()) {
        context->glEnableVertexAttribArray(attrPos);
        context->glVertexAttribPointer(attrPos, 4, GL_FLOAT, false, 0, NULL);
    }
    if (attrUV != -1 && d.bindUV()) {
        context->glEnableVertexAttribArray(attrUV);
        context->glVertexAttribPointer(attrUV, 2, GL_FLOAT, false, 0, NULL);
    }

    // Bind the index buffer and then draw shapes from it.
    // This invokes the shader program, which accesses the vertex buffers.
    d.bindIdx();
    context->glDrawElements(d.drawMode(), d.elemCount(), GL_UNSIGNED_INT, 0);

    if (attrPos != -1) context->glDisableVertexAttribArray(attrPos);
    if (attrUV != -1) context->glDisableVertexAttribArray(attrUV);

    context->printGLErrorLog();
}



QString ShaderProgram::qTextFileRead(const char *fileName)
{
    QString text;
    QFile file(fileName);
    if(file.open(QFile::ReadOnly))
    {
        QTextStream in(&file);
        text = in.readAll();
        text.append('\0');
    }
    return text;
}

void ShaderProgram::printShaderInfoLog(int shader)
{
    int infoLogLen = 0;
    int charsWritten = 0;
    GLchar *infoLog;

    context->glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);

    // should additionally check for OpenGL errors here

    if (infoLogLen > 0)
    {
        infoLog = new GLchar[infoLogLen];
        // error check for fail to allocate memory omitted
        context->glGetShaderInfoLog(shader,infoLogLen, &charsWritten, infoLog);
        qDebug() << "ShaderInfoLog:" << endl << infoLog << endl;
        delete [] infoLog;
    }

    // should additionally check for OpenGL errors here
}

void ShaderProgram::printLinkInfoLog(int prog)
{
    int infoLogLen = 0;
    int charsWritten = 0;
    GLchar *infoLog;

    context->glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &infoLogLen);

    // should additionally check for OpenGL errors here

    if (infoLogLen > 0) {
        infoLog = new GLchar[infoLogLen];
        // error check for fail to allocate memory omitted
        context->glGetProgramInfoLog(prog, infoLogLen, &charsWritten, infoLog);
        qDebug() << "LinkInfoLog:" << endl << infoLog << endl;
        delete [] infoLog;
    }
}

void ShaderProgram::setGeometryColor(glm::vec4 color)
{
    useMe();

    if(unifColor != -1)
    {
        context->glUniform4fv(unifColor, 1, &color[0]);
    }
}
void ShaderProgram::setLightSpaceMatrix(const glm::mat4& lightSpaceMx) {
    useMe();

    if (unifLightSpaceMatrix != -1) {
        context->glUniformMatrix4fv(unifLightSpaceMatrix, 1, GL_FALSE, &lightSpaceMx[0][0]);
    }
}

void ShaderProgram::setModelMatrix(const glm::mat4 &model)
{
    useMe();

    if (unifModel != -1) {
        // Pass a 4x4 matrix into a uniform variable in our shader
        // Handle to the matrix variable on the GPU
        context->glUniformMatrix4fv(unifModel,
                                    // How many matrices to pass
                                    1,
                                    // Transpose the matrix? OpenGL uses column-major, so no.
                                    GL_FALSE,
                                    // Pointer to the first element of the matrix
                                    &model[0][0]);
    }

    if (unifModelInvTr != -1) {
        glm::mat4 modelinvtr = glm::inverse(glm::transpose(model));
        // Pass a 4x4 matrix into a uniform variable in our shader
        // Handle to the matrix variable on the GPU
        context->glUniformMatrix4fv(unifModelInvTr,
                                    // How many matrices to pass
                                    1,
                                    // Transpose the matrix? OpenGL uses column-major, so no.
                                    GL_FALSE,
                                    // Pointer to the first element of the matrix
                                    &modelinvtr[0][0]);
    }
}



void ShaderProgram::setTime(int t) {
    useMe();

    if (unifTime != -1) {
        context->glUniform1i(unifTime, t);
    }
}

char* ShaderProgram::textFileRead(const char* fileName) {
    char* text;

    if (fileName != NULL) {
        FILE *file = fopen(fileName, "rt");

        if (file != NULL) {
            fseek(file, 0, SEEK_END);
            int count = ftell(file);
            rewind(file);

            if (count > 0) {
                text = (char*)malloc(sizeof(char) * (count + 1));
                count = fread(text, sizeof(char), count, file);
                text[count] = '\0';	//cap off the string with a terminal symbol, fixed by Cory
            }
            fclose(file);
        }
    }
    return text;
}

void ShaderProgram::useMe()
{
    context->glUseProgram(prog);
}
