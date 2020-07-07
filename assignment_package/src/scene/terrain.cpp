#include <scene/terrain.h>
#include <scene/cube.h>
#include <iostream>
#include <string>
#include <QThreadPool>
#include <scene/worker.h>
#include <glm/gtx/random.hpp>


Terrain::Terrain(OpenGLContext* context) : context(context), dimensions(64, 256, 64), numElementsAddedWithExpand(0)
{}

BlockType Terrain::getBlockAt(int x, int y, int z) const
{
    float xCoord = x - x % 16;
    float zCoord = z - z % 16;

    if(x < 0 && x % 16 != 0) {
        xCoord = x - (x % 16 + 16);
    }

    if (z < 0 && z % 16 != 0) {
        zCoord = z - (z % 16 + 16);
    }

    uint64_t xzCoord = (static_cast<uint64_t> (xCoord) << 32) | (static_cast<uint64_t> (zCoord) & 0x00000000FFFFFFFF);
    auto search =  chunkMap.find(xzCoord);
    if (search != chunkMap.end())
    {
        return chunkMap.value(xzCoord)->blockAt(x - xCoord, y, z - zCoord);
    }
    return EMPTY;
}

bool Terrain::chunkExistsAt(int x, int y, int z) {

    float xCoord = x - x % 16;
    float zCoord = z - z % 16;

    if(x < 0 && x % 16 != 0) {
        xCoord = x - (x % 16 + 16);
    }

    if (z < 0 && z % 16 != 0) {
        zCoord = z - (z % 16 + 16);
    }

    uint64_t xzCoord = (static_cast<uint64_t> (xCoord) << 32) | (static_cast<uint64_t> (zCoord) & 0x00000000FFFFFFFF);
    auto search =  chunkMap.find(xzCoord);

    return chunkMap.contains(xzCoord);

}

Chunk* Terrain::getChunkAt(int x, int z) {

    int xCoord = x - x % 16;
    int zCoord = z - z % 16;

    if(x < 0 && x % 16 != 0) {
        xCoord = x - (x % 16 + 16);
    }

    if (z < 0 && z % 16 != 0) {
        zCoord = z - (z % 16 + 16);
    }

    uint64_t xzCoord = (static_cast<uint64_t> (xCoord) << 32) | (static_cast<uint64_t> (zCoord) & 0x00000000FFFFFFFF);
    if (chunkMap.contains(xzCoord))
    {
        return chunkMap.value(xzCoord);
    }
    return nullptr;
}

void Terrain::setBlockAt(int x, int y, int z, BlockType t)
{
    bool usedChunk = false;

    float xCoord = x - x % 16;
    float zCoord = z - z % 16;

    if(x < 0 && x % 16 != 0) {
        xCoord = x - (x % 16 + 16);
    }

    if (z < 0 && z % 16 != 0) {
        zCoord = z - (z % 16 + 16);
    }

    uint64_t xzCoord = (static_cast<uint64_t> (xCoord) << 32) | (static_cast<uint64_t> (zCoord) & 0x00000000FFFFFFFF);
    auto search =  chunkMap.find(xzCoord);

    if (search != chunkMap.end())
    {
        chunkMap.value(xzCoord)->blockAt(x - xCoord, y, z - zCoord) = t;
        usedChunk = true;
        toCreate.insert(chunkMap.value(xzCoord));

    }
}


void Terrain :: fillChunk(Chunk &chunk) {
    for (int x = 0; x < 16; x++) {
        for(int z = 0; z < 16; z++ ) {
            float heightTest = fmin(254, fbm((chunk.xzCoords.x + x) / 160.f, (chunk.xzCoords.y + z) / 160.f) * 32.f + 129.f);
            chunk.fillBetween(x,z,0,128,STONE);
            chunk.fillBetween(x,z,129,heightTest,DIRT);
            chunk.blockAt(x,heightTest + 1, z) = GRASS;
        }
    }
}

void Terrain :: fillChunkBiome(Chunk& chunk) {
    // chunkbounds : x = minX, y = maxX, z = minZ, w = maxZ
    for (int currX = 0; currX < 16; currX++)
    {
        for (int currZ = 0; currZ < 16; currZ++)
        {

            float x = chunk.xzCoords.x + currX;
            float z = chunk.xzCoords.y + currZ;

            chunk.fillBetween(currX, currZ, 0,128,STONE);
            //---------------------------------------
            //BIOME & WORLEY
            //---------------------------------------
            int numBiome = 3;

            float cellSize = 50.f; //each cell spans #cellSize blocks
            //get the cell location that the current block is located within
            int curBlockCellLocX = glm::floor(x / cellSize);
            int curBlockCellLocZ = glm::floor(z / cellSize);
            BlockType curBlockType = BEDROCK; //for debug

            // Want the block to be of the type of the closest "heart"
            // Want to check how close the "heart" is to the block and what type of block it is
            float distToClosestHeart = 999999999.f;
            float distToNextClosestHeart = 999999999.f;
            int modClosestHeartResult = 0; //helps to determine what blocktype of the heart block is
            int modNextClosestHeartResult = 0;
            glm::vec2 locClosestHeart;
            glm::vec2 locNextClosestHeart;

            //iterate throught 9 cells (8 surrounding and itself)
            for (int curCellX = curBlockCellLocX - 1;
                 curCellX <= curBlockCellLocX + 1; curCellX ++) {
                for (int curCellZ = curBlockCellLocZ - 1;
                     curCellZ <= curBlockCellLocZ + 1; curCellZ ++) {
                    //the input to random2 is converted back to block size
                    glm::vec2 heartCurCellLoc =
                            glm::vec2(curCellX * cellSize, curCellZ * cellSize) +
                            cellSize * random2(glm::vec2(curCellX, curCellZ));

                    // Check if the location of this block is closer than
                    // the closest distance from this block to any hearts of cells
                    float distToCurHeart = glm::length(heartCurCellLoc - glm::vec2(x, z));

                    // If the location of this block is closer to this heart than previous hearts,
                    // set the current block to be the same as this heart's block type
                    if (distToCurHeart < distToClosestHeart){
                        modNextClosestHeartResult = modClosestHeartResult;
                        modClosestHeartResult = glm::mod( ((int)heartCurCellLoc.x +
                                                           (int)heartCurCellLoc.y), numBiome);

                        distToNextClosestHeart = distToClosestHeart;
                        distToClosestHeart = distToCurHeart;

                        locNextClosestHeart = locClosestHeart;
                        locClosestHeart = heartCurCellLoc;
                    }

                }
            }

            //avoid negative modulo values
            modClosestHeartResult = glm::abs(modClosestHeartResult);
            modNextClosestHeartResult = glm::abs(modNextClosestHeartResult);

            // Decide block texture ----------------------------------------------------------------------------

            // (modHeartToUse, distToUse, locToUse) corresponds to the same heart that determines the type
            // of biome that the current block belongs to
            // It can be either the closest heart, or the next closest heart
            int modHeartToUse = modClosestHeartResult; // marks the biome that the current block is of
            int modHeartNotToUse = modNextClosestHeartResult;
            float distToUse = distToClosestHeart;
            float distNotToUse = distToNextClosestHeart;
            glm::vec2 locToUse = locClosestHeart;
            glm::vec2 locNotToUse = locNextClosestHeart;

            // To create transition between biomes
            // If distance to closest heart is further away than constant * cellSize, the chance of this
            // block being the same biome as the closest heart's dependes on how far away it is. If
            // it is not the same as the closest heart's,
            if (distToClosestHeart >  0.43 * (distToClosestHeart + distToNextClosestHeart)) {
                // Arbitrary functions to determine which block it should be
                // Returns a value from 0 - 100
                float randVal = rand() % 100;
                if (randVal < 49)  {
                    modHeartToUse = modNextClosestHeartResult;
                    modHeartNotToUse = modClosestHeartResult;
                    distToUse = distToNextClosestHeart;
                    distNotToUse = distToClosestHeart;
                    locToUse = locNextClosestHeart;
                    locNotToUse = locClosestHeart;
                }
            }




            //--------------------------------------------------------------------------
            // From the result of modulo, decides which biomes to belong to;
            if (modHeartToUse == 2) {
                curBlockType = SNOW;
            } else if (modHeartToUse == 1) {
                curBlockType = GRASS;
            } else {
                curBlockType == SAND;
            }

            // --------------------------------------------------------------
            //TRYWORKING
            float heightToUse;
            float heightNotToUse;

            // Find the height of the heart of the biome which this current block is the same type with
            if (modHeartToUse == 2) {
                heightToUse = fmin(254, fbm(locToUse.x / 160.f, locToUse.y / 160.f) * 40.f + 129.f);
            } else if (modHeartToUse == 1) {
                heightToUse = fmin(254, fbm(locToUse.x / 160.f, locToUse.y / 160.f) * 30.f + 129.f);
            } else {
                heightToUse = fmin(254, fbm(locToUse.x / 160.f, locToUse.y / 160.f) * 27.f + 129.f);
            }

            // Find the height of the next closest heart of which this current block is potentially
            // not the same type with
            if (modHeartNotToUse != modHeartToUse) {
                if (modHeartNotToUse == 2) {
                    heightNotToUse = fmin(254, fbm(locNotToUse.x / 160.f,
                                                   locNotToUse.y / 160.f) * 40.f + 129.f);
                } else if (modHeartNotToUse == 1) {
                    heightNotToUse = fmin(254, fbm(locNotToUse.x / 160.f,
                                                   locNotToUse.y / 160.f) * 30.f + 129.f);
                } else {
                    heightNotToUse = fmin(254, fbm(locNotToUse.x / 160.f,
                                                   locNotToUse.y / 160.f) * 27.f + 129.f);
                }
            }

            // Interpolate the height of the block when the two closest hearts are of different biomes type
            float heightCurBlock;

            if (modHeartNotToUse != modHeartToUse &&
                    distToClosestHeart >  0.43 * (distToClosestHeart + distToNextClosestHeart)) {
                heightCurBlock = glm::mix(heightToUse, heightNotToUse, distToUse / (distToUse + distNotToUse));
            } else {
                if (modHeartToUse == 2) {
                    heightCurBlock = fmin(254, fbm(x / 160.f, z / 160.f) * 40.f + 129.f);
                } else if (modHeartToUse  == 1) {
                    heightCurBlock = fmin(254, fbm(x / 160.f, z / 160.f) * 30.f + 129.f);
                } else {
                    heightCurBlock = fmin(254, fbm(x / 160.f, z / 160.f) * 27.f + 129.f);
                }
            }

            if (curBlockType == GRASS) {
                chunk.fillBetween(currX, currZ, 129, heightCurBlock, DIRT);
            } else if (curBlockType == SAND) {
                chunk.fillBetween(currX, currZ, 129, heightCurBlock, SAND);
            } else {
                chunk.fillBetween(currX, currZ, 129, heightCurBlock, BEDROCK);
            }

            chunk.blockAt(currX, heightCurBlock + 1, currZ) =  curBlockType; //----biome

        }
    }

}

Chunk* Terrain :: makeChunkAtList(int x, int z, QHash <uint64_t, Chunk*>& newChunks) {
    bool usedChunk = false;

    float xCoord = x - x % 16;
    float zCoord = z - z % 16;

    if(x < 0 && x % 16 != 0) {
        xCoord = x - (x % 16 + 16);
    }

    if (z < 0 && z % 16 != 0) {
        zCoord = z - (z % 16 + 16);
    }

    uint64_t xzCoord = (static_cast<uint64_t> (xCoord) << 32) | (static_cast<uint64_t> (zCoord) & 0x00000000FFFFFFFF);
    auto search =  chunkMap.find(xzCoord);

    if (chunkMap.contains(xzCoord) || chunkList.contains(xzCoord))
    {
        return nullptr;
    }



    Chunk* newChunk = new Chunk(context);
    //take mod of 16 to determine nearest multiple of 16, coordinate at which to create new chunk
    newChunk->xzCoords = glm::vec2(xCoord, zCoord);
    newChunk->xzCoord = xzCoord;
    //newChunk->blockAt(x - xCoord, y, z - zCoord) = t;


    fillChunkBiome(*newChunk);

    newChunks.insert(xzCoord, newChunk);
    // adding to chunk map
    //recreate.insert(newChunk);

    return newChunk;
}


