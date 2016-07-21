#include "Main.h"

// OpenGL Stuff

void controlCB(int control)
{
  if ( control == RAYCAST ) 
  {
	  if(raycast == false)
		  raycast = true;
	  else
		  raycast = false;
  }
  if ( control == GRADIENT ) 
  {
	  if(gradient == false)
	  {
		  gradient = true;
		  GUIXToonRoll->disable();
		  jittering->disable();
		  GUInoise->disable();
		  GUIgscale->disable();
		  GUItscale->disable();
		  GUIfmax->disable();
		  GUIfmin->disable();
		  GUIalpha->disable();
		  GUIthick->disable();
	  }
	  else
	  {
		  gradient = false;
		  GUIXToonRoll->enable();
		  jittering->enable();
		  GUInoise->enable();
		  GUIgscale->enable();
		  GUItscale->enable();
		  GUIfmax->enable();
		  GUIfmin->enable();
		  GUIalpha->enable();
		  GUIthick->enable();
	  }
  }
  if ( control == DVR ) 
  {
	  if(dvr == false)
	  {
		  dvr = true;
		  GUIXToonRoll->disable();
		  GUIRayCastRoll->disable();
	  }
	  else
	  {
		  dvr = false;
		  GUIXToonRoll->enable();
		  GUIRayCastRoll->enable();
	  }
  }
  if ( control == RAYIN ) 
  {
	  if(rayin == false)
	  {
		  rayin = true;
		  GUIXToonRoll->disable();
		  GUIRayCastRoll->disable();
	  }
	  else
	  {
		  rayin = false;
		  GUIXToonRoll->enable();
		  GUIRayCastRoll->enable();
	  }
  }
  if ( control == RAYOUT ) 
  {
	  if(rayout == false)
	  {
		  rayout = true;
		  GUIXToonRoll->disable();
		  GUIRayCastRoll->disable();
	  }
	  else
	  {
		  rayout = false;
		  GUIXToonRoll->enable();
		  GUIRayCastRoll->enable();
	  }
  }
  if ( control == XTOONPGM ) 
  {
	  if(xtoon == false)
	  {
		  xtoon = true;
		  GUIfmax->disable();
		  GUIfmin->disable();
		  GUIthick->disable();
	  }
	  else
	  {
		  xtoon = false;
		  GUIfmax->enable();
		  GUIfmin->enable();
		  GUIthick->enable();
	  }
  }
  if( control == XTOONTYPE)
  {
	  if(toneDetail == 1)
		  tone_detail = 1;
	  else
		  tone_detail = 0;

	  if(backlight == 1)
		  back = 1;
	  else
		  back = 0;

	  if(specHighlight == 1)
		  spec_light = 1;
	  else
		  spec_light = 0;
  }
  if( control == JITTER)
  {
	  if(j == 1)
		  jitter = 1;
	  else
		  jitter = 0;
  }
}

void initVariables()
{
	// Ray Caster Variables
	zoom = 3;

	view_rotate[0] = 1;
	view_rotate[1] = 0;
	view_rotate[2] = 0;
	view_rotate[3] = 0;
	view_rotate[4] = 0;
	view_rotate[5] = 1;
	view_rotate[6] = 0;
	view_rotate[7] = 0;
	view_rotate[8] = 0;
	view_rotate[9] = 0;
	view_rotate[10] = 1;
	view_rotate[11] = 0;
	view_rotate[12] = 0;
	view_rotate[13] = 0;
	view_rotate[14] = 0;
	view_rotate[15] = 1;

	light_pos[0] = 5.0;
	light_pos[1] = 5.0;
	light_pos[2] = 30.0;
	
	nSamples = 64;
	stepLength = 0.01;
	threshold = 1.0;
	gradientScale = 0.5;
	gradientDelta = 1.0;
	transferScale = 1.0;
	noiseDelta = 1.0;

	fMax = 255.0;
	fMin = 0.0;
	alpha = 0.5;

	gradient = false;
	gradientEnable = 0;

	raycast = true;
	raycastEnable = 0;

	dvr = false;
	dvrEnable = 0;

	R = 1.0;
	backlight_detail = 5.0;
	S = 1.5;
	xToonFlag = 0;	
	toneDetail = 0;
	specHighlight = 0;
	backlight = 0;

	rayin = rayout = false;
	rayinToggle = rayoutToggle = 0;

	thickness = 0.1;

	jitter = 1;
	j = 1;
}

