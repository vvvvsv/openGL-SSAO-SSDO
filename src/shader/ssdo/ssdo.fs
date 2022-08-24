#version 330 core
out vec3 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D texNoise;
uniform samplerCube skybox;

uniform vec3 samples[32];

// parameters
int kernelSize = 32;
float radius = 0.5;

// tile noise texture over screen based on screen dimensions divided by noise size
const vec2 noiseScale = vec2(800.0/4.0, 800.0/4.0);

uniform mat4 projection;
uniform mat4 iview; // inverse of view to world-space

void main()
{
    // get input for SSDO algorithm
    vec3 fragPos = texture(gPosition, TexCoords).xyz;
    vec3 normal = normalize(texture(gNormal, TexCoords).rgb);
    vec3 randomVec = normalize(texture(texNoise, TexCoords * noiseScale).xyz);
    // create TBN change-of-basis matrix: from tangent-space to view-space
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    // iterate over the sample kernel and calculate SSDO's indirect light
    vec3 directLight = vec3(0.0, 0.0, 0.0);
    vec3 indirectLight = vec3(0.0, 0.0, 0.0);
    for(int i = 0; i < kernelSize; ++i)
    {
        // get sample position
        vec3 samplePos = TBN * samples[i]; // from tangent to view-space
        samplePos = fragPos + samplePos * radius;

        // project sample position (to sample texture) (to get position on screen/texture)
        vec4 offset = vec4(samplePos, 1.0);
        offset = projection * offset; // from view to clip-space
        offset.xyz /= offset.w; // perspective divide
        offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0

        // get sample info
        float sampleDepth = -texture(gPosition, offset.xy).w; // get depth value of kernel sample
		vec3 samplePos1 = texture(gPosition, offset.xy).xyz;
        vec3 sampleNormal = texture(gNormal, offset.xy).rgb;
		vec3 sampleColor = texture(gAlbedo, offset.xy).xyz;

        // range check & accumulate
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
        if (sampleDepth >= samplePos.z)
        {
    		indirectLight += rangeCheck * max(dot(sampleNormal, normalize(fragPos - samplePos1)), 0.0) * sampleColor;
        }
        else
        {
            vec4 skyboxDirection = iview * vec4(samplePos - fragPos, 0.0);
			vec3 skyboxColor = texture(skybox, skyboxDirection.xyz).xyz;
			directLight += rangeCheck * skyboxColor * dot(normal, normalize(samplePos - fragPos));
        }
    }
    directLight = 0.5 * (directLight / kernelSize);
    indirectLight = 5.0 * (indirectLight / kernelSize);

    FragColor = directLight + indirectLight;
}