void Terrain :: setBlockAtNewChunks(int x, int y, int z, BlockType t, QHash<uint64_t, Chunk *> &newChunks) {
    //    std::cout << "trying to set block at" << x << ", " << y << ", " << z << "\n";

    bool usedChunk = false;

    int xCoord = x - x % 16;
    int zCoord = z - z % 16;

    if(x < 0 && x % 16 != 0) {
        xCoord = x - (x % 16 + 16);
    }

    if (z < 0 && z % 16 != 0) {
        zCoord = z - (z % 16 + 16);
    }

    uint64_t xzCoord = (static_cast<uint64_t> (xCoord) << 32) | (static_cast<uint64_t> (zCoord) & 0x00000000FFFFFFFF);

    if(newChunks.contains(xzCoord)) {
        newChunks.value(xzCoord)->blockAt(x - xCoord, y, z - zCoord) = t;
        //toCreate.insert(newChunks.value(xzCoord));

    }


}



Terrain::~Terrain() {
    for(auto p =  chunkMap.begin(); p !=  chunkMap.end(); p++) {
        delete p.value();
    }
}

void Terrain::fillBetweenNewChunks(int x, int z, int bottomLimit, int topLimit, BlockType t, QHash <uint64_t, Chunk*>& newChunks)
{

    float xCoord = x - x % 16;
    float zCoord = z - z % 16;

    if(x < 0 && x % 16 != 0) {
        xCoord = x - (x % 16 + 16);
    }

    if (z < 0 && z % 16 != 0) {
        zCoord = z - (z % 16 + 16);
    }


    //    uint64_t xzCoord = (static_cast<uint64_t> (xCoord) << 32) | (static_cast<uint64_t> (zCoord) & 0x00000000FFFFFFFF);
    //    if(!chunkMap.contains(xzCoord)) {
    //        return;
    //    }
    for (int i = bottomLimit; i <= topLimit; ++i)
    {
        setBlockAtNewChunks(x, i, z, t, newChunks);
    }
}



void Terrain::fillBetweenList(int x, int z, int bottomLimit, int topLimit, BlockType t)
{
    for (int i = bottomLimit; i <= topLimit; ++i)
    {
        // setBlockAtNewChunks(x, i, z, t);
    }
}
uint64_t xzCoord(int x, int z) {


    float xCoord = x - x % 16;
    float zCoord = z - z % 16;

    if(x < 0 && x % 16 != 0) {
        xCoord = x - (x % 16 + 16);
    }

    if (z < 0 && z % 16 != 0) {
        zCoord = z - (z % 16 + 16);
    }


    return (static_cast<uint64_t> (xCoord) << 32) | (static_cast<uint64_t> (zCoord) & 0x00000000FFFFFFFF);

}


// Fractal Brownian Motion for random height field generation
float rand(glm::vec2 n) {
    return (glm::fract(sin(glm::dot(n + glm::vec2(91.2328, -19.8232), glm::vec2(12.9898, 4.1414))) * 43758.5453));
}

float cosineInterpolate(float a, float b, float x)
{
    float ft = x * 3.1415927;
    float f = (1 - cos(ft)) * .5;

    return a*(1-f) + b*f;
}

float interpNoise2D(float x, float z)
{
    float intX = floor(x);
    float fractX = glm::fract(x);
    float intZ = floor(z);
    float fractZ = glm::fract(z);

    float v1 = rand(glm::vec2(intX, intZ));
    float v2 = rand(glm::vec2(intX + 1, intZ));
    float v3 = rand(glm::vec2(intX, intZ + 1));
    float v4 = rand(glm::vec2(intX + 1, intZ + 1));

    float i1 = cosineInterpolate(v1, v2, fractX);
    float i2 = cosineInterpolate(v3, v4, fractX);
    return cosineInterpolate(i1, i2, fractZ);
}

float fbm(float x, float z)
{
    float total = 0;
    float persistence = 0.55f;
    int octaves = 8;

    for(int i = 1 ; i <= octaves; i++)
    {
        float freq = pow(2.f, i);
        float amp = pow(persistence, i);

        total += interpNoise2D(x * freq,
                               z * freq) * amp;
    }
    return total;
}

void Terrain::setWorldBounds(glm::vec4 newBounds)
{
    m_currBounds = newBounds;
}

glm::vec2 Terrain::random2(glm::vec2 p) {
    return glm::fract(43758.5453f * glm::sin(glm::vec2(glm::dot(p, glm::vec2(127.1, 311.7)),
                                                       glm::dot(p, glm::vec2(269.5, 183.3)))));
}

void Terrain::calcInitPos(float x, float z)  {
    float heightTest = fmin(254, fbm(x / 160.f, z / 160.f) * 32.f + 129.f);
    m_playerInitPos = glm::vec3(28, heightTest + 3.f, 32);

}

void Terrain::createInitScene(float minX, float maxX, float minZ, float maxZ)
{
    m_currBounds = glm::vec4(minX, maxX, minZ, maxZ);

    // Create the basic terrain floor
    for(int x = minX; x < maxX; ++x)
    {
        for(int z = minZ; z < maxZ; ++z)
        {
            for(int y = 0; y <= 255; ++y)
            {
                if (y <= 128)
                {
                    setBlockAt(x, y, z, STONE);
                }
                else if (y > 128 & y <= 255)
                {
                    //---------------------------------------
                    //BIOME & WORLEY
                    //---------------------------------------
                    int numBiome = 3;

                    float cellSize = 50.f; //each cell spans #cellSize blocks
                    //get the cell location that the current block is located within
                    int curBlockCellLocX = glm::floor(x / cellSize);
                    int curBlockCellLocZ = glm::floor(z / cellSize);
                    BlockType curBlockType = BEDROCK; //for debug

                    // Want the block to be of the type of the closest "heart"
                    // Want to check how close the "heart" is to the block and what type of block it is
                    float distToClosestHeart = 999999999.f;
                    float distToNextClosestHeart = 999999999.f;
                    int modClosestHeartResult = 0; //helps to determine what blocktype of the heart block is
                    int modNextClosestHeartResult = 0;
                    glm::vec2 locClosestHeart;
                    glm::vec2 locNextClosestHeart;

                    //iterate throught 9 cells (8 surrounding and itself)
                    for (int curCellX = curBlockCellLocX - 1;
                         curCellX <= curBlockCellLocX + 1; curCellX ++) {
                        for (int curCellZ = curBlockCellLocZ - 1;
                             curCellZ <= curBlockCellLocZ + 1; curCellZ ++) {
                            //the input to random2 is converted back to block size
                            glm::vec2 heartCurCellLoc =
                                    glm::vec2(curCellX * cellSize, curCellZ * cellSize) +
                                    cellSize * random2(glm::vec2(curCellX, curCellZ));

                            // Check if the location of this block is closer than
                            // the closest distance from this block to any hearts of cells
                            float distToCurHeart = glm::length(heartCurCellLoc - glm::vec2(x, z));

                            // If the location of this block is closer to this heart than previous hearts,
                            // set the current block to be the same as this heart's block type
                            if (distToCurHeart < distToClosestHeart){
                                modNextClosestHeartResult = modClosestHeartResult;
                                modClosestHeartResult = glm::mod( ((int)heartCurCellLoc.x +
                                                                   (int)heartCurCellLoc.y), numBiome);

                                distToNextClosestHeart = distToClosestHeart;
                                distToClosestHeart = distToCurHeart;

                                locNextClosestHeart = locClosestHeart;
                                locClosestHeart = heartCurCellLoc;
                            }

                        }
                    }

                    //avoid negative modulo values
                    modClosestHeartResult = glm::abs(modClosestHeartResult);
                    modNextClosestHeartResult = glm::abs(modNextClosestHeartResult);

                    // Decide block texture ----------------------------------------------------------------------------

                    // (modHeartToUse, distToUse, locToUse) corresponds to the same heart that determines the type
                    // of biome that the current block belongs to
                    // It can be either the closest heart, or the next closest heart
                    int modHeartToUse = modClosestHeartResult; // marks the biome that the current block is of
                    int modHeartNotToUse = modNextClosestHeartResult;
                    float distToUse = distToClosestHeart;
                    float distNotToUse = distToNextClosestHeart;
                    glm::vec2 locToUse = locClosestHeart;
                    glm::vec2 locNotToUse = locNextClosestHeart;

                    // To create transition between biomes
                    // If distance to closest heart is further away than constant * cellSize, the chance of this
                    // block being the same biome as the closest heart's dependes on how far away it is. If
                    // it is not the same as the closest heart's,
                    if (distToClosestHeart >  0.43 * (distToClosestHeart + distToNextClosestHeart)) {
                        // Arbitrary functions to determine which block it should be
                        // Returns a value from 0 - 100
                        float randVal = rand() % 100;
                        if (randVal < 49)  {
                            modHeartToUse = modNextClosestHeartResult;
                            modHeartNotToUse = modClosestHeartResult;
                            distToUse = distToNextClosestHeart;
                            distNotToUse = distToClosestHeart;
                            locToUse = locNextClosestHeart;
                            locNotToUse = locClosestHeart;
                        }
                    }




                    //--------------------------------------------------------------------------
                    // From the result of modulo, decides which biomes to belong to;
                    if (modHeartToUse == 2) {
                        curBlockType = SNOW;
                    } else if (modHeartToUse == 1) {
                        curBlockType = GRASS;
                    } else {
                        curBlockType == SAND;
                    }

                    // --------------------------------------------------------------
                    //TRYWORKING
                    float heightToUse;
                    float heightNotToUse;

                    // Find the height of the heart of the biome which this current block is the same type with
                    if (modHeartToUse == 2) {
                        heightToUse = fmin(254, fbm(locToUse.x / 160.f, locToUse.y / 160.f) * 40.f + 129.f);
                    } else if (modHeartToUse == 1) {
                        heightToUse = fmin(254, fbm(locToUse.x / 160.f, locToUse.y / 160.f) * 30.f + 129.f);
                    } else {
                        heightToUse = fmin(254, fbm(locToUse.x / 160.f, locToUse.y / 160.f) * 27.f + 129.f);
                    }

                    // Find the height of the next closest heart of which this current block is potentially
                    // not the same type with
                    if (modHeartNotToUse != modHeartToUse) {
                        if (modHeartNotToUse == 2) {
                            heightNotToUse = fmin(254, fbm(locNotToUse.x / 160.f,
                                                           locNotToUse.y / 160.f) * 40.f + 129.f);
                        } else if (modHeartNotToUse == 1) {
                            heightNotToUse = fmin(254, fbm(locNotToUse.x / 160.f,
                                                           locNotToUse.y / 160.f) * 30.f + 129.f);
                        } else {
                            heightNotToUse = fmin(254, fbm(locNotToUse.x / 160.f,
                                                           locNotToUse.y / 160.f) * 27.f + 129.f);
                        }
                    }

                    // Interpolate the height of the block when the two closest hearts are of different biomes type
                    float heightCurBlock;

                    if (modHeartNotToUse != modHeartToUse &&
                            distToClosestHeart >  0.43 * (distToClosestHeart + distToNextClosestHeart)) {
                        heightCurBlock = glm::mix(heightToUse, heightNotToUse, distToUse / (distToUse + distNotToUse));
                    } else {
                        if (modHeartToUse == 2) {
                            heightCurBlock = fmin(254, fbm(x / 160.f, z / 160.f) * 40.f + 129.f);
                        } else if (modHeartToUse  == 1) {
                            heightCurBlock = fmin(254, fbm(x / 160.f, z / 160.f) * 30.f + 129.f);
                        } else {
                            heightCurBlock = fmin(254, fbm(x / 160.f, z / 160.f) * 27.f + 129.f);
                        }
                    }

                    if (curBlockType == GRASS) {
                        //fillBetween(x, z, 129, heightCurBlock, DIRT);
                    } else if (curBlockType == SAND) {
                        //fillBetween(x, z, 129, heightCurBlock, SAND);
                    } else {
                        //fillBetween(x, z, 129, heightCurBlock, BEDROCK);
                    }

                    setBlockAt(x, heightCurBlock + 1, z, curBlockType); //----biome

                    if (x == 28 && z == 32)
                    {
                        m_playerInitPos = glm::vec3(28, heightCurBlock + 5.f, 32);
                    }

                    break;
                }
            }
        }
    }
    //makeRiver();
    makeTrees();
}

