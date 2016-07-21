varying vec4 vPosition;
varying vec3 eyePosition;

// For samplers
uniform sampler2D RayStart;
uniform sampler2D RayStop;
uniform sampler3D Volume;
uniform sampler2D TransferFunc_01;
uniform sampler2D TransferFunc_02;
uniform sampler2D TransferFunc_03;
uniform sampler2D TransferFunc_04;

// For ray caster
uniform float StepLength;
uniform float Threshold;
uniform int NSamples;
uniform float NoiseDelta;
uniform float GradientScale;
uniform float GradientDelta;
uniform float TransScale;

// For orientation visualizing transfer function
uniform float FMin;
uniform float FMax;
vec3 F = vec3(0);

// For linear interpolation of alpha in gradient
uniform float Alpha;

// For xtoon
uniform float r;
uniform float s;
uniform float BacklightDetail;
uniform int xtoonFlag;
uniform int xtoonTypeTone;
uniform int xtoonTypeSpec;
uniform int xtoonTypeBack;

// For lighting
uniform vec3 LightPosition;
vec4 spec_val = vec4(0);
vec4 backlight_val = vec4(0);

// To calculate difference, to sharpen the image
const float sizes = 256.0;
const vec4 mask = vec4(1.0, 0.0, 0.0, 0.0);
vec4 e = vec4(vec3(-1.0, 1.0, 0.0)/sizes, 0.0);

// For Sobel operator
vec4 g_x = vec4(0);
vec4 g_y = vec4(0);
vec4 g_z = vec4(0);
vec4 sobelResult = vec4(0);

// For contours
vec3 prevNormal = vec3(0.0, 0.0, 1.0);
uniform float Thickness;

// For accumlators
vec4 gradient_acc = vec4(0);
vec4 trans_acc = vec4(0);
float alpha_acc = 0.0; 

// For color blending and result
float alpha_sample = 0.0;
vec4 trans_val = vec4(0);
vec4 trans_sample = vec4(0);
vec4 color_sample = vec4(0);
vec4 dst = vec4(0);

// Stochastic Jittering
uniform int Jitter;

vec3 linearMapF(vec3 scalar, float minF, float maxF)
{
	vec3 result = vec3(0);

	if((scalar.x < minF) && (scalar.y < minF) && (scalar.z < minF))
	{
		result = vec3(0.0, 0.0, 0.0);
	}
	else if((scalar.x >= minF) && (scalar.x <= maxF) &&
			(scalar.y >= minF) && (scalar.y <= maxF) &&
			(scalar.z >= minF) && (scalar.z <= maxF))
	{
		result = (scalar.xyz - minF) / (maxF - minF);
	}
	else if((scalar.x > maxF) && (scalar.y > maxF) && (scalar.z > maxF))
	{
		result = vec3(1.0, 1.0, 1.0);
	}

	return result;
}

float lookup(vec3 coord)
{
    vec3 V = texture3D(Volume, coord).xyz;
    return dot(V, V);
}

