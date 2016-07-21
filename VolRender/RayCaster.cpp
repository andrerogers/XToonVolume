#include "RayCaster.h"

#include <iostream>

RayCaster::RayCaster(void)
{
}

RayCaster::RayCaster(int width, int height)
	: width(width), height(height)
{
}

RayCaster::~RayCaster(void)
{
}

void RayCaster::createRayStartFBO()
{
	glGenFramebuffers(1, &fboRayStart);
	glBindFramebuffer(GL_FRAMEBUFFER, fboRayStart);

	glGenRenderbuffers(1, &colorBufferRayStart);
	glBindRenderbuffer(GL_RENDERBUFFER, colorBufferRayStart);

	glGenTextures(1, &texRayStart);
	glBindTexture(GL_TEXTURE_2D, texRayStart);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glGenerateMipmap(GL_TEXTURE_2D);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_HALF_FLOAT, NULL);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texRayStart, 0);

	if(GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER))
	{
		std::cout<<"Unable to create frame buffer..."<<std::endl;
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	std::cout<<"Ray Start Frame Buffer created..."<<std::endl;
	std::cout<<" "<<std::endl;
}

void RayCaster::createRayStopFBO()
{
	glGenFramebuffers(1, &fboRayStop);
	glBindFramebuffer(GL_FRAMEBUFFER, fboRayStop);

	glGenRenderbuffers(1, &colorBufferRayStop);
	glBindRenderbuffer(GL_RENDERBUFFER, colorBufferRayStop);

	glGenTextures(1, &texRayStop);
	glBindTexture(GL_TEXTURE_2D, texRayStop);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glGenerateMipmap(GL_TEXTURE_2D);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_HALF_FLOAT, NULL);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texRayStop, 0);

	if(GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER))
	{
		std::cout<<"Unable to create frame buffer..."<<std::endl;
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	std::cout<<"Ray Start Frame Buffer created..."<<std::endl;
	std::cout<<" "<<std::endl;
}

void RayCaster::createRayVertsFBO()
{
	glGenFramebuffers(1, &fboRayVerts);
	glBindFramebuffer(GL_FRAMEBUFFER, fboRayVerts);

	glGenRenderbuffers(1, &colorBufferRayVerts);
	glBindRenderbuffer(GL_RENDERBUFFER, colorBufferRayVerts);

	glGenTextures(1, &texRayVertsStart);
	glBindTexture(GL_TEXTURE_2D, texRayVertsStart);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glGenerateMipmap(GL_TEXTURE_2D);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_HALF_FLOAT, NULL);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texRayVertsStart, 0);

	glBindTexture(GL_TEXTURE_2D, 0); // check

	if(GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER))
	{
		std::cout<<"Unable to create frame buffer..."<<std::endl;
	}

	glGenTextures(1, &texRayVertsStop);
	glBindTexture(GL_TEXTURE_2D, texRayVertsStop);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glGenerateMipmap(GL_TEXTURE_2D);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_HALF_FLOAT, NULL);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, texRayVertsStop, 0);

	if(GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER))
	{
		std::cout<<"Unable to create frame buffer..."<<std::endl;
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	std::cout<<"Ray Verts Buffer created..."<<std::endl;
	std::cout<<" "<<std::endl;
}

void RayCaster::createResultFBO()
{
	glGenFramebuffers(1, &fboResult);
	glBindFramebuffer(GL_FRAMEBUFFER, fboResult);

	glGenRenderbuffers(1, &colorBufferResult);
	glBindRenderbuffer(GL_RENDERBUFFER, colorBufferResult);

	glGenTextures(1, &texResult);
	glBindTexture(GL_TEXTURE_2D, texResult);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_HALF_FLOAT, NULL);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texResult, 0);

	if(GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER))
	{
		std::cout<<"Unable to create frame buffer..."<<std::endl;
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	std::cout<<"Result Frame Buffer created..."<<std::endl;
	std::cout<<" "<<std::endl;
}