void Terrain::createScene(float minX, float maxX, float minZ, float maxZ)
{
    Worker* thread = new Worker(&mutex, glm::vec4(minX, maxX, minZ, maxZ), this);
    QThreadPool::globalInstance()->start(thread);



}


void Terrain::setAdjacencies(Chunk* chunk)
{
    float zBack = chunk->xzCoords.y - 16;
    float xBack = chunk->xzCoords.x;
    uint64_t xzCoordBack = (static_cast<uint64_t> (xBack) << 32) | (static_cast<uint64_t> (zBack) & 0x00000000FFFFFFFF);
    // std::unordered_map<uint64_t, Chunk*>::iterator back = chunkMap.find(xzCoordBack);
    if((chunkMap.contains(xzCoordBack)))
    {
        chunk->back = chunkMap.value(xzCoordBack);
        chunkMap.value(xzCoordBack)->front = chunk;

    }

    float zFront = chunk->xzCoords.y + 16;
    float xFront = chunk->xzCoords.x;
    uint64_t xzCoordFront = (static_cast<uint64_t> (xFront) << 32) | (static_cast<uint64_t> (zFront) & 0x00000000FFFFFFFF);
    //std::unordered_map<uint64_t, Chunk*>::iterator front = chunkMap.find(xzCoordFront);
    if((chunkMap.contains(xzCoordFront)))
    {
        chunk->front = chunkMap.value(xzCoordFront);
        chunkMap.value(xzCoordFront)->back = chunk;

    }

    float zLeft = chunk->xzCoords.y;
    float xLeft = chunk->xzCoords.x - 16;
    uint64_t xzCoordLeft = (static_cast<uint64_t> (xLeft) << 32) | (static_cast<uint64_t> (zLeft) & 0x00000000FFFFFFFF);
    // std::unordered_map<uint64_t, Chunk*>::iterator left = chunkMap.find(xzCoordLeft);
    if(chunkMap.contains(xzCoordLeft))
    {
        chunk->left = chunkMap.value(xzCoordLeft);
        chunkMap.value(xzCoordLeft)->right = chunk;

    }

    float zRight = chunk->xzCoords.y;
    float xRight = chunk->xzCoords.x + 16;
    uint64_t xzCoordRight = (static_cast<uint64_t> (xRight) << 32) | (static_cast<uint64_t> (zRight) & 0x00000000FFFFFFFF);
    // std::unordered_map<uint64_t, Chunk*>::iterator right = chunkMap.find(xzCoordRight);
    if(chunkMap.contains(xzCoordRight))
    {
        chunk->right = chunkMap.value(xzCoordRight);
        chunkMap.value(xzCoordRight)->left = chunk;

    }

}



Chunk::Chunk(OpenGLContext* context) : Drawable(context), blockArray(65536),
    front(nullptr), back(nullptr), left(nullptr), right(nullptr),
    createdOpaque(false)

{}

Chunk:: ~Chunk() {}

BlockType& Chunk::blockAt(int x, int y, int z) {
    return blockArray[x + 16 * y + 256 * 16 * z];
}

BlockType Chunk::blockAt(int x, int y, int z) const {
    return blockArray[x + 16 * y + 256 * 16 * z];
}


void Chunk::fillBetween(int x, int z, int bottomLimit, int topLimit, BlockType t) {
    if(x < 0 || x > 15 || z < 0 || z > 15) {
        return;
    }

    for (int i = bottomLimit; i <= topLimit; ++i)
    {
        blockAt(x, i, z) = t;
    }


}



void createSquareIndices(int& initial, std::vector<int>& idx) {
    for(int i = 0; i + 2 < 4; i++) {
        idx.push_back(initial);
        idx.push_back(initial + i + 1);
        idx.push_back(initial + i + 2);
    }
    initial += 4;
}


bool Terrain::isTransparent(BlockType t) {
    return (t == WATER || t == LAVA);
}


//void Chunk::pushBlockIntoVBO(std::vector<glm::vec4>& inter, std::vector<int>& idx,
//                             glm::vec2& uvStartCoord, float& cosPow, float& animatable,
//                             int& currX, int& currY, int& currZ,
//                             float& norLengthBlock, int& initial,
//                             BlockType& blockType, bool depthMode)
//{
//    glm::vec2 topUVStartCoord;
//    glm::vec2 botUVStartCoord;

//    if (!depthMode) {
//        // For certain block types, the starting UV coordinates for top and bottom are different
//        // Switch from uv coordinate to block coordinate
//        topUVStartCoord = uvStartCoord / norLengthBlock ;
//        botUVStartCoord = uvStartCoord / norLengthBlock;
//        if (blockType == GRASS) {
//            topUVStartCoord = glm::vec2(8, 2);
//            botUVStartCoord = glm::vec2(2, 0);
//        } else if (blockType == WOOD) {
//            topUVStartCoord = glm::vec2(5, 1);
//            botUVStartCoord = glm::vec2(5, 1);
//        }
//        // Switch from block coordinate to uv coordinate
//        topUVStartCoord *= norLengthBlock;
//        botUVStartCoord *= norLengthBlock;
//    }

//    //if it is not water and adjacent to water
//    bool opaqueAdjTrans = true;
//    bool opaque = true;
//    if(Terrain::isTransparent(blockAt(currX,currY,currZ))) {
//        opaque = false;
//    }
//    bool adjEmpty = false;


//    if(currX == 15) {
//        adjEmpty = false;

//        if (right != nullptr) {
//            adjEmpty = (right -> blockAt(0,currY,currZ) == EMPTY);
//            opaqueAdjTrans = Terrain::isTransparent(right -> blockAt(0,currY,currZ)) && opaque;
//        } else {
//            adjEmpty = true;
//        }
//    } else {
//        adjEmpty = (blockArray.at((currX + 1) + 16 * currY + 16 * 256 * currZ) == EMPTY);
//        opaqueAdjTrans = Terrain::isTransparent(blockArray.at((currX + 1) + 16 * currY + 16 * 256 * currZ)) && opaque;
//    }


//    // face right of the block------------------------------------------------------------

//    if (adjEmpty || opaqueAdjTrans) {

//        inter.push_back(glm::vec4(float(currX + 1), float(currY), float(currZ), 1.0f));
//        if (!depthMode) {
//            inter.push_back(glm::vec4(1,0,0,0));
//            inter.push_back(glm::vec4(uvStartCoord.x, uvStartCoord.y + norLengthBlock,
//                                      cosPow, animatable));
//        }

//        inter.push_back(glm::vec4(float(currX + 1), float(currY + 1), float(currZ), 1.0f));
//        if (!depthMode) {
//            inter.push_back(glm::vec4(1,0,0,0));
//            inter.push_back(glm::vec4(uvStartCoord.x, uvStartCoord.y, cosPow, animatable));
//        }

//        inter.push_back(glm::vec4(float(currX + 1), float(currY + 1), float(currZ + 1), 1.0f));
//        if (!depthMode) {
//            inter.push_back(glm::vec4(1,0,0,0));
//            inter.push_back(glm::vec4(uvStartCoord.x + norLengthBlock, uvStartCoord.y,
//                                      cosPow, animatable));
//        }

//        inter.push_back(glm::vec4(float(currX + 1), float(currY), float(currZ + 1), 1.0f));
//        if (!depthMode) {
//            inter.push_back(glm::vec4(1,0,0,0));
//            inter.push_back(glm::vec4(uvStartCoord.x + norLengthBlock, uvStartCoord.y + norLengthBlock,
//                                      cosPow, animatable));
//        }
//        createSquareIndices(initial, idx);
//        adjEmpty = false;
//    }

//    if(currX == 0) {
//        adjEmpty = false;
//        if (left != nullptr) {
//            adjEmpty = (left->blockAt(15,currY,currZ) == EMPTY);
//            opaqueAdjTrans = Terrain::isTransparent(left->blockAt(15,currY,currZ)) && opaque;

//        } else {
//            adjEmpty = true;
//        }
//    } else {
//        adjEmpty = (blockArray.at((currX - 1) + 16 * currY + 256 * 16 * currZ) == EMPTY);
//        opaqueAdjTrans = Terrain::isTransparent(blockArray.at((currX - 1) + 16 * currY + 256 * 16 * currZ)) && opaque;


//    }

//    // face left of the block------------------------------------------------------------

//    if(adjEmpty || opaqueAdjTrans) {
//        inter.push_back(glm::vec4(float(currX), float(currY), float(currZ), 1.0f));
//        if (!depthMode) {
//            inter.push_back(glm::vec4(-1,0,0,0));
//            inter.push_back(glm::vec4(uvStartCoord.x,
//                                      uvStartCoord.y + norLengthBlock,
//                                      cosPow,
//                                      animatable));
//        }

//        inter.push_back(glm::vec4(float(currX), float(currY + 1), float(currZ), 1.0f));
//        if (!depthMode) {
//            inter.push_back(glm::vec4(-1,0,0,0));
//            inter.push_back(glm::vec4(uvStartCoord.x,
//                                      uvStartCoord.y,
//                                      cosPow,
//                                      animatable));
//        }

//        inter.push_back(glm::vec4(float(currX), float(currY + 1), float(currZ + 1), 1.0f));
//        if (!depthMode) {
//            inter.push_back(glm::vec4(-1,0,0,0));
//            inter.push_back(glm::vec4(uvStartCoord.x + norLengthBlock,
//                                      uvStartCoord.y,
//                                      cosPow,
//                                      animatable));
//        }

//        inter.push_back(glm::vec4(float(currX), float(currY), float(currZ + 1), 1.0f));
//        if (!depthMode) {
//            inter.push_back(glm::vec4(-1,0,0,0));
//            inter.push_back(glm::vec4(uvStartCoord.x + norLengthBlock,
//                                      uvStartCoord.y + norLengthBlock,
//                                      cosPow,
//                                      animatable));
//        }

//        createSquareIndices(initial, idx);
//        adjEmpty = false;
//    }

//    if(currY == 255) {
//        adjEmpty = true;

//    } else {
//        adjEmpty = (blockArray.at(currX + 16 * (currY + 1) + 256 * 16 * currZ) == EMPTY);
//        opaqueAdjTrans = Terrain::isTransparent(blockArray.at(currX + 16 * (currY + 1) + 256 * 16 * currZ)) && opaque;

//    }

//    // face on top of the block------------------------------------------------------------
//    if (adjEmpty || opaqueAdjTrans) {
//        //don't need to check chunk adjacency since there are no chunks above
//        inter.push_back(glm::vec4(float(currX), float(currY + 1), float(currZ), 1.0f));
//        if (!depthMode) {
//            inter.push_back(glm::vec4(0,1,0,0));
//            inter.push_back(glm::vec4(topUVStartCoord.x, topUVStartCoord.y,
//                                      cosPow, animatable));
//        }

