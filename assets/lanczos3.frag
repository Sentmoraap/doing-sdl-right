#version 150

uniform sampler2D tex;
uniform ivec2 sourceSize;
noperspective in vec2 texCoord;

out vec3 fragColor;

float lanczos3(float x)
{
	x = max(abs(x), 0.00001);
	float val = x * 3.142592654;
	return sin(val) * sin(val / 3) / (val * val);
}

void main()
{
	vec2 pixel = texCoord * sourceSize + 0.5;
	vec2 frac = fract(pixel);
	vec2 onePixel = 1.f / sourceSize;
	pixel = floor(pixel) / sourceSize - onePixel / 2;
	float lanczosX[6];
	float sum = 0;
	for(int x = 0; x < 6; x++)
	{
		lanczosX[x] = lanczos3(x - 2 - frac.x);
		sum += lanczosX[x];
	}
	for(int x = 0; x < 6; x++) lanczosX[x] /= sum;
	sum = 0;
	float lanczosY[6];
	for(int y = 0; y < 6; y++)
	{
		lanczosY[y] = lanczos3(y - 2 - frac.y);
		sum += lanczosY[y];
	}
	for(int y = 0; y < 6; y++) lanczosY[y] /= sum;
	fragColor = vec3(0);
	for(int y = -2; y <= 3; y++)
	{
		vec3 colour = vec3(0);
		for(int x = -2; x <= 3; x++)
				colour += texture(tex, pixel + vec2(x * onePixel.x, y * onePixel.y)).rgb * lanczosX[x + 2];
		fragColor += colour * lanczosY[y + 2];
	}
}
