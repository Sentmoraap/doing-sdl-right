#version 150

#define NB_ITERATIONS 4

uniform sampler2D tex;
uniform ivec2 destSize;
uniform ivec2 sourceSize;
uniform int nbIterations;
uniform float coverageMult;

noperspective in vec2 texCoord;

out vec3 fragColor;

void main()
{
	vec2 pixelCoverage = coverageMult * destSize / vec2(sourceSize);
    fragColor = vec3(0, 0, 0);
    vec2 pixelCoords = texCoord * sourceSize;
    pixelCoords -= 0.5 / pixelCoverage;
    ivec2 iPixelCoords = ivec2(floor(pixelCoords));
    vec2 modPixelCoords = mod(pixelCoords, 1);
    float totalXCoverage = 0;
    for(int ix = 0; ix < nbIterations; ix++)
    {
        vec3 yColor = vec3(0, 0, 0);
        float totalYCoverage = 0;
        for(int iy = 0; iy < nbIterations; iy++)
        {
            float currentYCoverage = 0;
            currentYCoverage = iy == 0 ? (1 - modPixelCoords.y) : 1.f;
            currentYCoverage *= pixelCoverage.y;
            if(currentYCoverage + totalYCoverage >= 1) currentYCoverage = 1 - totalYCoverage;
            totalYCoverage += currentYCoverage;
            yColor += currentYCoverage * texelFetch(tex, iPixelCoords + ivec2(ix, iy), 0).rgb;
        }
        float currentXCoverage = 0;
        currentXCoverage = ix == 0 ? (1 - modPixelCoords.x) : 1.f;
        currentXCoverage *= pixelCoverage.x;
        if(currentXCoverage + totalXCoverage >= 1) currentXCoverage = 1 - totalXCoverage;
        totalXCoverage += currentXCoverage;
        fragColor += currentXCoverage * yColor;
    }
}