//        inter.push_back(glm::vec4(float(currX + 1), float(currY + 1), float(currZ), 1.0f));
//        if (!depthMode) {
//            inter.push_back(glm::vec4(0,1,0,0));
//            inter.push_back(glm::vec4(topUVStartCoord.x + norLengthBlock, topUVStartCoord.y,
//                                      cosPow, animatable));
//        }

//        inter.push_back(glm::vec4(float(currX + 1), float(currY + 1), float(currZ + 1), 1.0f));
//        if (!depthMode) {
//            inter.push_back(glm::vec4(0,1,0,0));
//            inter.push_back(glm::vec4(topUVStartCoord.x + norLengthBlock, topUVStartCoord.y + norLengthBlock,
//                                      cosPow, animatable));
//        }

//        inter.push_back(glm::vec4(float(currX), float(currY + 1), float(currZ + 1), 1.0f));
//        if (!depthMode) {
//            inter.push_back(glm::vec4(0,1,0,0));
//            inter.push_back(glm::vec4(topUVStartCoord.x, topUVStartCoord.y + norLengthBlock,
//                                      cosPow, animatable));
//        }

//        createSquareIndices(initial, idx);
//        adjEmpty = false;
//    }

//    if(currY == 0) {
//        adjEmpty = true;
//    } else {
//        adjEmpty = (blockArray.at(currX + 16 * (currY - 1) + 256 * 16 * currZ) == EMPTY);
//        opaqueAdjTrans = Terrain::isTransparent(blockArray.at(currX + 16 * (currY - 1) + 256 * 16 * currZ)) && opaque;

//    }


//    // face at bottom of the block------------------------------------------------------------
//    if (adjEmpty || opaqueAdjTrans) {
//        //don't need to check chunk adjacency since there are no chunks below
//        inter.push_back(glm::vec4(float(currX), float(currY), float(currZ), 1));
//        if (!depthMode) {
//            inter.push_back(glm::vec4(0,-1,0,0));
//            inter.push_back(glm::vec4(botUVStartCoord.x, botUVStartCoord.y,
//                                      cosPow, animatable));
//        }

//        inter.push_back(glm::vec4(float(currX + 1), float(currY), float(currZ), 1));
//        if (!depthMode) {
//            inter.push_back(glm::vec4(0,-1,0,0));
//            inter.push_back(glm::vec4(botUVStartCoord.x + norLengthBlock, botUVStartCoord.y,
//                                      cosPow, animatable));
//        }

//        inter.push_back(glm::vec4(float(currX + 1), float(currY), float(currZ + 1), 1));
//        if (!depthMode) {
//            inter.push_back(glm::vec4(0,-1,0,0));
//            inter.push_back(glm::vec4(botUVStartCoord.x + norLengthBlock, botUVStartCoord.y + norLengthBlock,
//                                      cosPow, animatable));
//        }

//        inter.push_back(glm::vec4(float(currX), float(currY), float(currZ + 1), 1));
//        if (!depthMode) {
//            inter.push_back(glm::vec4(0,-1,0,0));
//            inter.push_back(glm::vec4(botUVStartCoord.x, botUVStartCoord.y + norLengthBlock,
//                                      cosPow, animatable));
//        }

//        createSquareIndices(initial, idx);
//        adjEmpty = false;
//    }

//    if(currZ == 0) {
//        adjEmpty = false;
//        if (back != nullptr) {
//            adjEmpty = (back->blockAt(currX,currY,15) == EMPTY);
//            opaqueAdjTrans = Terrain::isTransparent(back->blockAt(currX,currY,15)) && opaque;

//        } else {
//            adjEmpty = true;
//        }

//    } else {
//        adjEmpty = (blockArray.at(currX + 16 * currY + 256 * 16 * (currZ - 1)) == EMPTY);
//        opaqueAdjTrans = Terrain::isTransparent(blockArray.at(currX + 16 * currY + 256 * 16 * (currZ - 1))) && opaque;

//    }

//    // face behind block------------------------------------------------------------
//    if(adjEmpty || opaqueAdjTrans) {
//        inter.push_back(glm::vec4(float(currX), float(currY), float(currZ), 1));
//        if (!depthMode) {
//            inter.push_back(glm::vec4(0,0,1,0));
//            inter.push_back(glm::vec4(uvStartCoord.x + norLengthBlock, uvStartCoord.y + norLengthBlock,
//                                      cosPow, animatable));
//        }

//        inter.push_back(glm::vec4(float(currX + 1), float(currY), float(currZ), 1));
//        if (!depthMode) {
//            inter.push_back(glm::vec4(0,0,1,0));
//            inter.push_back(glm::vec4(uvStartCoord.x, uvStartCoord.y + norLengthBlock,
//                                      cosPow, animatable));
//        }

//        inter.push_back(glm::vec4(float(currX + 1), float(currY + 1), float(currZ), 1));
//        if (!depthMode) {
//            inter.push_back(glm::vec4(0,0,1,0));
//            inter.push_back(glm::vec4(uvStartCoord.x, uvStartCoord.y, cosPow, animatable));
//        }

//        inter.push_back(glm::vec4(float(currX), float(currY + 1), float(currZ), 1));
//        if (!depthMode) {
//            inter.push_back(glm::vec4(0,0,1,0));
//            inter.push_back(glm::vec4(uvStartCoord.x + norLengthBlock, uvStartCoord.y,
//                                      cosPow, animatable));
//        }

//        createSquareIndices(initial, idx);
//        adjEmpty = false;
//    }

//    if(currZ == 15) {
//        adjEmpty = false;
//        if (front != nullptr) {
//            adjEmpty = (front->blockAt(currX, currY, 0) == EMPTY);
//            opaqueAdjTrans = Terrain::isTransparent(front->blockAt(currX,currY,0)) && opaque;

//        } else {
//            adjEmpty = true;
//        }

//    } else {
//        adjEmpty = (blockArray.at(currX + 16 * currY + 256 * 16 * (currZ + 1)) == EMPTY);
//        opaqueAdjTrans = Terrain::isTransparent(blockArray.at(currX + 16 * currY + 256 * 16 * (currZ + 1))) && opaque;

//    }

//    // face in front of block------------------------------------------------------------

//    if(adjEmpty || opaqueAdjTrans) {

//        inter.push_back(glm::vec4(float(currX), float(currY), float(currZ + 1), 1));
//        if (!depthMode) {
//            inter.push_back(glm::vec4(0,0,-1,0));
//            inter.push_back(glm::vec4(uvStartCoord.x + norLengthBlock, uvStartCoord.y + norLengthBlock,
//                                      cosPow, animatable));
//        }

//        inter.push_back(glm::vec4(float(currX + 1), float(currY), float(currZ + 1), 1));
//        if (!depthMode) {
//            inter.push_back(glm::vec4(0,0,-1,0));
//            inter.push_back(glm::vec4(uvStartCoord.x, uvStartCoord.y + norLengthBlock,
//                                      cosPow, animatable));
//        }

//        inter.push_back(glm::vec4(float(currX + 1), float(currY + 1), float(currZ + 1), 1));
//        if (!depthMode) {
//            inter.push_back(glm::vec4(0,0,-1,0));
//            inter.push_back(glm::vec4(uvStartCoord.x, uvStartCoord.y, cosPow, animatable));
//        }

//        inter.push_back(glm::vec4(float(currX), float(currY + 1), float(currZ + 1), 1));
//        if (!depthMode) {
//            inter.push_back(glm::vec4(0,0,-1,0));
//            inter.push_back(glm::vec4(uvStartCoord.x + norLengthBlock, uvStartCoord.y,
//                                      cosPow, animatable));
//        }

//        createSquareIndices(initial, idx);
//        adjEmpty = false;
//    }
//}

