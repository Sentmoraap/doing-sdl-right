#version 150

uniform sampler2D tex;
uniform ivec2 sourceSize;
//uniform vec2 bc;
noperspective in vec2 texCoord;

out vec3 fragColor;

void main()
{
	vec2 bc = vec2(0, 0.5);
	vec2 pixel = texCoord * sourceSize + 0.5;
	vec2 frac = fract(pixel);
	vec2 frac2 = frac * frac;
	vec2 frac3 = frac * frac2;
	vec2 onePixel = 1.f / sourceSize;
	pixel = floor(pixel) / sourceSize - onePixel / 2;
	vec3 colours[4];
	// 16 reads, unoptimized but forks for every Mitchell-Netravali filter
	for(int i = -1; i <= 2; i++)
	{
		vec3 p0 = texture(tex, pixel + vec2(   -onePixel.x, i * onePixel.y)).rgb;
		vec3 p1 = texture(tex, pixel + vec2(             0, i * onePixel.y)).rgb;
		vec3 p2 = texture(tex, pixel + vec2(    onePixel.x, i * onePixel.y)).rgb;
		vec3 p3 = texture(tex, pixel + vec2(2 * onePixel.x, i * onePixel.y)).rgb;
		colours[i + 1] = ((-bc.x / 6 - bc . y) * p0 + (- 1.5 * bc.x - bc.y + 2) * p1 
				+ (1.5 * bc.x + bc.y - 2) * p2 + (bc.x / 6 + bc.y) * p3) * frac3.x
				+ ((0.5 * bc.x + 2 * bc.y) * p0 + (2 * bc.x + bc.y - 3) * p1
				+ (-2.5 * bc.x - 2 * bc.y + 3) * p2 - bc.y * p3) * frac2.x
				+ ((-0.5 * bc.x - bc.y) * p0 + (0.5 * bc.x + bc.y) * p2) * frac.x
				+ p0 * bc.x / 6 + (-bc.x / 3 + 1) * p1 + p2 * bc.x / 6;
	}
	fragColor = ((-bc.x / 6 - bc . y) * colours[0] + (- 1.5 * bc.x - bc.y + 2) * colours[1] 
			+ (1.5 * bc.x + bc.y - 2) * colours[2] + (bc.x / 6 + bc.y) * colours[3]) * frac3.y
			+ ((0.5 * bc.x + 2 * bc.y) * colours[0] + (2 * bc.x + bc.y - 3) * colours[1]
			+ (-2.5 * bc.x - 2 * bc.y + 3) * colours[2] - bc.y * colours[3]) * frac2.y
			+ ((-0.5 * bc.x - bc.y) * colours[0] + (0.5 * bc.x + bc.y) * colours[2]) * frac.y
			+ colours[0] * bc.x / 6 + (-bc.x / 3 + 1) * colours[1] + colours[2] * bc.x / 6;
}
