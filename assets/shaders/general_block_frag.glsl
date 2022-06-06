#version 460 core

out vec4 FragColor;

struct Light
{
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 viewPos;
uniform Light light;
uniform sampler2D diffuseMap;

void main()
{
    // ambient
    vec3 ambient = light.ambient * texture(diffuseMap, TexCoords).rgb;
  	
    // diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * texture(diffuseMap, TexCoords).rgb;

    vec3 result = ambient + diffuse;
    FragColor = vec4(result, 1.0);
}