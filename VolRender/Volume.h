#pragma once

#include <GL\glew.h>
#include <GL\wglew.h>

#define WIDTH 256
#define HEIGHT 256
#define DEPTH 256

class Volume
{
public:
	// Constructor
	Volume(void);

	// Destructor
	~Volume(void);

	// Methods
	inline GLuint getVolumeTexture() { return volumeTexture; }
	inline GLuint getGradientTexture() { return gradientTexture; }
	
	bool loadVolume(char *fileName);
	bool loadGradientVolume(char *fileName);
	void initExtensions();

private:
	GLuint volumeTexture;
	GLuint gradientTexture;

	PFNGLTEXIMAGE3DPROC glTexImage3D;
};

