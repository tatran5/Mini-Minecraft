----------------------------------------------------------------------------------------------------------------------------------
MINI MINECRAFT MILESTONE 1
----------------------------------------------------------------------------------------------------------------------------------

Procedural : Dzung
- Fbm: I implemented a shader first to test my fbm, which I consulted from the slides. At first I had a lot of difficulty trying to get it to be smooth and not as noisy, but all it took as dividing by a large number before inputting into the noise function. I also added cosine interpolation for smoother result instead of using mix. 
- Ever expanding terrain: I simply check for the four corners and the four edge cases, so in total, I had 8 cases. For the four corners, I just generated new land in 3 direction (so 3 new pieces of 64x256x64 were generated) since it looks better than just generating one piece of land that's in the direction of the player's look vector. 
- Removing block: I just marched along the look vector and if at any point there's a non-empty block (I used floor to find the bottom left corner coordinates), then I just set it to empty.
- Adding block: Similarly, I also casted a ray, but this time I had to check for face intersections since simple checking if box is empty does not tell me where I need to place the new block. 

--------------------------------------
Efficient Terrain Rendering and Chunking: Jake Lem

First, for the array of blockTypes I used a std::vector of 65536 instead of a regular array since there was a lot of room for error with a regular array. In my create function for the Chunk class, I iterated through every block in the scene and ran through a series of conditionals to determine whether or not they were empty (in which case I wouldn't draw them at all), or were adjacent to an non-empty block on a given face, in which case I wouldn't draw the given face. In order to implement the even more efficient rendering section, I decided to add additional instance variables in the chunk class to denote chunk adjacency. As such, I included pointers to a left, right, front, and back Chunk*. I didn't need to include anything for the top or bottom of chunks since each chunk already reached the top of the terrain. I knew that for any block face that was adjacent to another Chunk, it would have to either be on the 0th or 15th index of the Chunk in either the x or z direction (ie at the ends of the chunk). I only checked if either of these were the case, and then checked that against a single block in the adjacent chunk in that face's direction, the block at that chunk's adjacent x or z face at the same y position.

I then implemented interleaved vbos, which required me to add a new function in drawable to bind and generate a vbo for interleaved coordinates, and then to draw the interleaved coordinates in shaderProgram::draw using the stride and offset for glVertexAttribPointer. This wasn't too difficult, except for the fact that I didn't realize you had to include the current element in the stride (ie if you had positions, normals, and colors, your stride would be 3 * sizeof(glm::vec4) rather than 2 * sizeof(glm::vec4)). I then had to edit my Chunk class so that all the vertex information went into a single std::vector in the proper order, which just required some rearranging of information.

Probably the most challenging part for me was implementing the setBlockAt function in terrain, which required me to iterate through the map of Chunk world positions and chunks and determine whether the given coordinate fell within the bounds of any chunks. If it did, I would add the given blockType to the Chunk's local space, ie the world position of the block minus the chunk's world position. If the block didn't fall within the space of any Chunks, then I had to make a new Chunk and add the block there. To do this, I calculated the nearest x and z coordinates divisible by 16 by taking the modulus with 16 and subtracting it from the original coordinate. Then I created a new Chunk and added it to the ChunkMap with these given x and z coordinates, then added the block to the local space of this Chunk by using the same method as above. I also had to check whether this new Chunk was adjacent to any existing chunks on the map and adjust pointers accordingly. It was difficult to do all of this, especially with negative numbers, since I had some trouble with storing negative numbers correctly in the uInt64_t and with calculating the correct positions for chunks at negative coordinates (since the C++ modulus function had some unexpected results with negative numbers).

--------------------------------------
Game Engine: Tea Tran

For the receiving inputs, I decided that the character can only jump if he is not in fly mode, otherwise it would mess up the gravity acting on the player, since that gravity is set to -10 everytime jump was hit. I also made that the character can only jump forward if W is already pressed, and space bar is hit after that, since it does not really make sense for a person to jump and then adjust his x and z velocity while in the air. The character also has the ability to move in two directions at once. I have some difficulties with the mouse movement cursor - hiding the mouse movement but still has the cross cursor in the middle of the scree, and make it smooth . I have not come up with a solution for that.

For the physics update, it is desirable that when W is pressed for moving forward and the look vector is at an angle, the character is still moving forward. I made a separate function in camera class to move along the projection of the look vector of the camera on the XZ plane. 

I cast rays from the corners of my player's bounding box because it is easier than checking box box intersection. I had difficulties with checking if the character is on the ground and the surrounding of it at the same time. The solution is only checking the ground when iterating the bottom corners of the character. Another difficulty faced was that the collision works, the character stops when it hits a face of a non-empty cube. However, it can see the inside. A solution is to push the character back out a bit more (offset by translating the camera along its look or right vector). As a result, when the character constantly attempts to collide with a box, the transition of the camera is not smooth. I have not come up with a solution to fix this.

----------------------------------------------------------------------------------------------------------------------------------
MINI MINECRAFT MILESTONE 2
----------------------------------------------------------------------------------------------------------------------------------
L-river system: Jake
For this project I implemented procedural trees and rivers. For rivers, I made a turtle struct as advised in the instructions, then played around with different sentences and functions for advancing and orientation. For the delta river, I branched twice, whereas for the linear river I advanced multiple times before branching it again. I implemented a version of bresenham's line algorithm to calculate the connection between two points in the turtle's walk. I also used the size of the turtle's stack to change the depth of the river, so that the more it branched the thinner it got. 


For trees, I modified my expanded strings and characters to functions to cover rotation in all three dimensions. Instead of using bresenham's to connect different points in the turtle's walk, I instead repeatedly incremented the turtle's position by its orientation and then drew blocks in that new position. 


I also implemented distance fog by modifying the blinnPhong shader to take in a handle for camera position and then used that to interpolate between the sky color and the block colors.

--------------------------------------
Texture and texture animation: Tea

- Load in images: 
I based it on lecture slides. However, a challenge was that the whole terrain kept being black, even though the QImage was correct. The problem was thatI have to bind the texture in ShaderProgram::draw(). 

- Interleaved VBOs supporting opaque and non opaque: 
For each chunk, I create 4 member variables (interleaved vbo and idx for opaque amd non opaque blocks) and the binding (bindBuffer data, etc.) as 2 separate functions - one for binding opaque and the other for non opaque blocks. The problem is that when drawing scene, I have to bind opaque, draw, bind non opaque, draw, which means I have to store the vbos. Right now, everything seems to be fast even after many terrain expansion, but my implementation is not an optimal solution. A difficulty faced was I called generateInterleaved and generateIdx twice, which created some undesired behavior.

- Blinn phong shading: 
I used the implementation of shaderFun with nearly no modifications. The only thing I changed was using the cosine power for the exponent in one of the functions. 

--------------------------------------
Multithreading and swimming : Dzung
- Multithreading: I used multithreading for fbm calculations and creation of chunks. At first I didn't quite understand why we needed to have a list for our threads to append to, but then I realized that concurrency and such can be a big issue when multithread so I eventually added a list and it works better now. I still have a bug where it doesn't always render all 16 new chunks and we still need to figure out how to incorporate multithreading to make it work for the river generation system that we're having right now.

- Overlay: I added an overlay of blue when the player is underwater and overlay of red when they are under lava. This was achieved pretty quickly based on the direction. The one bug that we have right now is that the transition when the player gets out of water (the overlay disappears before the player gets out of water), which is due to a slight off value in camera-> eye variable and collision detection.

- Swimming: The player presses space and they can swim upward. I also modified player physics so they can swim under water and doesn't stop when they collide with water.

----------------------------------------------------------------------------------------------------------------------------------
MINI MINECRAFT MILESTONE 3
----------------------------------------------------------------------------------------------------------------------------------

Tea - Biomes

I used Worley noise to choose which "grid" would be of which biome. There are 3 in total - dessert, grassland and snowy mountain. Certain assets would have more chance to appear on certain biomes compared with the others (tree on snowy mountains, etc.)  The blocks between two biomes choose randomly if they should be of the same block type in the closest one or the next closest one.

What i did in all three milestones: player physics, texture mapping, biomes

Jake - Multithreading, Sky, Post-process

****IN ORDER TO SEE POST PROCESS SHADER, PRESS M KEY****

For this milestone I implemented the skybox, improved multithreading with river generation, and created a post process shader that simulates painting. For the skybox, I used the provided code and edited it so that the sun moved as a function of time. I also made the sky change color relative to the position of the sun. I then edited the blinn phong shader so that the direction of light changed at the same rate as the direction of the sun. 

For multithreading, I refactored the chunking structure to prevent race conditions. I fully generated all the blocks in any given newly created chunk before passing it to the chunkList. In order to thread rivers, I created a collection of chunks called newChunks, and edited the river functions so that they would only modify the chunks in newChunks. I then called the river function repeatedly within the worker thread every time a new chunk was generated so that rivers could be simultaneously created with chunks.

I also implemented a post process shader to follow the overlay shader. I created a new frame buffer object and texture that could take in the previous texture render pass (either the water overlay or a no op shader) and then modified it create a painterly effect. To achieve this effect, I used a modified gaussian blur function that only averaged vertices of similar colors; this allowed me to maintain somewhat strong outlines around distinct shapes. I then took the blurred image and posterized it by taking the greyscale and flooring it, then multiplying that by a color factor. I also took a regular gaussian blur then mixed the posterized blurred image and the gaussian blurred image. Finally, for fragments that were above a specific saturation/lightness, I returned the original diffuse color so that richer colors stuck out. 

Everything I did in total:

Chunking
River generation
Procedural tree generation
Improved multithreading
Skybox rendering
Post process shader

Dzung

Features implemented:
1) Procedural Terrain generation
2) ray casting for adding/removing blocks
3) Swimming 
4) Multithreaded terrain generation
5) Post process camera overlay for water 
6) Sound (walking and jumping)

For milestone 3, I implemented sound and set up the post process shader pipeline. Sound was pretty straight forward, but for the post process shader, I had this bug where it was just a blinking screen for a bit. Turned out I wasn't passing in the right uvs. And I made a method for create Quad that I never called so yeah it was a time. 


