varying vec4 vPosition;

varying vec4 gl_FrontColor; 

void main()
{
	gl_FrontColor = gl_Vertex;
	gl_TexCoord[0]  = gl_Vertex;

	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex ;
	vPosition = gl_Position;
}
