#version 410 core

in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoords;
in vec4 fragPosLightSpace;

out vec4 fColor;

//matrices
uniform mat4 model;
uniform mat4 view;
uniform mat3 normalMatrix;


//lighting
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 spotLightPos;
uniform vec3 spotLightDir;
uniform float cutoff;
uniform float outerCutoff;
uniform bool isNight;
uniform vec3 pointLightLocation;
uniform vec3 pointLightLocation2;
uniform vec3 pointLightLocation3;

float spotQuadratic = 0.0028f;
float spotLinear = 0.027f;
float spotConstant = 0.5f;

vec3 spotLightAmbient = vec3(0.0f, 0.0f, 0.0f);
vec3 spotLightSpecular = vec3(1.0f, 1.0f, 1.0f);
vec3 spotLightColor = vec3(12,12,12);
float shininess = 64.0f;

// textures
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shadowMap;

//fog
uniform bool foginit;
uniform float fogDensity;

//components
vec3 ambient;
float ambientStrength = 0.1f;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.5f;
float ambientPoint = 0.5f;
float specularStrengthPoint = 0.9f;
float shininessPoint = 64.0f;
float constant = 0.01f;
float linear = 0.22f;
float quadratic = 0.2f;

void computeDirLight()
{
    //compute eye space coordinates
    vec4 fPosEye = view * model * vec4(fPosition, 1.0f);
    vec3 normalEye = normalize(normalMatrix * fNormal);

    //normalize light direction
    vec3 lightDirN = vec3(normalize(view * vec4(lightDir, 0.0f)));

    //compute view direction (in eye coordinates, the viewer is situated at the origin
    vec3 viewDir = normalize(- fPosEye.xyz);

    //compute ambient light
    ambient = ambientStrength * lightColor;

    //compute diffuse light
    diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;
    if(!isNight)
        diffuse = 0.7f *diffuse;

    //compute specular light
    vec3 reflectDir = reflect(-lightDirN, normalEye);
    float specCoeff = pow(max(dot(viewDir, reflectDir), 0.0f), 32);
    specular = specularStrength * specCoeff * lightColor;

    if(isNight){
        diffuse = vec3(0, 0, 0);
        specular = vec3(0.01, 0.01, 0.01);
    }
}

float computeShadow()
{
	//perform perspective divide
	vec3 normalizedCoords= fragPosLightSpace.xyz / fragPosLightSpace.w;

	//tranform from [-1,1] range to [0,1] range
	normalizedCoords = normalizedCoords * 0.5 + 0.5;

	//get closest depth value from lights perspective
	float closestDepth = texture(shadowMap, normalizedCoords.xy).r;

	//get depth of current fragment from lights perspective
	float currentDepth = normalizedCoords.z;

	//if the current fragments depth is greater than the value in the depth map, the current fragment is in shadow 
	//else it is illuminated
	//float shadow = currentDepth > closestDepth ? 1.0 : 0.0;
	float bias = 0.001f;
	//float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
	if (normalizedCoords.z > 1.0f)
		return 0.0f;
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -3; x <= 3; ++x){
        for(int y = -3; y <= 3; ++y){
            float pcfDepth = texture(shadowMap, normalizedCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 49.0;
    return shadow;
}

float computeFog(){
    vec4 fPosEye = view * model * vec4(fPosition, 1.0f);
    float fragmentDistance = length(fPosEye);
    float fogFactor = exp(-pow(fragmentDistance * fogDensity, 2));
    return clamp(fogFactor, 0.0f, 1.0f);
}

vec3 computePointLight(vec3 pointLightLocation){
    vec3 lightColor = vec3(1.0f, 0.474f, 0.301f); 
    vec4 fPosEye = view * model * vec4(fPosition, 1.0f);
    vec3 normalEye = normalize(normalMatrix * fNormal);
    vec3 lightDirN = vec3(normalize(view * vec4(pointLightLocation, 0.0f)));
    vec3 viewDirN = normalize(vec3(0.0f) - fPosEye.xyz);
    vec3 ambient = ambientPoint * lightColor;
	vec3 diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;
	vec3 halfVector = normalize(lightDirN + viewDirN);
	vec3 reflection = reflect(-lightDirN, normalEye);
    float specCoeff = pow(max(dot(normalEye, halfVector), 0.0f), shininessPoint);
	vec3 specular = specularStrengthPoint * specCoeff * lightColor;
	float distance = length((view * vec4(pointLightLocation, 1.0f)).xyz - fPosEye.xyz);
	float att = 1.0f / (constant + linear * distance + quadratic * distance * distance);
	return (ambient + diffuse + specular) * att * vec3(7.5f,7.5f,7.5f);
}

vec3 computeSpotLight(){
    vec3 lightDir = normalize(spotLightPos - fPosition);
    vec4 fPosEye = view * model * vec4(fPosition, 1.0f);

    float theta = dot(lightDir, normalize(-spotLightDir));
    float epsilon = cutoff - outerCutoff;
    float intensity = clamp((theta - outerCutoff) / epsilon, 0.0, 1.0);

    vec3 normalEye = normalize(normalMatrix * fNormal);
    vec3 lightDirN = vec3(normalize(view * vec4(lightDir, 0.0f)));
    vec3 viewDir = normalize(- fPosEye.xyz);
    vec3 viewDirN = normalize(vec3(0.0f) - fPosEye.xyz);
    vec3 halfVector = normalize(lightDirN + viewDirN);

    float diff = max(dot(fNormal, lightDir), 0.0f);
	float spec = pow(max(dot(normalEye, halfVector), 0.0f), shininess);
	float dist = length(spotLightPos - fPosition);
	float attenuation = 1.0f / (spotConstant + spotLinear * dist + spotQuadratic * dist * dist);
        
    vec3 ambient = attenuation * intensity * spotLightColor * spotLightAmbient * texture(diffuseTexture, fTexCoords).rgb;
    vec3 specular = attenuation * intensity * spotLightColor * spotLightSpecular * spec * texture(specularTexture, fTexCoords).rgb;
    vec3 diffuse = attenuation * intensity * spotLightColor * spotLightSpecular * diff * texture(diffuseTexture, fTexCoords).rgb;

    return ambient + specular + diffuse;
}

void main() 
{
    computeDirLight();
    vec3 color;
    vec3 lightVal = vec3(1.0f, 1.0f, 1.0f);

    float fogFactor = computeFog();
	vec4 fogColor = vec4(0.3f, 0.3f, 0.3f, 1.0f);

    if(!isNight){
        ambient *= texture(diffuseTexture, fTexCoords).rgb;
	    diffuse *= texture(diffuseTexture, fTexCoords).rgb;
	    specular *= texture(specularTexture, fTexCoords).rgb;
        float shadow = computeShadow();
        color = min((ambient + (1.0f - shadow) * diffuse) + (1.0f - shadow) * specular, 1.0f);
    }
    else{
        color = min((ambient + diffuse) * texture(diffuseTexture, fTexCoords).rgb + specular * texture(specularTexture, fTexCoords).rgb, 1.0f);
        lightVal= computeSpotLight();
    }
    lightVal += computePointLight(pointLightLocation) 
                + computePointLight(pointLightLocation2)
                + computePointLight(pointLightLocation3);

    if (!foginit || isNight){
        fColor = min(vec4(color, 1.0f) * vec4(lightVal, 1.0f), 1.0f);
    }else{
        fColor = mix(fogColor, min(vec4(color, 1.0f) * vec4(lightVal, 1.0f), 1.0f), fogFactor);
    }
}
