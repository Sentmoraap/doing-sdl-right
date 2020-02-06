#version 150

out vec4 fragColor;

void main()
{
	fragColor = vec4(0, 0, 0, 0.5);
	float colorMult = 0.5;
	float funcMult = 0.01;
	ivec2 coords = ivec2(gl_FragCoord.xy * vec2(1024, 768));
	if((coords.x + coords.y) % 2 == 0)
	{
		for(int i = 0; i < 100; i++)
		{
			fragColor.r += colorMult * cos(gl_FragCoord.x * funcMult);
			fragColor.g += colorMult * sin(gl_FragCoord.x * funcMult);
			fragColor.b += log(abs(fragColor.r) + 1);
			colorMult *= 0.5;
			funcMult *= 3;
		}
	}
	else
	{
		for(int i = 0; i < 101; i++)
		{
			fragColor.r += colorMult * sin(gl_FragCoord.x * funcMult);
			fragColor.g += colorMult * cos(gl_FragCoord.x * funcMult);
			fragColor.b += log(abs(fragColor.r) + 0.5);
			colorMult *= 0.6;
			funcMult *= 2;
		}
	}
}
