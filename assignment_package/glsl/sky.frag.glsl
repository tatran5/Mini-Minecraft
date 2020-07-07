#version 150

uniform mat4 u_ViewProj;    // We're actually passing the inverse of the viewproj
// from our CPU, but it's named u_ViewProj so we don't
// have to bother rewriting our ShaderProgram class

uniform ivec2 u_Dimensions; // Screen dimensions
uniform vec3 u_CameraPos;
uniform int u_Time;
in vec4 fs_Pos;
out vec4 outColor;
const float PI = 3.14159265359;
const float TWO_PI = 6.28318530718;

const vec3 daytime[5] = vec3[](vec3(255, 255, 250) / 255.0,
vec3(240, 246, 239) / 255.0,
vec3(236, 233, 230) / 255.0,
vec3(201, 201, 198) / 255.0,
vec3(200, 221, 228) / 255.0);


// Sunset palette
//low to high
const vec3 sunset[5] = vec3[](vec3(255, 255, 250) / 255.0,
vec3(240, 246, 239) / 255.0,
vec3(236, 233, 230) / 255.0,
vec3(160, 165, 176) / 255.0,
vec3(130, 125, 120) / 255.0);
// Dusk palette
const vec3 dusk[5] = vec3[](vec3(255, 255, 250) / 255.0,
vec3(160, 165, 176) / 255.0,
vec3(130, 125, 120) / 255.0,
vec3(87, 86, 79) / 255.0,
vec3(77, 75, 69) / 255.0);

//old color palette
//const vec3 daytime[5] = vec3[](vec3(235, 224, 192) / 255.0,
//vec3(168, 208, 218) / 255.0,
//vec3(136, 180, 216) / 255.0,
//vec3(180, 200, 198) / 255.0,
//vec3(200, 221, 228) / 255.0);
//// Sunset palette
////low to high
//const vec3 sunset[5] = vec3[](vec3(142, 120, 184) / 255.0,
//vec3(252, 144, 168) / 255.0,
//vec3(255, 182, 192) / 255.0,
//vec3(142, 120, 184) / 255.0,
//vec3(57, 32, 51) / 255.0);
//// Dusk palette
//const vec3 dusk[5] = vec3[](vec3(144, 96, 144) / 255.0,
//vec3(96, 72, 120) / 255.0,
//vec3(72, 48, 120) / 255.0,
//vec3(48, 24, 96) / 255.0,
//vec3(0, 24, 72) / 255.0);

const vec3 sunColor = vec3(180, 34, 10) / 255.0;
const vec3 cloudColor = sunset[3];

vec2 sphereToUV(vec3 p) {
    //gives the horizontal component of the ray along the viewing frustum
    float phi = atan(p.z, p.x);
    if(phi < 0) {
        //make sure phi is in range of 0 to 2pi since atan can be negative
        phi += TWO_PI;
    }
    //gives the vertical component of the ray along the viewing frustum
    float theta = acos(p.y);

    //return the uv coord of the fragment in uv range
    return vec2(1 - phi / TWO_PI, 1 - theta / PI);
}


vec2 random2( vec2 p ) {
    return fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))))*43758.5453);
}

vec3 random3( vec3 p ) {
    return fract(sin(vec3(dot(p,vec3(127.1, 311.7, 191.999)),
                          dot(p,vec3(269.5, 183.3, 765.54)),
                          dot(p, vec3(420.69, 631.2,109.21))))
                 *43758.5453);
}

//convert uv coordinates to day color palette, where we interpolate between colors at different intervals
vec3 uvToDay(vec2 uv) {
    if(uv.y < 0.5) {
        return daytime[0];
    }
    else if(uv.y < 0.55) {
        return mix(daytime[0], daytime[1], (uv.y - 0.5) / 0.05);
    }
    else if(uv.y < 0.6) {
        return mix(daytime[1], daytime[2], (uv.y - 0.55) / 0.05);
    }
    else if(uv.y < 0.65) {
        return mix(daytime[2], daytime[3], (uv.y - 0.6) / 0.05);
    }
    else if(uv.y < 0.7) {

        return mix(daytime[3], daytime[4], (uv.y - 0.65) / 0.1);
    }
    return daytime[4];
}