void Chunk::pushBlockIntoVBO(std::vector<glm::vec4>& inter, std::vector<int>& idx,
                             glm::vec2& uvStartCoord, float& cosPow, float& animatable,
                             int& currX, int& currY, int& currZ,
                             float& norLengthBlock, int& initial, BlockType& blockType)
{
    // For certain block types, the starting UV coordinates for top and bottom are different
    // Switch from uv coordinate to block coordinate
    glm::vec2 topUVStartCoord = uvStartCoord / norLengthBlock ;
    glm::vec2 botUVStartCoord = uvStartCoord / norLengthBlock;
    if (blockType == GRASS) {
        topUVStartCoord = glm::vec2(8, 2);
        botUVStartCoord = glm::vec2(2, 0);
    } else if (blockType == WOOD) {
        topUVStartCoord = glm::vec2(5, 1);
        botUVStartCoord = glm::vec2(5, 1);
    }
    // Switch from block coordinate to uv coordinate
    topUVStartCoord *= norLengthBlock;
    botUVStartCoord *= norLengthBlock;

    //if it is not water and adjacent to water
    bool opaqueAdjTrans = true;
    bool opaque = true;
    if(Terrain::isTransparent(blockAt(currX,currY,currZ))) {
        opaque = false;
    }
    bool adjEmpty = false;


    if(currX == 15) {
        adjEmpty = false;

        if (right != nullptr) {
            adjEmpty = (right -> blockAt(0,currY,currZ) == EMPTY);
            opaqueAdjTrans = Terrain::isTransparent(right -> blockAt(0,currY,currZ)) && opaque;
        } else {
            adjEmpty = true;
        }
    } else {
        adjEmpty = (blockArray.at((currX + 1) + 16 * currY + 16 * 256 * currZ) == EMPTY);
        opaqueAdjTrans = Terrain::isTransparent(blockArray.at((currX + 1) + 16 * currY + 16 * 256 * currZ)) && opaque;


    }


    // face right of the block------------------------------------------------------------

    if (adjEmpty || opaqueAdjTrans) {

        inter.push_back(glm::vec4(float(currX + 1), float(currY), float(currZ), 1.0f));
        inter.push_back(glm::vec4(1,0,0,0));
        inter.push_back(glm::vec4(uvStartCoord.x, uvStartCoord.y + norLengthBlock,
                                  cosPow, animatable));

        inter.push_back(glm::vec4(float(currX + 1), float(currY + 1), float(currZ), 1.0f));
        inter.push_back(glm::vec4(1,0,0,0));
        inter.push_back(glm::vec4(uvStartCoord.x, uvStartCoord.y, cosPow, animatable));

        inter.push_back(glm::vec4(float(currX + 1), float(currY + 1), float(currZ + 1), 1.0f));
        inter.push_back(glm::vec4(1,0,0,0));
        inter.push_back(glm::vec4(uvStartCoord.x + norLengthBlock, uvStartCoord.y,
                                  cosPow, animatable));

        inter.push_back(glm::vec4(float(currX + 1), float(currY), float(currZ + 1), 1.0f));
        inter.push_back(glm::vec4(1,0,0,0));
        inter.push_back(glm::vec4(uvStartCoord.x + norLengthBlock, uvStartCoord.y + norLengthBlock,
                                  cosPow, animatable));

        createSquareIndices(initial, idx);
        adjEmpty = false;
    }

    if(currX == 0) {
        adjEmpty = false;
        if (left != nullptr) {
            adjEmpty = (left->blockAt(15,currY,currZ) == EMPTY);
            opaqueAdjTrans = Terrain::isTransparent(left->blockAt(15,currY,currZ)) && opaque;

        } else {
            adjEmpty = true;
        }
    } else {
        adjEmpty = (blockArray.at((currX - 1) + 16 * currY + 256 * 16 * currZ) == EMPTY);
        opaqueAdjTrans = Terrain::isTransparent(blockArray.at((currX - 1) + 16 * currY + 256 * 16 * currZ)) && opaque;


    }

    // face left of the block------------------------------------------------------------

    if(adjEmpty || opaqueAdjTrans) {
        inter.push_back(glm::vec4(float(currX), float(currY), float(currZ), 1.0f));
        inter.push_back(glm::vec4(-1,0,0,0));
        inter.push_back(glm::vec4(uvStartCoord.x,
                                  uvStartCoord.y + norLengthBlock,
                                  cosPow,
                                  animatable));

        inter.push_back(glm::vec4(float(currX), float(currY + 1), float(currZ), 1.0f));
        inter.push_back(glm::vec4(-1,0,0,0));
        inter.push_back(glm::vec4(uvStartCoord.x,
                                  uvStartCoord.y,
                                  cosPow,
                                  animatable));

        inter.push_back(glm::vec4(float(currX), float(currY + 1), float(currZ + 1), 1.0f));
        inter.push_back(glm::vec4(-1,0,0,0));
        inter.push_back(glm::vec4(uvStartCoord.x + norLengthBlock,
                                  uvStartCoord.y,
                                  cosPow,
                                  animatable));

        inter.push_back(glm::vec4(float(currX), float(currY), float(currZ + 1), 1.0f));
        inter.push_back(glm::vec4(-1,0,0,0));
        inter.push_back(glm::vec4(uvStartCoord.x + norLengthBlock,
                                  uvStartCoord.y + norLengthBlock,
                                  cosPow,
                                  animatable));

        createSquareIndices(initial, idx);
        adjEmpty = false;
    }

    if(currY == 255) {
        adjEmpty = true;

    } else {
        adjEmpty = (blockArray.at(currX + 16 * (currY + 1) + 256 * 16 * currZ) == EMPTY);
        opaqueAdjTrans = Terrain::isTransparent(blockArray.at(currX + 16 * (currY + 1) + 256 * 16 * currZ)) && opaque;

    }

    // face on top of the block------------------------------------------------------------
    if (adjEmpty || opaqueAdjTrans) {
        //don't need to check chunk adjacency since there are no chunks above
        inter.push_back(glm::vec4(float(currX), float(currY + 1), float(currZ), 1.0f));
        inter.push_back(glm::vec4(0,1,0,0));
        inter.push_back(glm::vec4(topUVStartCoord.x, topUVStartCoord.y,
                                  cosPow, animatable));

        inter.push_back(glm::vec4(float(currX + 1), float(currY + 1), float(currZ), 1.0f));
        inter.push_back(glm::vec4(0,1,0,0));
        inter.push_back(glm::vec4(topUVStartCoord.x + norLengthBlock, topUVStartCoord.y,
                                  cosPow, animatable));

        inter.push_back(glm::vec4(float(currX + 1), float(currY + 1), float(currZ + 1), 1.0f));
        inter.push_back(glm::vec4(0,1,0,0));
        inter.push_back(glm::vec4(topUVStartCoord.x + norLengthBlock, topUVStartCoord.y + norLengthBlock,
                                  cosPow, animatable));

        inter.push_back(glm::vec4(float(currX), float(currY + 1), float(currZ + 1), 1.0f));
        inter.push_back(glm::vec4(0,1,0,0));
        inter.push_back(glm::vec4(topUVStartCoord.x, topUVStartCoord.y + norLengthBlock,
                                  cosPow, animatable));

        createSquareIndices(initial, idx);
        adjEmpty = false;
    }

    if(currY == 0) {
        adjEmpty = true;
    } else {
        adjEmpty = (blockArray.at(currX + 16 * (currY - 1) + 256 * 16 * currZ) == EMPTY);
        opaqueAdjTrans = Terrain::isTransparent(blockArray.at(currX + 16 * (currY - 1) + 256 * 16 * currZ)) && opaque;

    }


    // face at bottom of the block------------------------------------------------------------
    if (adjEmpty || opaqueAdjTrans) {
        //don't need to check chunk adjacency since there are no chunks below
        inter.push_back(glm::vec4(float(currX), float(currY), float(currZ), 1));
        inter.push_back(glm::vec4(0,-1,0,0));
        inter.push_back(glm::vec4(botUVStartCoord.x, botUVStartCoord.y,
                                  cosPow, animatable));

        inter.push_back(glm::vec4(float(currX + 1), float(currY), float(currZ), 1));
        inter.push_back(glm::vec4(0,-1,0,0));
        inter.push_back(glm::vec4(botUVStartCoord.x + norLengthBlock, botUVStartCoord.y,
                                  cosPow, animatable));

        inter.push_back(glm::vec4(float(currX + 1), float(currY), float(currZ + 1), 1));
        inter.push_back(glm::vec4(0,-1,0,0));
        inter.push_back(glm::vec4(botUVStartCoord.x + norLengthBlock, botUVStartCoord.y + norLengthBlock,
                                  cosPow, animatable));

        inter.push_back(glm::vec4(float(currX), float(currY), float(currZ + 1), 1));
        inter.push_back(glm::vec4(0,-1,0,0));
        inter.push_back(glm::vec4(botUVStartCoord.x, botUVStartCoord.y + norLengthBlock,
                                  cosPow, animatable));

        createSquareIndices(initial, idx);
        adjEmpty = false;
    }

    if(currZ == 0) {
        adjEmpty = false;
        if (back != nullptr) {
            adjEmpty = (back->blockAt(currX,currY,15) == EMPTY);
            opaqueAdjTrans = Terrain::isTransparent(back->blockAt(currX,currY,15)) && opaque;

        } else {
            adjEmpty = true;
        }

    } else {
        adjEmpty = (blockArray.at(currX + 16 * currY + 256 * 16 * (currZ - 1)) == EMPTY);
        opaqueAdjTrans = Terrain::isTransparent(blockArray.at(currX + 16 * currY + 256 * 16 * (currZ - 1))) && opaque;

    }

    // face behind block------------------------------------------------------------
    if(adjEmpty || opaqueAdjTrans) {
        inter.push_back(glm::vec4(float(currX), float(currY), float(currZ), 1));
        inter.push_back(glm::vec4(0,0,-1,0));
        inter.push_back(glm::vec4(uvStartCoord.x + norLengthBlock, uvStartCoord.y + norLengthBlock,
                                  cosPow, animatable));

        inter.push_back(glm::vec4(float(currX + 1), float(currY), float(currZ), 1));
        inter.push_back(glm::vec4(0,0,-1,0));
        inter.push_back(glm::vec4(uvStartCoord.x, uvStartCoord.y + norLengthBlock,
                                  cosPow, animatable));

        inter.push_back(glm::vec4(float(currX + 1), float(currY + 1), float(currZ), 1));
        inter.push_back(glm::vec4(0,0,-1,0));
        inter.push_back(glm::vec4(uvStartCoord.x, uvStartCoord.y, cosPow, animatable));

        inter.push_back(glm::vec4(float(currX), float(currY + 1), float(currZ), 1));
        inter.push_back(glm::vec4(0,0,-1,0));
        inter.push_back(glm::vec4(uvStartCoord.x + norLengthBlock, uvStartCoord.y,
                                  cosPow, animatable));

        createSquareIndices(initial, idx);
        adjEmpty = false;
    }

    if(currZ == 15) {
        adjEmpty = false;
        if (front != nullptr) {
            adjEmpty = (front->blockAt(currX, currY, 0) == EMPTY);
            opaqueAdjTrans = Terrain::isTransparent(front->blockAt(currX,currY,0)) && opaque;

        } else {
            adjEmpty = true;
        }

    } else {
        adjEmpty = (blockArray.at(currX + 16 * currY + 256 * 16 * (currZ + 1)) == EMPTY);
        opaqueAdjTrans = Terrain::isTransparent(blockArray.at(currX + 16 * currY + 256 * 16 * (currZ + 1))) && opaque;

    }

    // face in front of block------------------------------------------------------------

    if(adjEmpty || opaqueAdjTrans) {

        inter.push_back(glm::vec4(float(currX), float(currY), float(currZ + 1), 1));
        inter.push_back(glm::vec4(0,0,1,0));
        inter.push_back(glm::vec4(uvStartCoord.x + norLengthBlock, uvStartCoord.y + norLengthBlock,
                                  cosPow, animatable));

        inter.push_back(glm::vec4(float(currX + 1), float(currY), float(currZ + 1), 1));
        inter.push_back(glm::vec4(0,0,1,0));
        inter.push_back(glm::vec4(uvStartCoord.x, uvStartCoord.y + norLengthBlock,
                                  cosPow, animatable));

        inter.push_back(glm::vec4(float(currX + 1), float(currY + 1), float(currZ + 1), 1));
        inter.push_back(glm::vec4(0,0,1,0));
        inter.push_back(glm::vec4(uvStartCoord.x, uvStartCoord.y, cosPow, animatable));

        inter.push_back(glm::vec4(float(currX), float(currY + 1), float(currZ + 1), 1));
        inter.push_back(glm::vec4(0,0,1,0));
        inter.push_back(glm::vec4(uvStartCoord.x + norLengthBlock, uvStartCoord.y,
                                  cosPow, animatable));

        createSquareIndices(initial, idx);
        adjEmpty = false;
    }
}

void Chunk::bindOpaquePart() {
    count = idxOpaque.size();

    //generateInterleaved();
    context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufInter);
    context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, interOpaque.size() * sizeof(glm::vec4), interOpaque.data(), GL_STATIC_DRAW);

    //  generateIdx();
    context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufIdx);
    context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, idxOpaque.size() * sizeof(GLuint), idxOpaque.data(), GL_STATIC_DRAW);
}
void Chunk::bindNonOpaquePart() {
    count = idxNonOpaque.size();

    // generateInterleaved();
    context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufInter);
    context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, interNonOpaque.size() * sizeof(glm::vec4), interNonOpaque.data(), GL_STATIC_DRAW);

    //  generateIdx();
    context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufIdx);
    context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, idxNonOpaque.size() * sizeof(GLuint), idxNonOpaque.data(), GL_STATIC_DRAW);
}

void Chunk::clearData() {
    idxOpaque.clear();
    idxNonOpaque.clear();
    interOpaque.clear();
    interNonOpaque.clear();
}
void Chunk :: create() {
    clearData();
    // The below are for UV mapping
    float sizeWholeImageTexture = 256.f; //the given image is of size 256 x 256 --T
    float lengthBlock = 16.f;
    float norLengthBlock = lengthBlock / sizeWholeImageTexture; //normalized length of block
    glm::vec4 defColor = glm::vec4(0.2f, 1.0f, 0.6f, 1);

    int initialOpaque = 0;
    int initialNonOpaque = 0;

    if (!createdOpaque) { //To ensure we only iterate through all blocks once
        for (int currX = 0; currX < 16; currX++) {
            for (int currY = 0; currY < 256; currY++) {
                for (int currZ = 0; currZ < 16; currZ++) {

                    glm::vec2 uvStartCoord = glm::vec2(0.f, 0.f);
                    float opaque = true;
                    float animatable = 0.f; //1 means animatable (lava, water), 0 means not
                    float cosPow = 1.f;

                    int posInBlockArr = currX + 16 * currY + 16 * 256 * currZ;
                    BlockType curBlockType = blockArray[posInBlockArr];
                    if (curBlockType != EMPTY) {
                        switch(curBlockType)
                        {
                        case BAMBOO:
                            cosPow = 5000.f;
                            uvStartCoord = glm::vec2(9, 4);
                            break;
                        case BEDROCK:
                            cosPow = 8000.f;
                            uvStartCoord = glm::vec2(1, 1);
                            break;
                        case DIRT:
                            cosPow = 1.f;
                            uvStartCoord = glm::vec2(2, 0);
                            break;
                        case GRASS:
                            cosPow = 10.f;
                            uvStartCoord = glm::vec2(3, 0); // grass side
                            break;
                        case GUM:
                            cosPow = 1.5f;
                            uvStartCoord = glm::vec2(2, 8);
                            break;
                        case ICE:
                            opaque = false;
                            cosPow = 6000.f;
                            uvStartCoord = glm::vec2(3, 4);
                            break;
                        case LAVA:
                            cosPow = 200.f;
                            uvStartCoord = glm::vec2(14, 14);
                            animatable = 1.f;
                            break;
                        case LEAF:
                            cosPow = 10.f;
                            uvStartCoord = glm::vec2(5, 3);
                            break;
                        case SAND:
                            cosPow = 10.f;
                            uvStartCoord = glm::vec2(2, 1);
                            break;
                        case SNOW:
                            cosPow = 1.f;
                            uvStartCoord = glm::vec2(2, 4);
                            break;
                        case STONE:
                            cosPow = 7000.f;
                            uvStartCoord = glm::vec2(0, 0);
                            break;
                        case WATER:
                            opaque = false;
                            cosPow = 9000.f;
                            uvStartCoord = glm::vec2(13, 12);
                            animatable = 1.f;
                            break;
                        case WOOD:
                            cosPow = 1.f;
                            uvStartCoord = glm::vec2(4, 1); //wood side
                        }
                        uvStartCoord *= norLengthBlock; //change from block coordinate to uv coordinate

                        if (opaque) { //the block is opaque
                            pushBlockIntoVBO(interOpaque, idxOpaque, uvStartCoord, cosPow,
                                             animatable, currX, currY, currZ,
                                             norLengthBlock, initialOpaque, curBlockType);
                        } else { //the block is not opaque
                            pushBlockIntoVBO(interNonOpaque, idxNonOpaque, uvStartCoord, cosPow,
                                             animatable, currX, currY, currZ,
                                             norLengthBlock, initialNonOpaque, curBlockType);
                        }
                    }
                }
            }
        }
    }

    generateInterleaved();
    generateIdx();
}

