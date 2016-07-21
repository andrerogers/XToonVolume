#pragma once

#include <windows.h>	
#include <mmsystem.h>	
#include <iostream>		
#include "Utilities.h"
#include <gl\GLU.h>	
#include <GL\glut.h>
#include <GL\glui.h>

#include "Volume.h"
#include "RayCaster.h"

#define RAYCAST 200
#define GRADIENT 201
#define DVR 202
#define XTOON 203
#define XTOONTYPE 204
#define XTOONPGM 205
#define RAYIN 206
#define RAYOUT 207
#define JITTER 208

// OpenGL Functions
void init();
void initSub();
void updateScene();
void renderScene();
void renderSub();
void exitScene();
void keypress(unsigned char key, int x, int y);
void setViewport(int width, int height);

void controlCB(int control);
void initVariables();

// Variables

const int width = 800;
const int height = 600;
int windowId, subWindowId;
DWORD lastTickCount;

// Compute FPS
int frame = 0;
long time = 0, timebase = 0;
char s[50];

// Lighting
GLfloat ambientLight[4];
GLfloat diffuseLight[4];
GLfloat specularLight[4];

// Ray Caster Instance
RayCaster rayCaster;

// GLUI
GLUI *glui;

// Ray Caster Variables
bool raycast;
int raycastEnable;
bool gradient;
int gradientEnable;
bool dvr;
int dvrEnable;
bool xtoon;
int xtoonEnable;

bool rayin, rayout;
int rayinToggle, rayoutToggle; 

float zoom;
float view_rotate[16];
float light_pos[3];

float gradientDelta;
float stepLength;
float threshold;
float gradientScale;
float transferScale;
float noiseDelta;
int nSamples;

float fMin;
float fMax;
float alpha;

float R;
float backlight_detail;
float S;
int xToonFlag;
int tone_detail, spec_light, back;
int toneDetail, specHighlight, backlight;

float thickness;

int j, jitter;

// Texture Window
Utilities utility;
GLuint tex[4];
int texIndex;

// GLUI
GLUI_Panel *movement_panel, *raycaster_panel, *xtoon_panel;
GLUI_Rotation *view_rot;
GLUI_Translation *trans_z;
GLUI_Checkbox *raycastPGM, *xtoonPGM, *gradientPGM, *dvrPGM, *rin, *rout;
GLUI_Checkbox *xtoonBOX, *specularBOX, *backlightBOX, *toneDetailBox;
GLUI_Checkbox *jittering;
GLUI_Column *column_01;
GLUI_Rollout *GUIRayCastRoll, *GUIXToonRoll;
GLUI_Spinner *GUIsamples, *GUInoise, *GUIt, *GUIgscale, *GUIgdelta, *GUItscale, *GUIstep;
GLUI_Spinner *GUIfmax, *GUIfmin, *GUIalpha;
GLUI_Spinner *GUIr, *GUIR, *GUIs;
GLUI_Spinner *GUIthick;
