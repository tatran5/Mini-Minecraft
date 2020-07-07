#pragma once
#include <QList>
#include <la.h>
#include <drawable.h>
#include <unordered_map>
#include <bitset>
#include <iostream>
#include <qstack.h>
#include <QMutex>


// C++ 11 allows us to define the size of an enum. This lets us use only one byte
// of memory to store our different block types. By default, the size of a C++ enum
// is that of an int (so, usually four bytes). This *does* limit us to only 256 different
// block types, but in the scope of this project we'll never get anywhere near that many.
enum BlockType : unsigned char
{
    EMPTY,
    /*block type with textures starting from B to I*/
    BAMBOO, BEDROCK, DIRT, GRASS, GUM, ICE,
    /*remaining*/
    LAVA, LEAF, SAND, SNOW, STONE, WATER, WOOD
};

class Chunk : public Drawable {
protected:

public:

    bool filled;
    Chunk(OpenGLContext *context);
    ~Chunk();
    std::vector<BlockType> blockArray;
    BlockType blockAt(int x, int y, int z) const;
    BlockType& blockAt(int x, int y, int z);
    Chunk* front;
    Chunk* back;
    Chunk* left;
    Chunk* right;
    bool createdOpaque;



    void fillBetween(int x, int z, int bottomLimit, int topLimit, BlockType t);
    //VBO for the opaque blocks
    std::vector<glm::vec4> interOpaque;
    std::vector<int> idxOpaque;

    //VBO for the non-opaque blocks
    std::vector<glm::vec4> interNonOpaque;
    std::vector<int> idxNonOpaque;

    uint64_t xzCoord;
    glm::vec2 xzCoords;

    void setBlockAt(int x, int y, int z, BlockType b);

    //depthMode = true if we only push in position for depth maps
    //depthMode = false if we push in position, normal, uv, etc.
    void pushBlockIntoVBO(std::vector<glm::vec4>& inter/*VBO*/, std::vector<int>& idx,
                          glm::vec2& uvStartCoord, float& cosPow, float& animatable,
                          int& currX, int& currY, int& currZ,
                          float& norLengthBlock, int &initial,
                          BlockType& blockType);
    void create();
    //    void createForDepthMap(); //only cares about position as vertex attributes
    void bindOpaquePart(); //must be called somewhere after create()
    void bindNonOpaquePart(); //must be called somewhere after create()
    void clearData();
};


class Terrain
{
public:
    Terrain(OpenGLContext *context);
    ~Terrain();

    // You'll need to replace this with a far more
    OpenGLContext* context;                                                   // efficient system of storing terrain.

    QHash <uint64_t, Chunk*> chunkMap;
    QHash<uint64_t, Chunk*> incomplete;


    glm::ivec3 dimensions;
    glm::vec4 m_currBounds;
    // currBounds.x = left bound of terrain
    // currBounds.y = right bound of terrain
    // currBounds.z = bottom bound of terrain
    // currBounds.w = top bound of terrain

    glm::vec3 m_playerInitPos;



    void calcInitPos(float x, float z);
    void createScene(float minX, float maxX, float minZ, float maxZ);
    void fillBetweenNewChunks(int x, int z, int bottomLimit, int topLimit, BlockType t, QHash <uint64_t, Chunk*>& newChunks);

    void fillBetweenList(int x, int z, int bottomLimit, int topLimit, BlockType t);


    BlockType getBlockAt(int x, int y, int z) const;   // Given a world-space coordinate (which may have negative
    // values) return the block stored at that point in space.
    void setBlockAt(int x, int y, int z, BlockType t); // Given a world-space coordinate (which may have negative
    // values) set the block at that point in space to the
    // given type.

    void setBlockAtNewChunks(int x, int y, int z, BlockType t, QHash <uint64_t, Chunk*>& newChunks);
    Chunk* makeChunkAtList(int x, int z, QHash <uint64_t, Chunk*>& newChunks);

    void fillChunk(Chunk& chunk);


    void fillChunkBiome(Chunk& chunk);

    bool chunkExistsAt(int x, int y, int z);
    Chunk* getChunkAt(int x, int z);


    // FBM Terrain generation
    glm::vec2 getChunkCoord(int x, int z);

    void setWorldBounds(glm::vec4 newBounds);

    int findTop(int x, int z);
    //river generation
    float calcHeightTest(int x, int z);

    // multithreading --------------------------------------------------
    QMutex mutex;
    QHash <uint64_t, Chunk*> chunkList;
    void createInitScene(float minX, float maxX, float minZ, float maxZ);

    void setAdjacencies(Chunk* chunk);

    int numElementsAddedWithExpand;

    enum LType : unsigned char
    {
        DELTA, LINEAR, TREE, BAMBOOTREE, PINE
    };

