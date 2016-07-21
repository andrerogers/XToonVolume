varying vec4 vPosition;
varying vec3 eyePosition;

varying vec4 gl_FrontColor; 

void main()
{
	gl_FrontColor = gl_Vertex;
	gl_TexCoord[0]  = gl_Vertex;

	vec4 eyeTemp = gl_ModelViewMatrix * gl_Vertex;
	eyePosition = eyeTemp.xyz / eyeTemp.w;

	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex ;
	vPosition = gl_Position;
}
