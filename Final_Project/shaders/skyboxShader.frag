#version 410 core

in vec3 textureCoordinates;
out vec4 color;

uniform samplerCube skybox;

uniform bool foginit;
uniform bool isNight;

void main()
{
    if(!foginit && !isNight){
        color = texture(skybox, textureCoordinates);
    }else if(foginit && !isNight){
        color = mix(vec4(0.3f, 0.3f, 0.3f, 1.0f), texture(skybox, textureCoordinates), 0.0001f);
    }else{
    color = mix(vec4(0, 0, 0, 1.0f), texture(skybox, textureCoordinates), 0.0001f);
    }

}
