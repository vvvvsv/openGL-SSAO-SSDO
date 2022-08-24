#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

// texture samplers
uniform sampler2D textureImg;

void main()
{
	FragColor = texture(textureImg, TexCoord);
}