#version 150
// noOp.vert.glsl:
// A fragment shader used for post-processing that simply reads the
// image produced in the first render pass by the surface shader
// and outputs it to the frame buffer


in vec2 fs_UV;

out vec4 outColor;

uniform sampler2D u_Sampler2DTexture;
uniform ivec2 u_Dimensions;
uniform int u_Time;

uniform float FLT_MAX = 1.f / 0.f;
uniform float PI = 3.14159265358979323846;

vec2 random(vec2 p)
{
    return fract(sin(vec2(dot(p, vec2(127.1, 311.7)),
                          dot(p, vec2(269.5, 183.3))))
                          * 43758.5453);
}

void main()
{
//    float uvY = fs_UV.y + cos(fs_UV.x * 25.) * 0.06 *cos(u_Time);
//    vec4 diffuseColor = texture(u_Sampler2DTexture, vec2(fs_UV.x, fs_UV.y * cos(u_Time) * 0.06));
//    vec4 diffuseColor = texture(u_Sampler2DTexture, vec2(fs_UV.x + cos(u_Time) * 0.0006, fs_UV.y));
//    outColor = diffuseColor;
    float m = 16;
    float n = 16;

    float uDimx = float(u_Dimensions.x);
    float uDimy = float(u_Dimensions.y);

    // coords of current pixel in pixel space
    vec2 currPixelCoords = vec2(m * fs_UV.x, n * fs_UV.y);

    // Cell of current pixel
    vec2 currCellCoords = vec2(floor(m * fs_UV.x),
                               floor(n * fs_UV.y));

    vec4 baseColor = texture(u_Sampler2DTexture, fs_UV);

    float smallestDistance = FLT_MAX;
    vec3 finalPixelColor = vec3(0, 0, 0);

    for (float xGrid = max(0.f, currCellCoords.x - 1.f);
               xGrid <= min(currCellCoords.x + 1.f, m - 1.f); xGrid++) {
        for (float yGrid = max(0.f, currCellCoords.y - 1.f);
                   yGrid <= min(currCellCoords.y + 1.f, n - 1.f); yGrid++) {

            vec2 point = random(vec2(xGrid, yGrid));

            point = 0.5 + 0.5 * sin(0.01 * u_Time + 6.2831 * point);

            vec2 pointGridUV = vec2(xGrid + point.x, yGrid + point.y);


            float currDistance = distance(pointGridUV, currPixelCoords);

            if (currDistance < smallestDistance) {
                smallestDistance = currDistance;
                vec2 pointUV = vec2(pointGridUV.x / m, pointGridUV.y / n);

                vec4 randPixelColor = texture(u_Sampler2DTexture, pointUV);
                float normalizedDistance = clamp(currDistance, 0.f, 1.f);

                finalPixelColor = 0.2 * currDistance + randPixelColor.rgb;
            }
        }
    }

    outColor = vec4(baseColor.rgb + 0.7 * finalPixelColor, 1);


}
