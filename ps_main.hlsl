struct VS_OUTPUT
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float2 texCoords : TEXCOORD0;
	float3 worldPos : TEXCOORD1;
};

cbuffer CBufferPerFrame
{
	float totalTime;
	float3 eyePos;
	row_major float4x4 world;
	row_major float4x4 perspective;
	float4 ambient; // Red Green blue and A = light intensty 
	float4 diffuse; // Red Green blue and a = how transparent it is
	float4 specular;
	float4 fogColour;
	float2 specularPower;
	float fogStart;
	float fogRange;

};

struct DirectionalLight
{
	float4 colour; //intensity of the Directional Light is the alpha variable of the colour
	float3 direction;

};

struct PointLight
{
	float4 colour; //intensity of the Point Light is the alpha variable of the colour
	float3 position;
	float radius;
};

struct SpotLight
{
	float4 colour;
	float3 position;
	float radius;
	float3 direction;
	float innerRadius;
	float fallofRadius;
};

#define MAX_LIGHTS 64

cbuffer CBufferLighting
{
	DirectionalLight directionalLight;
	PointLight pointLights[MAX_LIGHTS];
	SpotLight spotLights[MAX_LIGHTS];
};


float4 debug(VS_OUTPUT input)
{
    //float3 output = float3(1.0, 1.0, 1.0);
    //output.r = sin(totalTime.r); 
    //output.g = cos(totalTime.r); 
    //output.b = sin(totalTime.r); 

    float4 output = (float4) 0;
    float4 colour = diffuse; //float4(1.0, 1.0, 1.0, 1.0); //Sampled from Texture

	float3 eyeDir = eyePos - input.worldPos.xyz;

	//Fog
	float fogAmount = saturate((length(eyeDir) - fogStart) / (fogRange));
	eyeDir = normalize(eyeDir);

    DirectionalLight l_Dir = directionalLight;
    float3 normal = input.normal;
    float3 lightDirection = -normalize(l_Dir.direction);
    float n_dot_l = dot(lightDirection, normal);
    
    float3 a = ambient.rgb * ambient.a * colour.rgb;
    float3 d = (float3) 0;
    float3 s = (float3) 0;

    if (n_dot_l > 0)
    {
        //Directional Light
        d = l_Dir.colour.rgb * l_Dir.colour.a * n_dot_l * colour.rgb;

        // R = 2 * (n.l) * n - l
        float3 r = normalize(2 * n_dot_l * normal - lightDirection);
        
        // s = r.v^p
		s = specular.rgb * specular.a * min(pow(saturate(dot(r, eyeDir)), specularPower.r), colour.a); // * l_Dir.colour.a;
    }

    //Point Lights    
    for (int i = 0; i < MAX_LIGHTS; i++)
    {
        PointLight l_Point0 = pointLights[i];

        float3 pointLightDirection = input.worldPos - l_Point0.position;
        float attenuation = saturate(1.0 - (length(pointLightDirection) / l_Point0.radius));
        pointLightDirection = normalize(pointLightDirection);

        n_dot_l = dot(pointLightDirection, normal);
        if (n_dot_l > 0)
        {
            float3 r = normalize(2 * n_dot_l * normal - pointLightDirection);
            d += l_Point0.colour.rgb * l_Point0.colour.a * colour.xyz * attenuation;
            s += specular.rgb * specular.a * min(pow(saturate(dot(r, eyeDir)), specularPower.r), colour.a) * attenuation;
        }
    }

	//Spot Lights    
	for (int i = 0; i < MAX_LIGHTS; i++)
	{
		SpotLight l_Spot = spotLights[i];

		float3 lightDir = input.worldPos - l_Spot.position;
		float attenuation = saturate(1.0 - (length(lightDir) / l_Spot.radius));
		lightDir = -normalize(lightDir);

		n_dot_l = dot(lightDir, normal);
		if (n_dot_l > 0)
		{
			float spotFactor = 0.0f;
			float lightAngle = dot(normalize(l_Spot.direction), lightDir);
			
			if (lightAngle > 0.0f)
			{
				spotFactor = smoothstep(l_Spot.fallofRadius, l_Spot.innerRadius, lightAngle);
			}

			float3 r = normalize(2 * n_dot_l * normal - lightDir);
			d += l_Spot.colour.rgb * l_Spot.colour.a * colour.xyz * attenuation * spotFactor;
			s += specular.rgb * specular.a * min(pow(saturate(dot(r, eyeDir)), specularPower.r), colour.a) * attenuation * spotFactor;
		}
	}
    
	output.rgb = a + d + s;
    output.rgb = lerp(output.rgb, fogColour.rgb, fogAmount) ;
    output.a = colour.a;
    
    return output;
}

float4 main(VS_OUTPUT input) : SV_TARGET
{
    return debug(input); 
	/*
	//float3 output;
	//output.r = sin(totalTime);
	//output.g = cos(totalTime);
	//output.b = sin(totalTime);

	float4 output = (float4)0.0f;
	float4 colour = diffuse;

	DirectionalLight l_Dir = directionalLight;
	//l_Dir.direction = normalize(float3(sin(totalTime.x), 0.5, cos(totalTime.x)));
	//l_Dir.colour = normalize(float4(0.2, 0.7, 0.3, 0.8));

	//l_Point0.radius = 3.0f;
	//l_Point0.colour = float4(1.0f, 0.0f, 0.0f, 1.0f);
	//l_Point0.position = float3(-2.0f, 0.0f, 0.0f);
	
	float3 normal = input.normal;
	float3 lightDirection = -normalize(l_Dir.direction);
	float n_dot_l = dot(lightDirection, normal);

	float3 a = ambient.rgb * ambient.a * colour.rgb; //ambient
	float3 d = (float3)0; //diffuse
	float3 s = (float3)0; //specular


	if (n_dot_l > 0)
	{
		d = l_Dir.colour.rgb * l_Dir.colour.a * n_dot_l * colour.rgb; // Direction light

		//Phong Model
		//R = 2 * (n.l) * n - l
		float3 r = normalize(2 * n_dot_l * normal - lightDirection);

		//s = r.v^p
		s = specular.rgb * specular.a * min(pow(saturate(dot(r, input.eyeDir)), specularPower.r), colour.a);
		//Saturate = is to clamp the number between 0 to 1 
	}

	//Point Lights
	for (int i = 0; i < MAX_LIGHTS; i++)
	{
		PointLight l_point0 = pointLight[i];

		float3 pointLightDirection = input.worldPos - l_point0.position;
		float attenuation = saturate(1.0f - (length(pointLightDirection) / l_point0.radius));
		pointLightDirection = normalize(pointLightDirection);

		n_dot_l = dot(pointLightDirection, normal);
		if (n_dot_l > 0)
		{
			float3 r = normalize(2 * n_dot_l * normal - pointLightDirection);

			d += l_point0.colour.rgb * l_point0.colour.a * n_dot_l * colour.rgb * attenuation;
			s += specular.rgb * specular.a * min(pow(saturate(dot(r, input.eyeDir)), specularPower.r), colour.a) * attenuation;
		}
	}

	output.rgb = a + d + s;
	output.a = colour.a;

	return output;
	*/
}