Terrain::Turtle::Turtle(glm::vec4 pos, glm::vec4 orientation, float depth)
    : pos(pos), orientation(orientation), depth(depth), material(WATER)
{

}

Terrain::Turtle::Turtle(glm::vec4 pos, glm::vec4 orientation, float depth, BlockType material)
    : pos(pos), orientation(orientation), depth(depth), material(material)
{

}

//Terrain::Turtle::Turtle(Turtle t)
//    : pos(t.pos), orientation(t.orientation), depth(t.depth)
//{

//}

Terrain::Turtle::Turtle()
    : pos(0), orientation(0), depth(0)

{

}



void Terrain::expandString(QString &sentence) {
    QString newString = "";
    for(QChar c : sentence) {
        if(charToExpandedString.contains(c)) {
            newString += charToExpandedString.value(c);
        } else {
            newString += c;
        }
        // charToDrawingOperation
    }

    sentence = newString;
}

void Terrain::setExpandedStrings(LType r) {
    charToExpandedString.clear();
    switch(r) {
    case DELTA:
        charToExpandedString.insert('X',"F[+F-F+FX][-F+FX]");
        break;
    case LINEAR:
        charToExpandedString.insert('X',"F[+F--FF][FFFX]");
        break;
    case TREE:
        charToExpandedString.insert('X',"bF.[+,<lsFX].bF[.>.<lsFX].bF[-.-lsFX]X");
        break;
    case BAMBOOTREE:
        charToExpandedString.insert('X',"bF.[+,<lFX].bF[.>.<lFX].bF[-.-lFX]X");
        break;

    case PINE:
        charToExpandedString.insert('X',"F[>>>>>FX]");

        break;
    default:
        charToExpandedString.insert('X',"F[+F-F+FX][-F+FX]");
        break;

    }
}

void Terrain::setDrawingRules(LType r) {
    charToDrawingOperation.insert('F',&Terrain::moveTurtleForwardRand);
    charToDrawingOperation.insert('-',&Terrain::rotateTurtleNeg);
    charToDrawingOperation.insert('+',&Terrain::rotateTurtlePos);
    charToDrawingOperation.insert('[',&Terrain::pushTurtle);
    charToDrawingOperation.insert(']',&Terrain::popTurtle);
    charToDrawingOperation.insert('<',&Terrain::rotateTurtleXNeg);
    charToDrawingOperation.insert('>',&Terrain::rotateTurtleXPos);
    charToDrawingOperation.insert('.',&Terrain::rotateTurtleZPos);
    charToDrawingOperation.insert(',',&Terrain::rotateTurtleZNeg);
    charToDrawingOperation.insert('l',&Terrain::makeTurtleLeaf);
    charToDrawingOperation.insert('b',&Terrain::makeTurtleBamboo);
    charToDrawingOperation.insert('g',&Terrain::makeTurtleGrass);
    charToDrawingOperation.insert('w',&Terrain::makeTurtleWood);
    charToDrawingOperation.insert('s',&Terrain::subTurtleDepth);
    charToDrawingOperation.insert('a',&Terrain::addTurtleDepth);

}


void Terrain::moveTurtleUp(LType r, Turtle& currTurt, QStack<Turtle>& turtleStack, QHash <uint64_t, Chunk*>& newChunks) {
    switch (r) {
    case TREE:
    {
        currTurt.pos.y += 3;
        break;
    }
    default:
    {
        break;
    }
    }
}

void Terrain::moveTurtleForwardRand(LType r, Turtle& currTurt, QStack<Turtle>& turtleStack, QHash <uint64_t, Chunk*>& newChunks) {
    switch (r) {
    case DELTA:
        for(int i = 0; i < 5; i++) {
            glm::vec4 prevPos = currTurt.pos;
            float randfloat = fbm(prevPos.x, prevPos.z) * 2;
            float randAng = -30 + fbm(prevPos.x, prevPos.z) * 70;
            glm::mat4 randRot = glm::rotate(glm::mat4(), glm::radians(randAng),glm::vec3(0.f,1.f,0.f));
            currTurt.pos += (float) (randfloat + 1.f / fmax(1.f, pow((float) turtleStack.size() * 0.5f, 1.3f))) * randRot * currTurt.orientation ;
            drawLine(prevPos, currTurt.pos, 4.f / fmax(0.5f,turtleStack.size() * 2), WATER, newChunks);

        }
        break;


    case LINEAR:
        for(int i = 0; i < 4; i++) {
            glm::vec4 prevPos = currTurt.pos;
            float randfloat = 4 + fbm(prevPos.x, prevPos.z) * 8;
            float randAng = -100 + fbm(prevPos.x, prevPos.z) * 200;
            glm::mat4 randRot = glm::rotate(glm::mat4(), glm::radians(randAng),glm::vec3(0.f,1.f,0.f));
            currTurt.pos += (float) (randfloat + 1.f / fmax(0.5f, pow((float) turtleStack.size() * 0.5f, 1.3f))) * randRot * currTurt.orientation ;
            drawLine(prevPos, currTurt.pos, 2.f / fmax(0.5f,turtleStack.size() * 0.4), LAVA, newChunks);


        }
        break;


    case TREE:
    {
        float randfloat = (4.f + fbm(currTurt.pos.x, currTurt.pos.z) * 2.f) / fmax(1, turtleStack.size() * 10);

        for(int i = 0; i < randfloat; i ++) {
            glm::vec4 prevPos = currTurt.pos;

            currTurt.pos += currTurt.orientation;

            int x = floor(currTurt.pos.x);
            int y = floor(fmax(fmin(currTurt. pos.y, 255), 0));
            int z = floor(currTurt.pos.z);
            for (float j = -currTurt.depth; j < currTurt.depth; j++) {
                for (float k = -currTurt.depth; k < currTurt.depth; k++) {
                    for(float  l = -currTurt.depth; l < currTurt.depth; l++) {
                        setBlockAt(x + j, y + k, z + l, currTurt.material);
                    }
                }
            }
        }

        currTurt.depth = fmax(0.5, currTurt.depth - 3);
        break;

    }
    case BAMBOOTREE:
    {
        float randfloat = (4.f + fbm(currTurt.pos.x, currTurt.pos.z) * 2.f) / fmax(1, turtleStack.size() * 10);

        for(int i = 0; i < randfloat; i ++) {
            glm::vec4 prevPos = currTurt.pos;

            currTurt.pos += currTurt.orientation;

            int x = floor(currTurt.pos.x);
            int y = floor(fmax(fmin(currTurt. pos.y, 255), 0));
            int z = floor(currTurt.pos.z);
            for (float j = -currTurt.depth; j < currTurt.depth; j++) {
                for (float k = -currTurt.depth; k < currTurt.depth; k++) {
                    for(float  l = -currTurt.depth; l < currTurt.depth; l++) {
                        setBlockAt(x + j, y + k, z + l, currTurt.material);
                    }
                }
            }
        }


        break;
    }
    case PINE:
    {
        float randfloat = 4.f + (fbm(currTurt.pos.x, currTurt.pos.z) * 2.f) / fmax(1, turtleStack.size() * 10);

        for(int i = 0; i < randfloat; i ++) {
            glm::vec4 prevPos = currTurt.pos;

            currTurt.pos += currTurt.orientation;

            int x = floor(currTurt.pos.x);
            int y = floor(fmax(fmin(currTurt. pos.y, 255), 0));
            int z = floor(currTurt.pos.z);
            for (float j = -currTurt.depth; j < currTurt.depth; j++) {
                for (float k = -currTurt.depth; k < currTurt.depth; k++) {
                    for(float  l = -currTurt.depth; l < currTurt.depth; l++) {
                        setBlockAt(x + j, y + k, z + l, currTurt.material);
                    }
                }
            }
        }
        break;

    }

    }
}
void Terrain::moveTurtleBackward(LType r, Turtle& currTurt, QStack<Turtle>& turtleStack, QHash <uint64_t, Chunk*>& newChunks) {
    currTurt.pos -= currTurt.orientation;
}

void Terrain::rotateTurtleZPos(LType r, Turtle& currTurt, QStack<Turtle>& turtleStack, QHash <uint64_t, Chunk*>& newChunks) {

    switch (r) {
    case TREE:
    {
        float angle = (50 + fbm(currTurt. orientation.x, currTurt. orientation.z) * 50) / fmax(1, turtleStack.size());
        currTurt.orientation = glm::normalize(glm::rotate(glm::mat4(),angle,glm::vec3(0.f,0.f,1.f)) * currTurt. orientation);
        break;
    }
    case PINE:
    {
        float angle = (50 + fbm(currTurt. orientation.x, currTurt. orientation.z) * 50) / fmax(1, turtleStack.size());
        currTurt.orientation = glm::normalize(glm::rotate(glm::mat4(),angle,glm::vec3(0.f,0.f,1.f)) * currTurt. orientation);
        break;
    }
    default:
        break;
    }
}

void Terrain::rotateTurtleZNeg(LType r, Turtle& currTurt, QStack<Turtle>& turtleStack, QHash <uint64_t, Chunk*>& newChunks) {
    switch (r) {
    case TREE:
    {
        float angle = -(50 + fbm(currTurt. orientation.x, currTurt. orientation.z) * 50) / fmax(1, turtleStack.size());
        currTurt.orientation = glm::normalize(glm::rotate(glm::mat4(),angle,glm::vec3(0.f,0.f,1.f)) * currTurt. orientation);

        break;
    }

    case BAMBOOTREE:
    {
        float angle = -(-100 + fbm(currTurt. orientation.x, currTurt. orientation.z) * 200) / fmax(1, turtleStack.size());
        currTurt.orientation = glm::rotate(glm::mat4(),angle,glm::vec3(0.f,0.f,1.f)) * currTurt. orientation;

        break;
    }
    case PINE:
    {
        float angle = -(-100 + fbm(currTurt. orientation.x, currTurt. orientation.z) * 200) / fmax(1, turtleStack.size());
        currTurt.orientation = glm::rotate(glm::mat4(),angle,glm::vec3(0.f,0.f,1.f)) * currTurt. orientation;

        break;
    }
    default:
        break;
    }
}


