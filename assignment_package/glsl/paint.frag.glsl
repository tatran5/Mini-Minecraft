#version 150
// noOp.vert.glsl:
// A fragment shader used for post-processing that simply reads the
// image produced in the first render pass by the surface shader
// and outputs it to the frame buffer


in vec2 fs_UV;

out vec4 outColor;

uniform sampler2D u_Sampler2DTexture;
uniform ivec2 u_Dimensions; // Screen dimensions
uniform int u_Time;


vec2 random2( vec2 p ) {
    return fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))))*43758.5453);
}

vec3 random3( vec3 p ) {
    return fract(sin(vec3(dot(p,vec3(127.1, 311.7, 191.999)),
                          dot(p,vec3(269.5, 183.3, 765.54)),
                          dot(p, vec3(420.69, 631.2,109.21))))
                 *43758.5453);
}

vec2 interpNoise2D(vec2 p) {
    p *= 10;
    float intX = floor(p.x);
    float fractX = fract(p.x);

    float intY = floor(p.y);
    float fractY = fract(p.y);

    float v1 = random2(vec2(intX,intY)).x;
    float v2 = random2(vec2(intX + 1,intY)).x;
    float v3 = random2(vec2(intX,intY + 1)).x;
    float v4 = random2(vec2(intX + 1,intY + 1)).x;


    float v5 = random2(vec2(intX,intY)).y;
    float v6 = random2(vec2(intX + 1,intY)).y;
    float v7 = random2(vec2(intX,intY + 1)).y;
    float v8 = random2(vec2(intX + 1,intY + 1)).y;


    float i1 = mix(v1, v2, fractX);
    float i2 = mix(v3, v4, fractX);
    float i3 = mix(v5, v7, fractY);
    float i4 = mix(v6, v8, fractY);
    return vec2(mix(i1, i2, fractY), mix(i3, i4, fractX));
}

vec2 fbm(vec2 p) {
    float octaves = 8;
    vec2 total = vec2(0);
    float pers = 0.6;
    float freq = 2.f;
    for (int i = 0; i < octaves; i++) {
        float amp = pow(pers, i);
        float f = pow(freq, i);
        total += amp * interpNoise2D(p * f);
    }

    return clamp(total, 0, 1);
}

vec3 posterize(vec3 col) {
    float increments = 12;
    vec3 res = vec3(0);


    res.r = col.r;
    res.g = col.g;
    res.b = col.b;

    float grey = 0.21 * res.x + 0.72 * res.y + 0.07 * res.z;

    //return dark brown if luminance is below 0.3
    if (grey < 0.3) {
        return mix(vec3 (0,0,0),  vec3(100,90,80) / 255, fs_UV.y);
    }
    if(grey < 0.4) {
        return mix(vec3(38,35,28) / 255, res, fs_UV.y);
    }
    return res;
}

float greyScale(vec3 col) {
    return 24 * col.r + 0.72 * col.g + 0.03 * col.b;
}

vec4 posterizeTone(vec4 color, vec4 tone) {
    float greyMap = 0.24 * color.r + 0.72 * color.g + 0.03 * color.b;
    vec4 col = vec4(0, 0, 0, 1);



    if(greyMap < 0.2) {
        //  outColor = mix(greyMap * sepia, vec4(0,0,0,1), greyMap / 0.1);

        col.xyz = mix(0.0 * tone.xyz, 0.8 * tone.xyz, fs_UV.y * fs_UV.x);
        // col.xyz = 0 * tone.xyz;

    } else if (greyMap < 0.4) {
        // outColor = mix(greyMap * sepia, vec4(0,0,0,1), (greyMap - 0.1) / 0.1);

        //  outColor = mix(vec4(68,65,68,255) / 255, vec4(0,0,0,1), fs_UV.y);
        //            col.xyz = greyMap * tone.xyz;

        col.xyz = mix(0.05 * tone.xyz, 0.8 * tone.xyz, fs_UV.y * fs_UV.x);


    } else if (greyMap < 0.6) {

        // outColor = mix(greyMap * sepia, vec4(0,0,0,1), (greyMap - 0.2) / 0.2);

        col.xyz = mix(0.1 * tone.xyz, 0.8 * tone.xyz, fs_UV.y * fs_UV.x);


        //  outColor = mix(vec4(98,95,98, 255) / 255, vec4(0,0,0,1), fs_UV.y);;

    } else if (greyMap < 0.8) {
        // outColor = mix(greyMap * sepia, vec4(0,0,0,1), (greyMap - 0.2)/ 0.4);
        col.xyz = mix(0.6 * tone.xyz, 0.8 * tone.xyz, fs_UV.y * fs_UV.x);


        // col.xyz = greyMap * tone.xyz;
    } else {
        // outColor = vec4(1,1,1,1);
        col.xyz = greyMap * tone.xyz;


    }

    //  col.xyz = greyMap * tone.xyz;


    return col;

}

