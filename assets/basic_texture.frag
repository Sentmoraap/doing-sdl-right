#version 150

uniform sampler2D tex;
noperspective in vec2 texCoord;

out vec3 fragColor;

void main()
{
	fragColor=texture(tex,texCoord,0).rgb;
}
