#pragma once

#include <GL\glew.h>

#include "Volume.h"
#include "Utilities.h"

class RayCaster
{
public:
	// Constructor
	RayCaster(void);
	RayCaster(int width, int height);

	// Destructor
	~RayCaster(void);

	// Methods
	void initRayCast();
	void renderRayCast();
	void rayCastKeyPress(unsigned char key, int x, int y);
	void drawQuads(float x, float y, float z);
	void drawFullscreenQuad();
	void reshapeOrtho(int w, int h);
	void createRayStartFBO();
	void createRayStopFBO();
	void createRayVertsFBO();
	void createResultFBO();
	void loadTransferFunction();

	inline void setRaycastPgm(bool rp) { raycast = rp; }
	inline void setGradientPgm(bool g) { gradient = g; }
	inline void setDVRPgm(bool d) { dvr = d; }
	inline void setXToonPgm(bool x) { xtoon = x; }

	inline void ShowRayIn(bool ri) { rayIn = ri; }
	inline void ShowRayOut(bool ro) { rayOut = ro; }

	void setRotation(float *m);
	inline void setZoom(float z) { zoom = z; }
	inline void setLightPos(glm::vec3 l) { lightPos = l; }

	inline void setStepLength(float sl) { stepLength = sl; }
	inline void setThreshold(float t) { threshold = t; }
	inline void setGradientScale(float gs) { gradientScale = gs; }
	inline void setGradientDelta(float gd) { gradientDelta = gd; }
	inline void setNSamples(int n) { nSamples = n; }
	inline void setNoiseDelta(float nd) { noiseDelta = nd; }
	inline void setTransferScale(float ts) { transferScale = ts; }

	inline void setFMin(float min) { fMin = min; }
	inline void setFMax(float max) { fMax = max; }
	inline void setAlpha(float a) { alpha = a; }

	inline void setCoarseDetail(float r) { R = r; }
	inline void setBacklightDetail(float bl) { backlight_detail = bl; }
	inline void setShinefactor(float s) { S = s; }
	inline void setXtoon(int xt) { xToonFlag = xt; }
	inline void setXtoonType(int t, int s, int b) { tone = t, spec = s, backLight = b; }

	inline void setThickness(float t) { thickness = t; }

	inline void setJittering(int j) { jitter = j; }

private:
	Volume volume;
	Utilities utilities;

	GLuint fboRayStart, colorBufferRayStart, texRayStart;
	GLuint fboRayStop, colorBufferRayStop, texRayStop;
	GLuint fboRayVerts, colorBufferRayVerts, texRayVertsStart, texRayVertsStop;
	GLuint fboResult, colorBufferResult, texResult;
	GLuint volumeTex;
	GLuint transTex[4];

	GLuint rayCasterProgram;
	GLuint rayVertexProgram;
	GLuint DVRProgram;
	GLuint gradientProgram;
	GLuint xtoonProgram;

	bool raycast, dvr, gradient, xtoon;
	bool rayIn, rayOut;

	float rotMat[16];
	float rotAngleX;
	float rotAngleY;
	float zoom;
	glm::vec3 lightPos;

	float threshold;
	float stepLength;
	float gradientScale;
	float gradientDelta;
	float transferScale;
	int nSamples;
	float noiseDelta;

	float fMin;
	float fMax;
	float alpha;

	float R;
	float backlight_detail;
	float S;
	int xToonFlag;
	int tone, spec, backLight;
	
	float thickness;

	int jitter;

	int width;
	int height;
};

