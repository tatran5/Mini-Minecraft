#include <scene/worker.h>
#include <QDebug>

Worker::Worker(QMutex* mutex, glm::vec4 chunkBounds, Terrain *mp_terrain) :
    m_mutex(mutex), m_chunkBounds(chunkBounds), mp_terrain(mp_terrain)
{}

void Worker::run()
{

    //if min bound is negative, draw from max to min

//    int xStart = m_chunkBounds.x;
//    int xEnd = m_chunkBounds.y;
//    int zStart = m_chunkBounds.z;
//    int zEnd = m_chunkBounds.w;
//    int xInc = 16;
//    int zInc = 16;



//    if(xEnd < 0) {
//        xStart = m_chunkBounds.y;
//        xEnd = m_chunkBounds.x;
//         xInc = -16;

//    }

//    if(zEnd < 0) {
//        zStart = m_chunkBounds.w;
//        zEnd = m_chunkBounds.z;
//         zInc = -16;

//    }
    //empty at first x and first z for a bit
    // chunkbounds : x = minX, y = maxX, z = minZ, w = maxZ

    QHash <uint64_t, Chunk*> newChunks;
    for (int x = m_chunkBounds.x; x < m_chunkBounds.y; x += 16) {
        for (int z = m_chunkBounds.z; z < m_chunkBounds.w; z += 16) {
            Chunk* newChunk = mp_terrain -> makeChunkAtList(x, z, newChunks);

            mp_terrain->makeRiver(newChunks);
            for(Chunk* chunk : newChunks) {
                mp_terrain -> chunkList.insert(chunk -> xzCoord, chunk);
            }
            newChunks.clear();


        }
    }




}

glm::vec2 Worker::random2(glm::vec2 p) {
    return glm::fract(43758.5453f * glm::sin(glm::vec2(glm::dot(p, glm::vec2(127.1, 311.7)),
                                                       glm::dot(p, glm::vec2(269.5, 183.3)))));
}