void Terrain::rotateTurtleXPos(LType r, Turtle& currTurt, QStack<Turtle>& turtleStack, QHash <uint64_t, Chunk*>& newChunks) {
    switch (r) {
    case TREE:
    {

        float angle =(50 + fbm(currTurt. orientation.x, currTurt. orientation.z) * 50) / fmax(1, turtleStack.size());
        currTurt.orientation = glm::normalize(glm::rotate(glm::mat4(),angle,glm::vec3(1.f,0.f,0.f)) * currTurt.orientation);

        break;
    }

    case BAMBOOTREE:
    {

        float angle =(-100 + fbm(currTurt. orientation.x, currTurt. orientation.z) * 200) / fmax(1, turtleStack.size());
        currTurt.orientation = glm::rotate(glm::mat4(),angle,glm::vec3(1.f,0.f,0.f)) * currTurt.orientation;

        break;
    }

    case PINE:
    {

        float angle =(-100 + fbm(currTurt. orientation.x, currTurt. orientation.z) * 200) / fmax(1, turtleStack.size());
        currTurt.orientation = glm::rotate(glm::mat4(),angle,glm::vec3(1.f,0.f,0.f)) * currTurt.orientation;

        break;
    }
    default:
        break;
    }
}

void Terrain::rotateTurtleXNeg(LType r, Turtle& currTurt, QStack<Turtle>& turtleStack, QHash <uint64_t, Chunk*>& newChunks) {
    switch (r) {
    case TREE:
    {
        float angle = -50 - fbm(currTurt.orientation.x, currTurt. orientation.z) * 50 / fmax(1, turtleStack.size());
        currTurt.orientation = glm::normalize(glm::rotate(glm::mat4(),angle,glm::vec3(1.f,0.f,0.f)) * currTurt.orientation);

        break;
    }
    case BAMBOOTREE:
    {
        float angle = -50 - fbm(currTurt. orientation.x, currTurt. orientation.z) * 50 / fmax(1, turtleStack.size());
        currTurt.orientation = glm::rotate(glm::mat4(),angle,glm::vec3(1.f,0.f,0.f)) * currTurt.orientation;

        break;
    }

    case PINE:
    {
        float angle = -50 - fbm(currTurt. orientation.x, currTurt. orientation.z) * 50 / fmax(1, turtleStack.size());
        currTurt.orientation = glm::rotate(glm::mat4(),angle,glm::vec3(1.f,0.f,0.f)) * currTurt.orientation;

        break;
    }
    default:
        break;
    }

}

void Terrain::rotateTurtlePos(LType r, Turtle& currTurt, QStack<Turtle>& turtleStack, QHash <uint64_t, Chunk*>& newChunks) {

    switch (r) {
    case DELTA:
    {
        float angle = (50 + fbm(currTurt.orientation.x, currTurt.orientation.z) * 40) / fmax(1, turtleStack.size());

        currTurt.orientation = glm::rotate(glm::mat4(),glm::radians(angle),glm::vec3(0.f,1.f,0.f)) * currTurt.orientation;
        break;
    }
    case LINEAR:
    {
        float angle = (50 + fbm(currTurt.orientation.x, currTurt.orientation.z) * 3) / fmax(1, turtleStack.size());

        currTurt.orientation = glm::rotate(glm::mat4(),glm::radians(angle),glm::vec3(0.f,1.f,0.f)) * currTurt.orientation;

        break;
    }

    case TREE:
    {
        float angle = (50 + fbm(currTurt. orientation.x, currTurt. orientation.z) * 50) / fmax(1, turtleStack.size());
        currTurt. orientation = glm::rotate(glm::mat4(),angle, glm::vec3(0.f,1.f,0.f)) * currTurt. orientation;
        break;
    }

    case BAMBOOTREE:
    {
        float angle = (50 + fbm(currTurt. orientation.x, currTurt. orientation.z) * 50) / fmax(1, turtleStack.size());
        currTurt. orientation = glm::rotate(glm::mat4(),angle, glm::vec3(0.f,1.f,0.f)) * currTurt. orientation;
        break;
    }


    case PINE:
    {
        float angle = -(50 + fbm(currTurt. orientation.x, currTurt. orientation.z) * 3) / fmax(1, turtleStack.size());

        currTurt. orientation = glm::rotate(glm::mat4(),angle, glm::vec3(0.f,1.f,0.f)) * currTurt. orientation;

        break;
    }
    }


}

void Terrain::rotateTurtleNeg(LType r, Turtle& currTurt, QStack<Turtle>& turtleStack, QHash <uint64_t, Chunk*>& newChunks) {

    switch (r) {
    case DELTA:
    {
        float angle = (-50 - fbm(currTurt.orientation.x, currTurt.orientation.z) * 40) / fmax(1, turtleStack.size());
        currTurt.orientation = glm::rotate(glm::mat4(),glm::radians(angle),glm::vec3(0.f,1.f,0.f)) * currTurt.orientation;

        break;
    }
    case LINEAR:
    {
        float angle = (-50 - fbm(currTurt.orientation.x, currTurt.orientation.z) * 3) / fmax(1, turtleStack.size());
        currTurt.orientation = glm::rotate(glm::mat4(),glm::radians(angle),glm::vec3(0.f,1.f,0.f)) * currTurt.orientation;

        break;
    }

    case TREE:
    {
        float angle = -(50 + fbm(currTurt. orientation.x, currTurt. orientation.z) * 3) / fmax(1, turtleStack.size());

        currTurt. orientation = glm::rotate(glm::mat4(),angle, glm::vec3(0.f,1.f,0.f)) * currTurt. orientation;

        break;
    }

    case BAMBOOTREE:
    {
        float angle = -(50 + fbm(currTurt. orientation.x, currTurt. orientation.z) * 3) / fmax(1, turtleStack.size());

        currTurt. orientation = glm::rotate(glm::mat4(),angle, glm::vec3(0.f,1.f,0.f)) * currTurt. orientation;

        break;
    }

    case PINE:
    {
        float angle = -(50 + fbm(currTurt. orientation.x, currTurt. orientation.z) * 3) / fmax(1, turtleStack.size());

        currTurt. orientation = glm::rotate(glm::mat4(),angle, glm::vec3(0.f,1.f,0.f)) * currTurt. orientation;

        break;
    }
    }
}

void Terrain::pushTurtle(LType r, Turtle& currTurt, QStack<Turtle>& turtleStack, QHash <uint64_t, Chunk*>& newChunks) {
    turtleStack.push(currTurt);
}

void Terrain::popTurtle(LType r, Turtle& currTurt, QStack<Turtle>& turtleStack, QHash <uint64_t, Chunk*>& newChunks) {
    if(turtleStack.size() > 0) {
        currTurt = turtleStack.pop();
    }
}

void Terrain:: bresenhamAlongH(int x1, int x2, int z1, int z2, int y, float thickness, BlockType type, QHash <uint64_t, Chunk*>& newChunks) {
    if (z2 < z1) {
        //will be negative
        int m_new = 2 * (z2 - z1);
        int slope_error_new = m_new + (x2 - x1);

        for (float x = x1, z = z1; x <= x2; x++)
        {


            // riverCreated.insert(getChunkAt(x, z));


            for(float i = -thickness; i < thickness; i++) {

                int bottom = 128;
                int deep = 15;

                //if(chunkExistsAt(x, bottom,z + i) && getBlockAt(x, bottom, z + i) != type) {
                fillBetweenNewChunks(x, z + i, bottom, 255, EMPTY, newChunks);
                fillBetweenNewChunks(x, z + i, bottom - deep, bottom, type, newChunks);
                carve(x, z + i, bottom + 1, 7, 10, newChunks);
                //}
            }

            // Add slope to increment angle formed
            //gets smaller since m_new is negative
            slope_error_new += m_new;

            // Slope error reached limit, time to
            // increment y and update slope error.
            if (slope_error_new <= 0)
            {
                z--;
                slope_error_new  += 2 * (x2 - x1);
            }
        }

    } else {

        int m_new = 2 * (z2 - z1);
        int slope_error_new = m_new - (x2 - x1);

        for (int x = x1, z = z1; x <= x2; x++)
        {


            //if (created.contains(getChunkAt(x, z))) {

            // riverCreated.insert(getChunkAt(x, z));
            //}

            for(float i = -thickness; i < thickness; i++) {
                int bottom = 128;
                int deep = 15;

                // if(chunkExistsAt(x, bottom, z + i) && getBlockAt(x, bottom, z + i) != type) {
                fillBetweenNewChunks(x, z + i, bottom, 255, EMPTY, newChunks);

                fillBetweenNewChunks(x,  z + i, bottom - deep, bottom, type, newChunks);

                carve(x, z + i, bottom + 1, 7, 10, newChunks);

                // }
            }
            // Add slope to increment angle formed
            slope_error_new += m_new;

            // Slope error reached limit, time to
            // increment y and update slope error.
            if (slope_error_new >= 0)
            {
                z++;
                slope_error_new  -= 2 * (x2 - x1);
            }
        }
    }

}

void Terrain::bresenhamAlongV(int x1, int x2, int z1, int z2, int y, float thickness, BlockType type, QHash <uint64_t, Chunk*>& newChunks) {

    if (x2 >= x1) {
        int m_new = 2 * (x2 - x1);
        int slope_error_new = m_new - (z2 - z1);

        //offset for locked dimension to be drawn, lets us procedurally extend into locked dimension
        int offset = 0;

        for (int x = x1, z = z1; z <= z2; z++)
        {



            // if (created.contains(getChunkAt(x, z))) {
            // riverCreated.insert(getChunkAt(x, z));
            // }

            for(float i = -thickness; i < thickness; i++) {
                int bottom = 128;
                int deep = 15;

                // if(chunkExistsAt(x + i,bottom,z) && getBlockAt(x + i, bottom, z) != type) {
                fillBetweenNewChunks(x + i, z, bottom, 255, EMPTY, newChunks);
                fillBetweenNewChunks(x + i, z, bottom - deep, bottom, type, newChunks);
                carve(x + i, z, bottom + 1, 7, 10, newChunks);

                // }
            }
            slope_error_new += m_new;

            // Slope error reached limit, time to
            // increment y and update slope error.
            if (slope_error_new >= 0)
            {
                x++;
                slope_error_new  -= 2 * (z2 - z1);
            }
        }


    } else {

        int m_new = 2 * (x2 - x1);
        int slope_error_new = m_new + (z2 - z1);

        int offset = 0;
        for (int x = x1, z = z1; z <= z2; z++)
        {



            // if (created.contains(getChunkAt(x, z))) {
            // riverCreated.insert(getChunkAt(x, z));
            //}

            for(float i = -thickness; i < thickness; i++) {
                int bottom = 128;
                int deep = 15;

                //if(chunkExistsAt(x + i,bottom,z) && getBlockAt(x + i, bottom, z) != type) {
                fillBetweenNewChunks(x + i, z, bottom, 255, EMPTY, newChunks);
                fillBetweenNewChunks(x + i, z, bottom - deep, bottom, type, newChunks);
                carve(x + i, z, bottom + 1, 7, 10, newChunks);

                //  }
            }

            // Add slope to increment angle formed
            slope_error_new += m_new;

            // Slope error reached limit, time to
            // increment y and update slope error.
            if (slope_error_new <= 0)
            {
                x--;
                slope_error_new  += 2 * (z2 - z1);
            }
        }

    }

}