float rand(vec2 num)
{        
	return fract(sin(dot(num.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

float xtoonToneDetail(float densityC, float tempDensity)
{
	float D;
	float densityMin = 0.01;
	float densityMax = densityMin * r;
	float density = tempDensity;

	float densityMinM = densityC - densityMin;
	float densityMinP = densityC + densityMin;

	float densityMaxM = densityC - densityMax;
	float densityMaxP = densityC + densityMax;

	D = 1.0 - log(density / densityMin) / log(densityMax - densityMin);

	if(density < densityMinM)
	{
		densityMin = densityC - densityMin;
		densityMax = densityC - r * densityMin;
		D = 1.0 - log(density / densityMinM) / log(densityMaxM / densityMinM);
	}
	else if(density > densityMinP)
	{
		densityMin = densityC + densityMin;
		densityMax = densityC + r * densityMin;
		D = log(density / densityMaxP) / log(densityMinP / densityMaxP);		
	}
			
	return D;
}

float xtoonSpecular(float VR)
{
	float D;

	D = pow(VR, s);	
	
	return D;
}

float xtoonBacklight(float NV)
{
	float D;
	
	D = pow(NV, BacklightDetail);
		
	return D;
}

vec4 lookupTransferTex(float a, float indexX, float indexY)
{
	vec4 result = vec4(0);

	//if(a > 0.0 && a < 0.33)
		result = texture2D(TransferFunc_02, vec2(indexX, indexY));
	//else if(a > 0.33 && a < 0.66)
		//result = texture2D(TransferFunc_01, vec2(indexX, indexY));
	//else if(a > 0.66 && a < 0.99)
		//result = texture2D(TransferFunc_03, vec2(indexX, indexY));
	//else 
		//result = texture2D(TransferFunc_04, vec2(indexX, indexY));

	return result;
}

void main()
{
    vec2 coord = (vPosition.xy / vPosition.w + 1.0) / 2.0;
    vec3 rayStart = texture2D(RayStart, coord).xyz;
    vec3 rayStop = texture2D(RayStop, coord).xyz;

    if (rayStart == rayStop)
	{
        discard;
        return;
    }

    vec3 ray = rayStop - rayStart;
    float rayLength = length(ray);
    vec3 stepVector = StepLength * ray / rayLength;

	// Stochastic jittering 
	if(Jitter == 1)
		rayStart += stepVector * rand(vPosition.xy / NoiseDelta);

    vec3 pos = rayStart;

    while (dst.a < 1.0 && rayLength > 0.0) 
	{
        float V = lookup(pos);

        if (V > Threshold) 
		{
			// Compute Gradient
			g_x = 2.0 * abs(texture3D(Volume, pos+e.yww)-texture3D(Volume, pos+e.xww))
						+ abs(texture3D(Volume, pos+e.yxw)-texture3D(Volume, pos+e.xxw))
						+ abs(texture3D(Volume, pos+e.yyw)-texture3D(Volume, pos+e.xyw)) 
						+ abs(texture3D(Volume, pos+e.ywx)-texture3D(Volume, pos+e.xwx))
						+ abs(texture3D(Volume, pos+e.ywy)-texture3D(Volume, pos+e.xwy));

			g_y = 2.0 * abs(texture3D(Volume, pos+e.wyw)-texture3D(Volume, pos+e.wxw))
						+ abs(texture3D(Volume, pos+e.xyw)-texture3D(Volume, pos+e.xxw)) 
						+ abs(texture3D(Volume, pos+e.yyw)-texture3D(Volume, pos+e.yxw))
						+ abs(texture3D(Volume, pos+e.wyx)-texture3D(Volume, pos+e.wxx))
						+ abs(texture3D(Volume, pos+e.wyy)-texture3D(Volume, pos+e.wxy));

			g_z = 2.0 * abs(texture3D(Volume, pos+e.wwy)-texture3D(Volume, pos+e.wwx))
						+ abs(texture3D(Volume, pos+e.xwy)-texture3D(Volume, pos+e.xwx)) 
						+ abs(texture3D(Volume, pos+e.ywy)-texture3D(Volume, pos+e.ywx))
						+ abs(texture3D(Volume, pos+e.wxy)-texture3D(Volume, pos+e.wxx))
						+ abs(texture3D(Volume, pos+e.wyy)-texture3D(Volume, pos+e.wyx));


			sobelResult = mask.xwww * g_x + mask.wxww * g_y + mask.wwxw * g_z + mask.wwwx;
			gradient_acc += abs(sobelResult);

            // Refine the hitpoint to reduce slicing artifacts
            vec3 s = -stepVector * 0.5;
            pos += s; 
			V = lookup(pos);

			for(int i = 0; i < NSamples; i++)
			{
				if (V > Threshold)
				{	 
					s *= 0.5; 
					pos += s; 
					V = lookup(pos);
				}
				else 
				{
					s *= -0.5;
					pos += s; 
					V = lookup(pos);
				}			
			}
				            
			if (V > Threshold)
			{
				// Gradient
				vec3 vGradient = gradient_acc.rgb * GradientDelta;
				vec3 nGradient = normalize(vGradient);				
				float GradientMag = length(vGradient);

				// Lighting
				vec3 viewDir = -normalize(eyePosition - pos);
                vec3 normal = normalize(gl_NormalMatrix * nGradient);
                float NdotL = dot(normal, LightPosition);
				float NdotV = dot(normal, viewDir);
				vec3 reflection = normalize(((2.0 * normal) * NdotL) - LightPosition);
				float VdotR = max(0.0, dot(viewDir,reflection));

				// Linear Mapping of Alpha values
				F = linearMapF(texture3D(Volume, pos).rgb, FMin, FMax);
				float FMag = length(F);
				
				alpha_sample = (1.0 - alpha_acc) * GradientMag + (alpha_acc * FMag);
				alpha_acc += alpha_sample;

				if(xtoonFlag == 1)
				{
					if(xtoonTypeTone == 1)
					{
						trans_val = lookupTransferTex(alpha_acc, GradientMag * GradientScale, xtoonToneDetail(V * TransScale, texture3D(Volume, pos).x));
						trans_acc += trans_val;
					}

					if(xtoonTypeSpec == 1)
						spec_val = lookupTransferTex(alpha_acc, NdotL, xtoonSpecular(VdotR));

					if(xtoonTypeBack == 1)
						backlight_val = lookupTransferTex(alpha_acc, NdotL, xtoonBacklight(NdotV));

					//trans_sample = mix(mix(mix(backlight_val, trans_val, 0.5), spec_val, 0.5), trans_acc, 0.5);
					trans_sample = mix(mix(backlight_val, trans_val, 0.5), spec_val, 0.5);
				}
				else
				{
					trans_val = lookupTransferTex(alpha_acc, NdotL, V * TransScale);
					trans_sample = trans_val;
				}

				// Color Blending
				vec4 baseColor = vec4(trans_sample.rgb, mix(alpha_acc, trans_acc.a, Alpha));
				color_sample = baseColor;
				color_sample.rgb *= color_sample.a;
				dst = (1.0 - dst.a) * color_sample + dst;

                break;
            }
        }

		if(alpha_acc > 1.0)
			alpha_acc = 0.0;

		if(length(gradient_acc) > Threshold)
		{
			gradient_acc = vec4(0);
		}

		if(length(trans_acc) > Threshold)
		{
			trans_acc = vec4(0);	
		}

        pos += stepVector;		
        rayLength -= StepLength;
    }

    gl_FragColor = dst;
}