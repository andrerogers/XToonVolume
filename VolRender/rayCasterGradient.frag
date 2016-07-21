varying vec4 vPosition;
varying vec3 eyePosition;

// For samplers
uniform sampler2D RayStart;
uniform sampler2D RayStop;
uniform sampler3D Volume;

// For ray caster
uniform int NSamples;
uniform float StepLength;
uniform float Threshold;
uniform float GradientDelta;

// Calculate difference to sharpen the image
const float sizes = 256.0;
const vec4 mask = vec4(1.0, 0.0, 0.0, 0.0);
vec4 e = vec4(vec3(-1.0, 1.0, 0.0)/sizes, 0.0);

// For Sobel operator
vec4 g_x = vec4(0);
vec4 g_y = vec4(0);
vec4 g_z = vec4(0);
vec4 sobelResult = vec4(0);
vec4 gradient_acc = vec4(0);

// Result
vec4 dst = vec4(0);
vec4 src = vec4(0);

float lookup(vec3 coord)
{
    vec3 V = texture3D(Volume, coord).xyz;
    return dot(V, V);
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

    vec3 pos = rayStart;

    while (dst.a < 1.0 && rayLength > 0.0) 
	{   
        float V = lookup(pos);

        if (V > Threshold) 
		{	
			// For Texture Gradient
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

            // Refine the hitpoint to reduce slicing artifacts:
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
				vec3 vGradient = gradient_acc.rgb * GradientDelta;

				src = vec4(vGradient.r, vGradient.g, vGradient.b, gradient_acc.a);
				src.rgb *= src.a;
				dst = (1.0 - dst.a) * src + dst;
                
				break;
            }
        }

		gradient_acc = vec4(0);				

        pos += stepVector;
        rayLength -= StepLength;
    }

    gl_FragColor = dst;
}