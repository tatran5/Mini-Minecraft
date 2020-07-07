#version 150
//// ^ Change this to version 130 if you have compatibility issues

//// Refer to the lambert shader files for useful comments

uniform int u_Time; // for animatable objects
uniform vec4 u_Color; // The color with which to render this instance of geometry.

uniform sampler2D u_Sampler2DTexture; // data of the texture image
uniform sampler2D u_Sampler2DNormal; // data of the normal image
uniform sampler2D m_depthMap;

in vec4 fs_LightVec;
in vec4 fs_CameraPos;
in vec4 fs_Pos;
in vec4 fs_Nor;
in vec4 fs_Col;
in vec2 fs_UV;
in float fs_CosPow;
in float fs_Animatable;
in vec4 fs_PosLightSpace;

out vec4 out_Col;

float shadowCalculation() {
    //    // perspective divide. Returns fragment's light-space position in the range [-1, 1]
    //    vec3 projCoords = fs_PosLightSpace.xyz / fs_PosLightSpace.w;
    //    // Because the depth from the depth map is in the range [0,1] and we also want to
    //    // use projCoords to sample from the depth map so we transform the NDC coordinates
    //    // to the range [0,1]
    //    projCoords = projCoords * 0.5 + 0.5;
    //    // Sample the depth map as the resulting [0,1] coordinates from projCoords
    //    // directly correspond to the transformed NDC coordinates from the first render pass.
    //    float closestDepth = texture(shadowMap, projCoords.xy).r;
    //    float currentDepth = projCoords.z;
    //    // helps to prevent shadow acne. Bias change based on surface angle towards the light
    //    float bias = max(0.05 * (1.0 - dot(fs_Nor, fs_LightVec)), 0.005);
    //    // Check whether currentDepth is higher than closestDepth (fragment is in shadow0
    //    float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
    //    // In case a projected coordinate is further than the light's far plane when
    //    // its z coordinate is larger than 1.0, making GL_CLAMP_TO_BORDER wrapping method
    //    // not work anymore as we compare the coordinate's z component with the depth map values
    //    // which this always returns true for z larger than 1.0. Solution for this is below
    //    if(projCoords.z > 1.0) {shadow = 0.0;}
    //    return shadow;
    // perform perspective divide
    vec3 projCoords = fs_PosLightSpace.xyz / fs_PosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(m_depthMap, projCoords.xy).r;
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // check whether current frag pos is in shadow
    float shadow = currentDepth > closestDepth  ? 1.0 : 0.0;

    return shadow;
}

void main()
{


    // Get specularIntensity, used for out_Col --------------------------
    // Make the lighter-colored part of the texture shinier
    // The closer the color of the fragment is to white, the shinier it is
    // Only want to have this effect if the object is supposed to be shiny (ex: metal, not dirt)
    vec4 diffuseColor = texture(u_Sampler2DTexture, fs_UV);

    float increaseInCosPow = 1.f;
    if (fs_CosPow >= 100) {
        float distFromWhite = length(vec3(1, 1, 1) - diffuseColor.rgb);
        increaseInCosPow =  2000 * distFromWhite;
    }

    // TODO Homework 4


    // Material base color (before shading)
    float timeFloat = float(u_Time);
    float disp = sin(timeFloat * 0.0007);
    float dispx = cos(timeFloat * 0.0007);


    //give the position of the center of the sun
    vec4 sunDir = vec4(normalize(vec3(0, disp, dispx)), 1);


    // Calculate the diffuse term for Lambert shading
    float diffuseTerm = dot(normalize(fs_Nor), normalize(sunDir));
    // Avoid negative lighting values
    diffuseTerm = clamp(diffuseTerm, 0, 1);
    vec4 H = (normalize(fs_CameraPos - fs_Pos) + normalize(sunDir)) * 0.5;

    float specularIntensity = max(pow(dot(H, fs_Nor), fs_CosPow), 0);
    float ambientTerm = 0.3;
    if (fs_Animatable > 0) {
        //animate by shifting the UV
        float numTexturePerRow = 16.f; // 16 blocks texture in the texture imagea
        float shiftX = fract(float(u_Time) / 100.f) / numTexturePerRow;
        diffuseColor = texture(u_Sampler2DTexture, vec2(fs_UV.x + shiftX, fs_UV.y));
    } else {
        diffuseColor = texture(u_Sampler2DTexture, fs_UV);
    }
    float shadow = shadowCalculation();
    float lightIntensity = (1 - shadow) * (diffuseTerm + specularIntensity)
            + ambientTerm;   //Add a small float value to the color multiplier
    //to simulate ambient lighting. This ensures that faces that are not
    //lit by our point light are not completely black.
    diffuseColor.rgb *= lightIntensity;

    float distance = length(fs_CameraPos.xz - fs_Pos.xz) / 75;
    diffuseColor.xyz = mix(diffuseColor.xyz - 0.3, u_Color.xyz, clamp(distance, 0, 1));



    // Compute final shaded color
    out_Col = diffuseColor;

}