vec3 uvToSunset(vec2 uv) {


    if(uv.y < 0.5) {
        return sunset[0];
    }
    else if(uv.y < 0.55) {
        return mix(sunset[0], sunset[1], (uv.y - 0.5) / 0.05);
    }
    else if(uv.y < 0.6) {
        return mix(sunset[1], sunset[2], (uv.y - 0.55) / 0.05);
    }
    else if(uv.y < 0.65) {
        return mix(sunset[2], sunset[3], (uv.y - 0.6) / 0.05);
    }
    else if(uv.y < 0.75) {
        return mix(sunset[3], sunset[4], (uv.y - 0.65) / 0.1);
    }
    return sunset[4];
}

vec3 uvToDusk(vec2 uv) {
    //use noise to create stars
    //round uv so that random2 generates less noisy random numbers
    vec2 floorpos = floor(uv.xy * 10000) / 10000;

    if(uv.y < 0.5) {

        if(random2(floorpos).x > 0.995) {
            return mix(dusk[0], vec3(1), uv.y);
        }
        return dusk[0];
    }
    else if(uv.y < 0.55) {
        if(random2(floorpos).x > 0.995) {
            return vec3(1);
        }
        return mix(dusk[0], dusk[1], (uv.y - 0.5) / 0.05);
    }
    else if(uv.y < 0.6) {
        if(random2(floorpos).x > 0.995) {
            return vec3(1);
        }

        return mix(dusk[1], dusk[2], (uv.y - 0.55) / 0.05);
    }
    else if(uv.y < 0.65) {
        if(random2(floorpos).x > 0.995) {
            return vec3(1);
        }
        return mix(dusk[2], dusk[3], (uv.y - 0.6) / 0.05);
    }
    else if(uv.y < 0.75) {
        if(random2(floorpos).x > 0.995) {
            return vec3(1);
        }
        return mix(dusk[3], dusk[4], (uv.y - 0.65) / 0.1);
    }

    if(random2(floorpos).x > 0.995) {
        return vec3(1);
    }
    return dusk[4];
}

void main()
{

    float timeFloat = float(u_Time);

    outColor = vec4(0,0.8,1, 1);
    vec2 ndc = (gl_FragCoord.xy / vec2(u_Dimensions)) * 2.0 - 1.0; // -1 to 1 NDC

    //    outColor = vec3(ndc * 0.5 + 0.5, 1);

    vec4 p = vec4(ndc.xy, 1, 1); // Pixel at the far clip plane
    p *= 1000.0; // Times far clip plane value
    p = /*Inverse of*/ u_ViewProj * p; // Convert from unhomogenized screen to world

    //determine the ray between the camera and the viewing frustum at the specific pixel
    //because it is normalized, we are essentially projecting a circle of radius 1 against the plane
    vec3 rayDir = normalize(p.xyz - u_CameraPos);



    vec2 uv = sphereToUV(rayDir);



    vec3 rand = random3(fs_Pos.xyz);

    float disp = sin(timeFloat * 0.0007);
    float dispx = cos(timeFloat * 0.0007);


    //give the position of the center of the sun
    vec3 sunDir = normalize(vec3(0, disp, dispx));
    float sunSize = 30;


    vec3 sunsetColor = uvToSunset(uv);
    vec3 duskColor = uvToDusk(uv);
    vec3 dayColor = uvToDay(uv);

    vec4 dayOut = vec4(dayColor, 1);

    //determine the angle the ray makes with the sun's position, ie see how far the pixel is from the sun
    float angle = acos(dot(rayDir, sunDir)) * 360.0 / PI;

    outColor = vec4(sunsetColor,1);


    //determine sunset colors
    if(angle < sunSize) {
        outColor = vec4(sunColor,1);
    }
    else {

        //determine how much in the same direction ray is w sun
        float raySunDot = dot(rayDir, sunDir);
        if(raySunDot > 0.75) {
            // Do nothing, sky is already correct color
        }

        //if the ray is in the same direction as sun up to a threshhold, interpolate with duskcolor
        else if(raySunDot > -0.1) {
            float t = (raySunDot - 0.75) / -0.85;
            outColor = mix(outColor, vec4(duskColor,1), t);
        }
        else {
            //if the ray is out of range, set to duskColor
            outColor = vec4(duskColor,1);
        }
    }

    //do not mix with the sun
    if(outColor != vec4(sunColor,1)) {
        //interpolate between day and night colors depending on the y position of the sun
        outColor = mix(outColor, dayOut, sunDir.y);
        outColor = mix(vec4(1), outColor, clamp(uv.y / 0.5 - 0.4, 0, 1));


    }




}