float saturation(vec3 col) {
    float cmax = max(col.b, (max(col.r, col.g)));
    float cmin = min(col.b, (min(col.r, col.g)));
    if(abs(cmax - cmin) < 0.01) {
        return 0;
    } else {
        return (cmax - cmin) / (1 - abs(cmax + cmin - 1));
    }
}

float lightness(vec3 col) {
    float cmax = max(col.b, (max(col.r, col.g)));
    float cmin = min(col.b, (min(col.r, col.g)));
    return (cmax + cmin) / 2;
}

void main()
{

    vec2 noise = fbm(fs_UV);
    vec4 wash = vec4(219, 220, 212, 255) / 255;

    vec4 diffuseColor = texture(u_Sampler2DTexture, fs_UV);



    vec4 sepia = noise.y * vec4(200, 201, 193, 255) / 255;
    sepia.g += pow(noise.x * 0.1, 3);
    outColor = diffuseColor;
    bool changed = false;
    const float PI = 3.1415926535897932384626433832795;
    const int dim = 11;
    float weights[dim * dim];
    float sigma = 3;
    vec4 totalCol = vec4(0);
    vec4 blurTwo = vec4(0);

    float totalWeight = 0;
    for(int w = -dim / 2; w <= dim / 2; w++) {
        for (int h = -dim / 2; h <= dim / 2; h++) {


            vec2 uv = fs_UV;
            //access the color w pixels right and h pixels up from current pixel

            uv.x = clamp(fs_UV.x + float(w) / float(u_Dimensions.x), 0, 1);
            uv.y = clamp(fs_UV.y + float(h) / float(u_Dimensions.y), 0, 1);
            float weight = 1 / (2.f * PI * pow(sigma, 2.f)) *
                    exp(-(pow(uv.x, 2.f) + pow(uv.y, 2.f)) / (2.f * pow(sigma, 2.f)));

            //only blur for colors similar
            vec4 texCol = texture(u_Sampler2DTexture, uv);
            vec4 texColOffset = texture(u_Sampler2DTexture, clamp((uv - 0.5) * 1.06 + 0.5, 0, 1));

            if(abs(texCol.r - diffuseColor.r) < 0.2
                    && abs(texCol.g - diffuseColor.g) < 0.2
                    && abs(texCol.b - diffuseColor.b) < 0.2) {

                totalCol +=  texCol * weight;
            }

            blurTwo += texColOffset * weight;
            totalWeight += weight;

        }
    }


    vec3 color = totalCol.xyz / totalWeight;
    vec3 blurredColor = blurTwo.xyz / totalWeight;
    outColor.xyz = color;
    outColor = posterizeTone(outColor, sepia);

    float greyBlur = greyScale(blurredColor);


    if(saturation(diffuseColor.xyz) > 0.3 || lightness(diffuseColor.xyz) > 0.8 || diffuseColor.a < 1) {
        outColor = mix(outColor, diffuseColor, 0.8);


    } else {
        outColor = mix(outColor, vec4(blurredColor, 1), 0.3);

    }


    if(greyScale(outColor.xyz) > 0.1) {
        outColor*=1.15;
    }

}
