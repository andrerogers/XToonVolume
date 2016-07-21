#include "Volume.h"

#include <iostream>

Volume::Volume(void)
{
	initExtensions();
}

Volume::~Volume(void)
{
}

void Volume::initExtensions()
{
	glTexImage3D = (PFNGLTEXIMAGE3DPROC) wglGetProcAddress("glTexImage3D");
}

bool Volume::loadVolume(char *fileName)
{
	printf("Loading Volume Data Set... \n");
	FILE *vfile = fopen(fileName, "rb");

	if(NULL == vfile)
	{
		printf("Error Loading!! File empty... \n");
		printf("\n");
		return false;
	}

	int size = WIDTH * HEIGHT * DEPTH;

	unsigned char *volume = new unsigned char[size];
	memset(volume, 0, size * sizeof(unsigned char));

	fread(volume, sizeof(unsigned char), size, vfile);
	fclose(vfile);

	glGenTextures(1, &volumeTexture);
	glBindTexture(GL_TEXTURE_3D,volumeTexture);

	// set the texture parameters
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	
	glTexImage3D(GL_TEXTURE_3D, 0, GL_LUMINANCE, WIDTH, HEIGHT, DEPTH, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, volume);

	delete [] volume;

	printf("Volume Loaded!! \n");
	printf("\n");

	return true;
}

bool Volume::loadGradientVolume(char *fileName)
{
	printf("Loading Gradient Data Set... \n");
	FILE *vfile = fopen(fileName, "rb");

	if(NULL == vfile)
	{
		printf("Error Loading!! File empty... \n");
		printf("\n");
		return false;
	}

	int size = WIDTH * HEIGHT * DEPTH;

	unsigned char *volume = new unsigned char[size * 3];
	memset(volume, 0, size * 3 * sizeof(unsigned char));

	fread(volume, sizeof(unsigned char), size * 3, vfile);
	fclose(vfile);

	glGenTextures(1, &gradientTexture);
	glBindTexture(GL_TEXTURE_3D,gradientTexture);

	// set the texture parameters
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB8, WIDTH, HEIGHT, DEPTH, 0, GL_RGB, GL_UNSIGNED_BYTE, volume);

	delete [] volume;

	printf("Gradient Loaded!! \n");
	printf("\n");

	return true;
}