void init()
{	
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	rayCaster = RayCaster(width, height);

	initVariables();

	rayCaster.initRayCast();
}

void renderScene()
{
	// Ray Cast Render
	rayCaster.ShowRayIn(rayin);
	rayCaster.ShowRayOut(rayout);
	rayCaster.setRaycastPgm(raycast);
	rayCaster.setDVRPgm(dvr);
	rayCaster.setGradientPgm(gradient);
	rayCaster.setXToonPgm(xtoon);

	rayCaster.setZoom(zoom);
	rayCaster.setRotation(view_rotate);
	rayCaster.setLightPos(glm::vec3(light_pos[0], light_pos[1], light_pos[2]));

	rayCaster.setNSamples(nSamples);
	rayCaster.setStepLength(stepLength);
	rayCaster.setThreshold(threshold);
	rayCaster.setGradientScale(gradientScale);
	rayCaster.setGradientDelta(gradientDelta);
	rayCaster.setTransferScale(transferScale);
	rayCaster.setNoiseDelta(noiseDelta);
	
	rayCaster.setFMax(fMax);
	rayCaster.setFMin(fMin);
	rayCaster.setAlpha(alpha);

	rayCaster.setCoarseDetail(R);
	rayCaster.setBacklightDetail(backlight_detail);
	rayCaster.setShinefactor(S);
	rayCaster.setXtoon(xToonFlag);
	rayCaster.setXtoonType(tone_detail, spec_light, back);

	rayCaster.setThickness(thickness);

	rayCaster.setJittering(jitter);
	
	rayCaster.renderRayCast();

    glutSwapBuffers();    

	// FPS
	frame++;

	time = glutGet(GLUT_ELAPSED_TIME);
	if (time - timebase > 1000) 
	{
		sprintf(s,"Ray Cast Volume Rendering | FPS: %4.2f",frame*1000.0/(time-timebase));
		timebase = time;
		frame = 0;
	}

	glutSetWindowTitle((const char*)&s);
}

