varying vec4 vPosition;

void main()
{
    vec4 pos = vPosition;

    if (gl_FrontFacing)
	{
        gl_FragData[0] = 0.5 * (pos + 1.0);
        gl_FragData[1] = vec4(0);
    } 
	else 
	{
        gl_FragData[0] = vec4(0);
        gl_FragData[1] = 0.5 * (pos + 1.0);
    }
}