void RayCaster::loadTransferFunction()
{
    GLbyte *pBytes;
	GLint width, height;
	
	char fileName[50];

	glEnable(GL_TEXTURE_2D);
	glGenTextures(4, &transTex[0]);

	for(int i=0; i<4; i++)
	{
		glBindTexture(GL_TEXTURE_2D, transTex[i]); 
		
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		sprintf_s(fileName,"transferTex_0%d.tga", i + 1);

		pBytes = utilities.loadTGA(fileName, &width, &height);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, pBytes);
		free(pBytes);

		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

void RayCaster::drawQuads(float x, float y, float z)
{
	glTranslatef(-x/2, -y/2, -z/2);

	glBegin(GL_QUADS);
	/* Back side */
	glNormal3f(0.0, 0.0, -1.0);
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(0.0, y, 0.0);
	glVertex3f(x, y, 0.0);
	glVertex3f(x, 0.0, 0.0);

	/* Front side */
	glNormal3f(0.0, 0.0, 1.0);
	glVertex3f(0.0, 0.0, z);
	glVertex3f(x, 0.0, z);
	glVertex3f(x, y, z);
	glVertex3f(0.0, y, z);

	/* Top side */
	glNormal3f(0.0, 1.0, 0.0);
	glVertex3f(0.0, y, 0.0);
	glVertex3f(0.0, y, z);
    glVertex3f(x, y, z);
	glVertex3f(x, y, 0.0);

	/* Bottom side */
	glNormal3f(0.0, -1.0, 0.0);
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(x, 0.0, 0.0);
	glVertex3f(x, 0.0, z);
	glVertex3f(0.0, 0.0, z);

	/* Left side */
	glNormal3f(-1.0, 0.0, 0.0);
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(0.0, 0.0, z);
	glVertex3f(0.0, y, z);
	glVertex3f(0.0, y, 0.0);

	/* Right side */
	glNormal3f(1.0, 0.0, 0.0);
	glVertex3f(x, 0.0, 0.0);
	glVertex3f(x, y, 0.0);
	glVertex3f(x, y, z);
	glVertex3f(x, 0.0, z);
	glEnd();

	glTranslatef(x/2, y/2, z/2);
}

void RayCaster::drawFullscreenQuad()
{ 
	glDisable(GL_DEPTH_TEST);
	glBegin(GL_QUADS);
   
	glTexCoord2f(0,0); 
	glVertex2f(0,0);

	glTexCoord2f(1,0); 
	glVertex2f(1,0);

	glTexCoord2f(1, 1); 
	glVertex2f(1, 1);

	glTexCoord2f(0, 1); 
	glVertex2f(0, 1);

	glEnd();
	glEnable(GL_DEPTH_TEST);
}

void RayCaster::reshapeOrtho(int w, int h)
{
	if (h == 0) h = 1;

	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, 1, 0, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void RayCaster::initRayCast()
{

	volume = Volume();
	utilities = Utilities();

	if(volume.loadVolume("head.raw"))
		volumeTex = volume.getVolumeTexture();
	
	createRayStartFBO();
	createRayStopFBO();
	createResultFBO();

	loadTransferFunction();

	std::cout<<"Loading Ray Verts Program"<<std::endl;
	rayVertexProgram = utilities.loadProgram("rayVertex.vert", NULL, NULL);
	std::cout<<"Loading DVR Program"<<std::endl;
	DVRProgram = utilities.loadProgram("rayCaster.vert", NULL, "rayCasterDVR.frag");
	std::cout<<"Loading Raycast Gradient Program"<<std::endl;
	gradientProgram = utilities.loadProgram("rayCaster.vert", NULL, "rayCasterGradient.frag");
	std::cout<<"Loading Raycast XToon Program"<<std::endl;
	xtoonProgram = utilities.loadProgram("rayCaster.vert", NULL, "rayCastXToon.frag");
	std::cout<<"Loading Raycast Program"<<std::endl;
	rayCasterProgram = utilities.loadProgram("rayCaster.vert", NULL, "rayCaster.frag");

	rotAngleX = 0.0;
	rotAngleY = 0.0;
}

void RayCaster::renderRayCast()
{
	float ratio = float(width)/float(height);
	
	// Fill screen with viewport
	glViewport(0, 0, width, height);

	// Reset projection matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
	// Set a 45 degree perspective
	gluPerspective(45, ratio, 0.0, 25);

    // Reset Modelview matrix      	
    glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	gluLookAt (0.0, 0.0, 5.0, 0.0, 0.0, -1.0, 0.0, 1.0, 0.0);
    
	glTranslatef(0.0, 0.0, 0.0);
	glTranslatef(0.0, 0.0, zoom);	
	glRotatef(rotAngleY, 0.0, 1.0, 0.0);
	glRotatef(rotAngleX, 1.0, 0.0, 0.0);
	glMultMatrixf(rotMat);
	
	glClearColor(0.0, 0.0, 0.0, 0.0); 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Ray Start (Render to FBO)
	glBindFramebuffer(GL_FRAMEBUFFER, fboRayStart);
	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glUseProgram(rayVertexProgram);	
	drawQuads(1.0, 1.0, 1.0);
	glUseProgram(0);
	glDisable(GL_CULL_FACE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glBindTexture(GL_TEXTURE_2D, texRayStart);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Ray Stop (Render to FBO)
	glBindFramebuffer(GL_FRAMEBUFFER, fboRayStop);
	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glUseProgram(rayVertexProgram);	
	drawQuads(1.0, 1.0, 1.0);
	glUseProgram(0);
	glDisable(GL_CULL_FACE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, texRayStop);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Ray Cast (Render to FBO)
	glBindFramebuffer(GL_FRAMEBUFFER, fboResult);
	glViewport(0, 0, width, height);
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	if(raycast)
	{
		glUseProgram(rayCasterProgram);	
	}
	if(dvr)
	{
		glUseProgram(DVRProgram);	
	}
	if(gradient)
	{
		glUseProgram(gradientProgram);
	}
	if(xtoon)
	{
		glUseProgram(xtoonProgram);
	}

	utilities.setUniform("LightPosition", lightPos);

	utilities.setUniform("StepLength", stepLength);
	utilities.setUniform("Threshold", threshold);
	utilities.setUniform("GradientScale", gradientScale);
	utilities.setUniform("GradientDelta", gradientDelta);
	utilities.setUniform("NoiseDelta", noiseDelta);
	utilities.setUniform("NSamples", nSamples);
	utilities.setUniform("TransScale", transferScale);

	utilities.setUniform("FMin", fMin);
	utilities.setUniform("FMax", fMax);
	utilities.setUniform("Alpha", alpha);

	utilities.setUniform("r", R);
	utilities.setUniform("BacklightDetail", backlight_detail);
	utilities.setUniform("s", S);
	utilities.setUniform("xtoonFlag", xToonFlag);
	utilities.setUniform("xtoonTypeTone", tone);
	utilities.setUniform("xtoonTypeSpec", spec);
	utilities.setUniform("xtoonTypeBack", backLight);

	utilities.setUniform("Thickness", thickness);

	utilities.setUniform("Jitter", jitter);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_3D, volumeTex);
	utilities.setUniform("Volume", 2);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, transTex[0]);
	utilities.setUniform("TransferFunc_01", 3);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, transTex[1]);
	utilities.setUniform("TransferFunc_02", 4);
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, transTex[2]);
	utilities.setUniform("TransferFunc_03", 5);
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, transTex[3]);
	utilities.setUniform("TransferFunc_04", 6);
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, texRayStop);
	utilities.setUniform("RayStop", 1);
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, texRayStart);
	utilities.setUniform("RayStart", 0);
	drawQuads(1.0, 1.0, 1.0);
	glUseProgram(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, texResult);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Render to Quad
	
	glEnable(GL_TEXTURE_2D);
	if(rayIn)
	{
		glBindTexture(GL_TEXTURE_2D, texRayStart);
	}
	else if(rayOut)
	{
		glBindTexture(GL_TEXTURE_2D, texRayStop);
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, texResult);
	}

	reshapeOrtho(width, height);
	//glColor4f(1.0, 1.0, 1.0, 1.0);
	drawFullscreenQuad();

	glDisable(GL_TEXTURE_2D);
}

void RayCaster::rayCastKeyPress(unsigned char key, int x, int y)
{
	if(key == 'a' || key == 'A')
		rotAngleY += 5.0;
	
	if(key == 'd' || key == 'D')
		rotAngleY -= 5.0;

	if(key == 'w' || key == 'W')
		rotAngleX += 5.0;
	
	if(key == 's' || key == 'S')
		rotAngleX -= 5.0;
}

void RayCaster::setRotation(float *m)
{
	rotMat[0] = m[0];
	rotMat[1] = m[1];
	rotMat[2] = m[2];
	rotMat[3] = m[3];
	rotMat[4] = m[4];
	rotMat[5] = m[5];
	rotMat[6] = m[6];
	rotMat[7] = m[7];
	rotMat[8] = m[8];
	rotMat[9] = m[9];
	rotMat[10] = m[10];
	rotMat[11] = m[11];
	rotMat[12] = m[12];
	rotMat[13] = m[13];
	rotMat[14] = m[14];
	rotMat[15] = m[15];
}