void initSub()
{
	texIndex = 0;

	// Load Textures
    GLbyte *pBytes;
	GLint width, height;
	
	char fileName[50];

	glEnable(GL_TEXTURE_2D);
	glGenTextures(4, &tex[0]);

	for(int i=0; i<4; i++)
	{
		glBindTexture(GL_TEXTURE_2D, tex[i]); 
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		sprintf_s(fileName,"transferTex_0%d.tga", i + 1);
		
		std::cout<<"For Texture Window:"<<fileName<<std::endl;

		pBytes = utility.loadTGA(fileName, &width, &height);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, pBytes);
		free(pBytes);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

void renderSub()
{
	float ratio = 1.0;
	
	// Fill screen with viewport
	glViewport(0, 0, 300, 300);

	// Reset projection matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
	// Set a 45 degree perspective
	gluPerspective(45, ratio, 0.0, 25);

    // Reset Modelview matrix      	
    glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	gluLookAt (0.0, 0.0, 5.0, 0.0, 0.0, -1.0, 0.0, 1.0, 0.0);
	
	glClearColor(0.0, 0.0, 0.0, 0.0); 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glEnable(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, tex[texIndex]);
	
	rayCaster.reshapeOrtho(300, 300);
	rayCaster.drawFullscreenQuad();
	
	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	glutSwapBuffers();
}

void keypress(unsigned char key, int x, int y)
{
	// If so, exit the program
    if(key == 27)
	{
		exitScene();
	}

	rayCaster.rayCastKeyPress(key, x, y);

	glutPostRedisplay();
}

void keypressSub(unsigned char key, int x, int y)
{
	// If so, exit the program
    if(key == 27)
	{
		exitScene();
	}

	if(key == '+')
	{
		if(texIndex < 4)
		{
			texIndex++;
			if(texIndex == 4)
				texIndex = 3;
		}
	}

	if(key == '-')
	{
		if(texIndex > -1)
		{
			texIndex--;
			if(texIndex == -1)
				texIndex = 0;
		}
	}

	glutPostRedisplay();
}

void updateScene()
{	
	// Wait until at least 16ms passed since start of last frame
	// Effectively caps framerate at ~60fps
	while(timeGetTime()-lastTickCount<16);
	lastTickCount=timeGetTime();

	// Draw the next frame
    glutPostRedisplay();
}

void exitScene()
{
    std::cout<<"Exiting scene..."<<std::endl;

    // Close window
    glutDestroyWindow(windowId);
	glutDestroyWindow(subWindowId);

    // Free any allocated memory

    // Exit program
    exit(0);
}

void setViewport(int width, int height) 
{
    // Work out window ratio, avoid divide-by-zero
    if(height == 0)
		height = 1;

	float ratio = float(width) / float(height);
	
	// Fill screen with viewport
	glViewport(0, 0, width, height);

	// Reset projection matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
	// Set a 45 degree perspective
	gluPerspective(45, ratio, 0.0, 25);
}

int main(int argc, char *argv[])
{        
    // Initialise OpenGL
    glutInit(&argc, argv); 

    // Set window position, size & create window
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowPosition(50,50);
    glutInitWindowSize(width,height);
	windowId = glutCreateWindow("Ray Cast Volume Rendering");
	
	glewInit();

    // Set GLUT callback functions
	glutDisplayFunc(renderScene);
	//glutIdleFunc(updateScene);
	glutKeyboardFunc(keypress);

    // Setup OpenGL state & scene resources 
    init();

	//GLUI
    glui = GLUI_Master.create_glui( "Controls", 0, 865, 50); 

	movement_panel = new GLUI_Panel( glui, "Movement Parameters" );
    
	view_rot = new GLUI_Rotation( movement_panel, "Rotate", view_rotate );
	view_rot->set_spin( 1.0 );

	trans_z = new GLUI_Translation( movement_panel, "Zoom", GLUI_TRANSLATION_Z, &zoom );
	trans_z->set_speed( .05 );

	raycastPGM = new GLUI_Checkbox(glui, "Raycast /w XToon & Alpha Interpolation", &raycastEnable, RAYCAST, controlCB);
	xtoonPGM = new GLUI_Checkbox(glui, "Raycast XToon Shader", &xtoonEnable, XTOONPGM, controlCB);
	gradientPGM = new GLUI_Checkbox(glui, "Gradient", &gradientEnable, GRADIENT, controlCB);
	dvrPGM = new GLUI_Checkbox(glui, "DVR", &dvrEnable, DVR, controlCB);
	rin = new GLUI_Checkbox(glui, "Show Ray Start", &rayinToggle, RAYIN, controlCB);
	rout = new GLUI_Checkbox(glui, "Show Ray Stop", &rayoutToggle, RAYOUT, controlCB);

	column_01 = new GLUI_Column( glui );

	GUIRayCastRoll = new GLUI_Rollout( glui, "Ray Cast Parameters", false);

	raycaster_panel = new GLUI_Panel( GUIRayCastRoll, "" );
	jittering = new GLUI_Checkbox( raycaster_panel, "Stippling", &j, JITTER, controlCB );
	jittering->set_alignment( GLUI_ALIGN_RIGHT );

    GUIsamples = new GLUI_Spinner( raycaster_panel, "N Samples", &nSamples );
	GUIsamples->set_int_limits( 0, 255 );
    GUIsamples->set_alignment( GLUI_ALIGN_RIGHT );

    GUInoise = new GLUI_Spinner( raycaster_panel, "Noise Delta", &noiseDelta );
	GUInoise->set_float_limits( 1.0, 10.0 );
    GUInoise->set_alignment( GLUI_ALIGN_RIGHT );

    GUIstep = new GLUI_Spinner( raycaster_panel, "Step Length", &stepLength );
	GUIstep->set_float_limits( 0.01f, 1.0f );
    GUIstep->set_alignment( GLUI_ALIGN_RIGHT );

    GUIt = new GLUI_Spinner( raycaster_panel, "Threshold", &threshold );
	GUIt->set_float_limits( 0.01f, 1.0f );
    GUIt->set_alignment( GLUI_ALIGN_RIGHT );

    GUIgscale = new GLUI_Spinner( raycaster_panel, "Gradient Scale", &gradientScale );
	GUIgscale->set_float_limits( 0.01f, 1.0f );
    GUIgscale->set_alignment( GLUI_ALIGN_RIGHT );

    GUIgdelta = new GLUI_Spinner( raycaster_panel, "Gradient Delta", &gradientDelta );
	GUIgdelta->set_float_limits( 0.01f, 1.0f );
    GUIgdelta->set_alignment( GLUI_ALIGN_RIGHT );

    GUIfmax = new GLUI_Spinner( raycaster_panel, "Histogram (Alpha) Max", &fMax );
	GUIfmax->set_float_limits( 0.01f, 255.0f );
    GUIfmax->set_alignment( GLUI_ALIGN_RIGHT );

    GUIfmin = new GLUI_Spinner( raycaster_panel, "Histogram (Alpha) Min", &fMin );
	GUIfmin->set_float_limits( 0.0f, 255.0f );
    GUIfmin->set_alignment( GLUI_ALIGN_RIGHT );

    GUIalpha = new GLUI_Spinner( raycaster_panel, "Alpha Value", &alpha );
	GUIalpha->set_float_limits( 0.01f, 1.0f );
    GUIalpha->set_alignment( GLUI_ALIGN_RIGHT );

    GUItscale = new GLUI_Spinner( raycaster_panel, "Transfer Scale", &transferScale );
	GUItscale->set_float_limits( 0.01f, 1.0f );
    GUItscale->set_alignment( GLUI_ALIGN_RIGHT );

    GUIthick = new GLUI_Spinner( raycaster_panel, "Thickness", &thickness );
	GUIthick->set_float_limits( 0.01f, 1.0f );
    GUIthick->set_alignment( GLUI_ALIGN_RIGHT );

	GUIXToonRoll = new GLUI_Rollout( glui, "XTOON Parameters", false );
	
	xtoon_panel = new GLUI_Panel( GUIXToonRoll, "" );

	toneDetailBox = new GLUI_Checkbox(xtoon_panel, "Tone Detail", &toneDetail, XTOONTYPE, controlCB );

    GUIr = new GLUI_Spinner( xtoon_panel, "Coarse Detail", &R );
	GUIr->set_float_limits( 1.0f, 25.0f );
    GUIr->set_alignment( GLUI_ALIGN_RIGHT );

	backlightBOX = new GLUI_Checkbox(xtoon_panel, "Backlighting", &backlight, XTOONTYPE, controlCB );

    GUIR = new GLUI_Spinner( xtoon_panel, "Backlight Detail", &backlight_detail );
	GUIR->set_float_limits( 0.0f, 25.0f );
    GUIR->set_alignment( GLUI_ALIGN_RIGHT );

	specularBOX = new GLUI_Checkbox(xtoon_panel, "Specular Highlight", &specHighlight, XTOONTYPE, controlCB );

    GUIs = new GLUI_Spinner( xtoon_panel, "Shine Factor", &S );
	GUIs->set_float_limits( 1.0f, 25.0f );
    GUIs->set_alignment( GLUI_ALIGN_RIGHT );

	xtoonBOX = new GLUI_Checkbox(xtoon_panel, "Enable / Disable", &xToonFlag, XTOON, controlCB );

	glui->set_main_gfx_window(windowId);
    
	glutInitWindowSize(300, 300);
	subWindowId = glutCreateWindow("Transfer Function 2D Texture");
	glutPositionWindow(865, 350);

	// Set GLUT callback functions
	glutReshapeFunc(setViewport);
	glutDisplayFunc(renderSub);
	glutKeyboardFunc(keypressSub);
	
	// Setup OpenGL state & scene resources 
	initSub();

	GLUI_Master.set_glutIdleFunc(updateScene); 

    // Show window & start update loop
    glutMainLoop();    

	return 0;
}
