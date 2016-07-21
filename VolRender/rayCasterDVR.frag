varying vec4 vPosition;

uniform sampler2D RayStart;
uniform sampler2D RayStop;
uniform sampler3D Volume;

const float StepLength = 0.01;

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
        return;
    }
    
	vec3 ray = rayStop - rayStart;
    float rayLength = length(ray);
    vec3 stepVector = StepLength * ray/rayLength;
    vec3 pos = rayStart;
    vec4 dst = vec4(0);
    
	while (dst.a < 1.0 && rayLength > 0.0)
	{
        vec3 V = texture3D(Volume, pos).xyz;
        vec4 src = vec4(0.25 * (V + 1.0), dot(V, V));

        src.rgb *= src.a;
        dst = (1.0 - dst.a) * src + dst;
        pos += stepVector;
        rayLength -= StepLength;
    }

    gl_FragColor = dst;
}