    struct Turtle
    {
        glm::vec4 pos;
        glm::vec4 orientation;
        float depth;
        Turtle();
        Turtle(glm::vec4 pos, glm::vec4 orientation, float depth);
        Turtle(glm::vec4 pos, glm::vec4 orientation, float depth, BlockType material);

        BlockType material;
        //Turtle(Turtle t);



    };

    typedef void(Terrain::*Rule)(LType r, Turtle& currTurt, QStack<Turtle>& turtleStack, QHash <uint64_t, Chunk*>& newChunks);

    QSet<Chunk*> created;

    QSet<Chunk*> recreate;

    QSet<Chunk*> toCreate;

   //QHash <uint64_t, Chunk*> newChunks;

    //QStack<Turtle> turtleStack;
    QHash<QChar, QString> charToExpandedString;
    QHash<QChar, Rule> charToDrawingOperation;
    //Turtle* currTurt;
    void expandString(QString& sentence);
    void setExpandedStrings(LType r);
    void setDrawingRules(LType r);

    void moveTurtleForwardRand(LType r, Turtle& currTurt, QStack<Turtle>& turtleStack, QHash <uint64_t, Chunk*>& newChunks);
    void moveTurtleBackward(LType r, Turtle& currTurt, QStack<Turtle>& turtleStack, QHash <uint64_t, Chunk*>& newChunks);
    void rotateTurtlePos(LType r, Turtle& currTurt, QStack<Turtle>& turtleStack, QHash <uint64_t, Chunk*>& newChunks);
    void rotateTurtleNeg(LType r, Turtle& currTurt, QStack<Turtle>& turtleStack, QHash <uint64_t, Chunk*>& newChunks);
    void rotateTurtleZPos(LType r, Turtle& currTurt, QStack<Turtle>& turtleStack, QHash <uint64_t, Chunk*>& newChunks);
    void rotateTurtleZNeg(LType r, Turtle& currTurt, QStack<Turtle>& turtleStack, QHash <uint64_t, Chunk*>& newChunks);

    void makeTurtleLeaf(LType r, Turtle& currTurt, QStack<Turtle>& turtleStack, QHash <uint64_t, Chunk*>& newChunks);
    void makeTurtleWood(LType r, Turtle& currTurt, QStack<Turtle>& turtleStack, QHash <uint64_t, Chunk*>& newChunks);
    void makeTurtleBamboo(LType r, Turtle& currTurt, QStack<Turtle>& turtleStack, QHash <uint64_t, Chunk*>& newChunks);
    void makeTurtleGrass(LType r, Turtle& currTurt, QStack<Turtle>& turtleStack, QHash <uint64_t, Chunk*>& newChunks);

    static bool isTransparent(BlockType t);

    void rotateTurtleXPos(LType r, Turtle& currTurt, QStack<Turtle>& turtleStack, QHash <uint64_t, Chunk*>& newChunks);
    void rotateTurtleXNeg(LType r, Turtle& currTurt, QStack<Turtle>& turtleStack, QHash <uint64_t, Chunk*>& newChunks);

    void addTurtleDepth(LType r, Turtle& currTurt, QStack<Turtle>& turtleStack, QHash <uint64_t, Chunk*>& newChunks);
    void subTurtleDepth(LType r, Turtle& currTurt, QStack<Turtle>& turtleStack, QHash <uint64_t, Chunk*>& newChunks);
    void makeTrees();

    void makeTreeAtChunk(Chunk* chunk);


    void drawBlockLockedAxis(int x, int lock, int z, BlockType b);
    void moveTurtleUp(LType r, Turtle& currTurt, QStack<Turtle>& turtleStack, QHash <uint64_t, Chunk*>& newChunks);
    void pushTurtle(LType r, Turtle& currTurt, QStack<Turtle>& turtleStack, QHash <uint64_t, Chunk*>& newChunks);
    void popTurtle(LType r, Turtle& currTurt, QStack<Turtle>& turtleStack, QHash <uint64_t, Chunk*>& newChunks);
    static bool isLiquid(BlockType t);
    void bresenhamAlongH(int x1, int x2, int z1, int z2, int y,
                         float thickness, BlockType type,  QHash <uint64_t, Chunk*>& newChunks);
    void bresenhamAlongV(int x1, int x2, int z1, int z2, int y,
                         float thickness, BlockType type,  QHash <uint64_t, Chunk*>& newChunks);

    void drawLine(glm::vec4 v1, glm::vec4 v2, float thickness, BlockType type, QHash<uint64_t, Chunk *> &newChunks);

    void makeRiver(QHash<uint64_t, Chunk *> &newChunks);

    void carve(int x, int z, int min, int radius, float fallOff, QHash<uint64_t, Chunk *> &newChunks);
    glm::vec2 random2(glm::vec2 p);
};


float rand(glm::vec2 n);
float interpNoise2D(float x, float z);
float fbm(float x, float z);
// Gives a smoother interpolation than mix
float cosineInterpolate(float a, float b, float x);
uint64_t xzCoord(int x, int z);
