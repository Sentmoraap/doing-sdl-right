#version 150

in vec2 pos;
in vec2 textureCoord;

noperspective out vec2 texCoord;

void main()
{
	gl_Position=vec4(pos,0.5,1);
	texCoord=textureCoord;
}
