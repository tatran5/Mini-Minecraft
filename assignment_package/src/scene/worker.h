#pragma once
#ifndef WORKER_H
#define WORKER_H

#include <QRunnable>
#include <algorithm>
#include "terrain.h"

class Worker : public QRunnable
{
private:
    glm::vec4 m_chunkBounds;
    QMutex* m_mutex;
    Terrain* mp_terrain;

public:
    Worker(QMutex* mutex, glm::vec4 chunkBounds, Terrain* mp_terrain);

    void setBlockAt(int x, int y, int z, BlockType t);
    void run() override;
    void fillBetween(int x, int z, int bottomLimit, int topLimit, BlockType t);
    glm::vec2 random2(glm::vec2 p);
};


//class RiverWorker : public QRunnable
//{
//private:

//public:

//    Terrain* mp_terrain;
//    QMutex* m_mutex;

//    RiverWorker(QMutex* mutex, Terrain* mp_terrain);


//    void updateChunks();
//    void run() override;
//};

#endif // WORKER_H
