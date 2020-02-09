#version 150

uniform sampler2D tex;
uniform ivec2 sourceSize;
uniform float bluryness;
noperspective in vec2 texCoord;

out vec3 fragColor;

void main()
{
	vec2 pixel = texCoord * sourceSize + 0.5;
	vec2 frac = fract(pixel);
	if(frac.x >= 0.5) frac.x = 1 - (1 - frac.x) * bluryness; else frac.x *= bluryness;
	if(frac.y >= 0.5) frac.y = 1 - (1 - frac.y) * bluryness; else frac.y *= bluryness;

	fragColor=texture(tex, (floor(pixel) + frac - 0.5) / sourceSize, 0).rgb;
}