//uses bresenhams line algorithm
void Terrain::drawLine(glm::vec4 v1, glm::vec4 v2, float thickness, BlockType type, QHash <uint64_t, Chunk*>& newChunks) {
    glm::vec4 start = v1;
    glm::vec4 end = v2;

    if(v1.x > v2.x) {
        start = v2;
        end = v1;

    }
    int x1 = start.x;
    int z1 = start.z;
    int x2 = end.x;
    int z2 = end.z;



    if(fabs(x1 - x2) > fabs(z1 - z2)) {




        if(newChunks.contains(xzCoord(x1,z1)) || newChunks.contains(xzCoord(x2,z2))) {
            bresenhamAlongH(x1, x2, z1, z2, 128, thickness, type, newChunks);
        }
    } else {

        if(v1.z > v2.z) {
            start = v2;
            end = v1;

        } else {
            start = v1;
            end = v2;
        }
        x1 = start.x;
        z1 = start.z;
        x2 = end.x;
        z2 = end.z;

        if(newChunks.contains(xzCoord(x1,z1)) || newChunks.contains(xzCoord(x2,z2))) {
            bresenhamAlongV(x1, x2, z1, z2, 128, thickness, type, newChunks);


        }

    }
}



void Terrain::makeTurtleLeaf(LType r, Turtle& currTurt, QStack<Turtle>& turtleStack, QHash <uint64_t, Chunk*>& newChunks) {
    currTurt.material = LEAF;
}

void Terrain:: makeTurtleWood(LType r, Turtle& currTurt, QStack<Turtle>& turtleStack, QHash <uint64_t, Chunk*>& newChunks) {
    currTurt.material = WOOD;

}

void Terrain:: makeTurtleBamboo(LType r, Turtle& currTurt, QStack<Turtle>& turtleStack, QHash <uint64_t, Chunk*>& newChunks) {
    currTurt.material = BAMBOO;

}

void Terrain:: makeTurtleGrass(LType r, Turtle& currTurt, QStack<Turtle>& turtleStack, QHash <uint64_t, Chunk*>& newChunks) {
    currTurt.material = GRASS;

}


void Terrain :: addTurtleDepth(LType r, Turtle& currTurt, QStack<Turtle>& turtleStack, QHash <uint64_t, Chunk*>& newChunks) {
    currTurt.depth += 1;

}

void Terrain :: subTurtleDepth(LType r, Turtle& currTurt, QStack<Turtle>& turtleStack, QHash <uint64_t, Chunk*>& newChunks) {
    currTurt.depth = fmax(0.5, currTurt.depth - 0.5);

}



void Terrain::makeRiver(QHash <uint64_t, Chunk*>& newChunks) {
    Turtle currTurt(glm::vec4(32,128,16,1), glm::vec4(1,0,0,1), 0, WATER);

    QStack<Turtle> turtleStack;
    setExpandedStrings(DELTA);
    setDrawingRules(DELTA);
    QString axiom = "FX";

    for(int i = 0; i < 5; i++) {
        expandString(axiom);
    }

    for(QChar c : axiom) {
        if(charToDrawingOperation.contains(c)) {
            Rule r = charToDrawingOperation.value(c);
            (this->*r)(DELTA, currTurt, turtleStack, newChunks);
        }
    }


    currTurt = Turtle(glm::vec4(32,128,48,1),glm::vec4(-1,0,0,1),0, LAVA);
    setExpandedStrings(LINEAR);
    setDrawingRules(LINEAR);
    axiom = "FX";

    for(int i = 0; i < 3; i++) {
        expandString(axiom);
    }

    for(QChar c : axiom) {
        if(charToDrawingOperation.contains(c)) {
            Rule r = charToDrawingOperation.value(c);
            (this->*r)(LINEAR, currTurt, turtleStack, newChunks);
        }
    }


}


void Terrain ::  makeTreeAtChunk(Chunk* chunk) {

    QHash <uint64_t, Chunk*> newChunks;
    float zCoord = (int) (chunk->xzCoord & 0x00000000FFFFFFFF);
    float xCoord = (int) ((chunk->xzCoord >> 32));

    float treeType = fbm(xCoord + 3, zCoord - 8);

    float chance = fbm(xCoord + 1, zCoord + 10) * 1.f;

    int randX = floor(fbm(xCoord, zCoord) * 16);
    int randZ = floor(fbm(xCoord + 7, zCoord - 3) * 16);

    float x = randX + xCoord;
    float z = randZ + zCoord;

    float heightTest = findTop(x,z);
    BlockType soil = getBlockAt(x, heightTest - 1, z);
    if(chance > 0.89) {

        if(soil == DIRT) {
            Turtle currTurt =  Turtle(glm::vec4(x,heightTest,z,1),glm::vec4(0,1,0,1),0.5, BAMBOO);
            QStack<Turtle> turtleStack;

            setExpandedStrings(BAMBOOTREE);
            setDrawingRules(DELTA);
            QString axiom = "FX";


            int branches = floor(fbm(xCoord + 12, zCoord - 12) * 2);


            for(int i = 0; i < 2 + branches; i++) {
                expandString(axiom);
            }

            for(QChar c : axiom) {
                if(charToDrawingOperation.contains(c)) {
                    Rule r = charToDrawingOperation.value(c);
                    (this->*r)(BAMBOOTREE, currTurt, turtleStack, newChunks);
                }
            }

        } else if (soil == GRASS) {
            Turtle currTurt =  Turtle(glm::vec4(x,heightTest,z,1),glm::vec4(0,1,0,1),2, WOOD);
            QStack<Turtle> turtleStack;

            setExpandedStrings(PINE);
            setDrawingRules(DELTA);
            QString axiom = "FX";


            int branches = floor(fbm(xCoord + 12, zCoord - 12) * 2);


            for(int i = 0; i < 1 + branches; i++) {
                expandString(axiom);
            }

            for(QChar c : axiom) {
                if(charToDrawingOperation.contains(c)) {
                    Rule r = charToDrawingOperation.value(c);
                    (this->*r)(TREE, currTurt, turtleStack, newChunks);
                }
            }


        } else {

            Turtle currTurt =  Turtle(glm::vec4(x,heightTest,z,1),glm::vec4(0,1,0,1),1.5, WOOD);
            QStack<Turtle> turtleStack;

            setExpandedStrings(TREE);
            setDrawingRules(DELTA);
            QString axiom = "FX";


            int branches = floor(fbm(xCoord + 12, zCoord - 12) * 2);


            for(int i = 0; i < 1 + branches; i++) {
                expandString(axiom);
            }

            for(QChar c : axiom) {
                if(charToDrawingOperation.contains(c)) {
                    Rule r = charToDrawingOperation.value(c);
                    (this->*r)(PINE, currTurt, turtleStack, newChunks);
                }
            }

        }
    }



}


void Terrain :: makeTrees() {
    QHash <uint64_t, Chunk*> newChunks;
    for(auto p =  chunkMap.begin(); p !=  chunkMap.end(); p++) {


        float zCoord = (int) (p.key() & 0x00000000FFFFFFFF);
        float xCoord = (int) ((p.key() >> 32));

        float treeType = fbm(xCoord + 3, zCoord - 8);

        float chance = fbm(xCoord + 1, zCoord + 10) * 1.f;

        int randX = floor(fbm(xCoord, zCoord) * 16);
        int randZ = floor(fbm(xCoord + 7, zCoord - 3) * 16);

        float x = randX + xCoord;
        float z = randZ + zCoord;

        float heightTest = findTop(x,z);
        BlockType soil = getBlockAt(x, heightTest - 1, z);
        p.value()->filled;
        if(chance > 0.7) {

            if(soil == DIRT) {
                Turtle currTurt =  Turtle(glm::vec4(x,heightTest,z,1),glm::vec4(0,1,0,1),4, BAMBOO);
                QStack<Turtle> turtleStack;

                setExpandedStrings(BAMBOOTREE);
                setDrawingRules(DELTA);
                QString axiom = "FX";


                int branches = floor(fbm(xCoord + 12, zCoord - 12) * 2);


                for(int i = 0; i < 2 + branches; i++) {
                    expandString(axiom);
                }

                for(QChar c : axiom) {
                    if(charToDrawingOperation.contains(c)) {
                        Rule r = charToDrawingOperation.value(c);
                        (this->*r)(BAMBOOTREE, currTurt, turtleStack, newChunks);
                    }
                }

            } else if (soil == GRASS) {
                Turtle currTurt =  Turtle(glm::vec4(x,heightTest,z,1),glm::vec4(0,1,0,1),40, WOOD);
                QStack<Turtle> turtleStack;

                setExpandedStrings(PINE);
                setDrawingRules(DELTA);
                QString axiom = "FX";


                int branches = floor(fbm(xCoord + 12, zCoord - 12) * 2);


                for(int i = 0; i < 1 + branches; i++) {
                    expandString(axiom);
                }

                for(QChar c : axiom) {
                    if(charToDrawingOperation.contains(c)) {
                        Rule r = charToDrawingOperation.value(c);
                        (this->*r)(TREE, currTurt, turtleStack, newChunks);
                    }
                }


            } else {

                Turtle currTurt =  Turtle(glm::vec4(x,heightTest,z,1),glm::vec4(0,1,0,1),4, WOOD);
                QStack<Turtle> turtleStack;

                setExpandedStrings(TREE);
                setDrawingRules(DELTA);
                QString axiom = "FX";


                int branches = floor(fbm(xCoord + 12, zCoord - 12) * 2);


                for(int i = 0; i < 2 + branches; i++) {
                    expandString(axiom);
                }

                for(QChar c : axiom) {
                    if(charToDrawingOperation.contains(c)) {
                        Rule r = charToDrawingOperation.value(c);
                        (this->*r)(TREE, currTurt, turtleStack, newChunks);
                    }
                }

            }
        }

        p.value()->filled = true;
    }
}


int Terrain::findTop(int x, int z) {
    for(int y = 0; y < 256; y++) {
        if(getBlockAt(x,y,z) == EMPTY) {
            return y;
        }
    }
    return 256;
}

float Terrain::calcHeightTest(int x, int z) {
    return fmin(254, fbm(x / 128.f, z / 128.f) * 32.f + 129.f);
}

void Terrain::carve(int x, int z, int min, int radius, float fallOff, QHash <uint64_t, Chunk*>& newChunks) {
    int posXHeight = calcHeightTest(x + radius + 1, z);
    int posZHeight = calcHeightTest(x, z + radius + 1);
    int negXHeight = calcHeightTest(x - radius - 1, z);
    int negZHeight = calcHeightTest(x, z - radius - 1);

    for(int i = -radius; i < radius + 1; i++) {
        for(int j = -radius; j < radius + 1; j++) {
            float interpX = glm::mix(negXHeight, posXHeight, ((float) (i + radius) / (2 * radius)));
            float interpZ = glm::mix(negZHeight, posZHeight, ((float) (j + radius) / (2 * radius)));


            float interpHeight = (interpX + interpZ) * 0.5f;


            int height = min;

            height = glm::mix(min, (int) interpHeight, ((float) (abs(i) + abs(j)) / (2 * radius))) + pow((abs(i) + abs(j)) * fallOff, 0.5);


            // if(getBlockAt(x + i, height, z + j) != EMPTY && chunkExistsAt(x + i, height, z + j)) {
            fillBetweenNewChunks(x + i, z + j, height, 255, EMPTY, newChunks);
            //}

            if (getChunkAt(x + i, z + j) != nullptr) {

                //  riverCreated.insert(getChunkAt(x + i, z + j));
            }



            if (getChunkAt(x + i + 1, z + j + 1) != nullptr) {

                //  riverCreated.insert(getChunkAt(x + i + 1, z + j + 1));
            }

            if (getChunkAt(x + i - 1, z + j - 1) != nullptr) {

                // riverCreated.insert(getChunkAt(x + i - 1, z + j - 1));
            }



            //  created.remove(getChunkAt(x + i, z + j));
            //  created.remove(getChunkAt(x + i - 1, z + j - 1));
            //  created.remove(getChunkAt(x + i + 1, z + j + 1));

            // }

        }
    }
}
