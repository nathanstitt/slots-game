/*
 * This code was created by Jeff Molofee '99 
 * (ported to Linux/SDL by Ti Leggett '01)
 *
 * If you've found this code useful, please let me know.
 *
 * Visit Jeff at http://nehe.gamedev.net/
 * 
 * or for port-specific comments, questions, bugreports etc. 
 * email to leggett@eecs.tulane.edu
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "SDL/SDL.h"
#include "time.h"
#include "SDL/SDL_mixer.h"
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <vector>
#include <iostream>
#include <math.h>
#include <fstream>
#include <map>

using namespace std;

/* Set up some booleans */
#define TRUE  1
#define FALSE 0

#define RECORDS_LOG_FILE "/home/nas/slots-records.csv"
/* screen width, height, and bit depth */
#define SCREEN_WIDTH  1024
#define SCREEN_HEIGHT  768
#define SCREEN_BPP      16

/* Max number of particles */
#define MAX_PARTICLES 1000



// flag logo
GLfloat flag_x_axis=180;            /* X Rotation */
bool flag_up=false;
float points[45][45][3]; /* The Points On The Grid Of Our "Wave" */
int wiggle_count = 0;    /* Counter Used To Control How Fast Flag Waves */
GLfloat hold;            /* Temporarily Holds A Floating ioint Value */

static std::ofstream records;
typedef std::map<int,time_t> patrons_t;
static patrons_t patrons;

/* Create our particle structure */
struct Particle
{
    int   active; /* Active (Yes/No) */
    float life;   /* Particle Life   */
    float fade;   /* Fade Speed      */

    float r;      /* Red Value       */
    float g;      /* Green Value     */
    float b;      /* Blue Value      */

    float x;      /* X Position      */
    float y;      /* Y Position      */
    float z;      /* Z Position      */

    float xi;     /* X Direction     */
    float yi;     /* Y Direction     */
    float zi;     /* Z Direction     */

    float xg;     /* X Gravity       */
    float yg;     /* Y Gravity       */
    float zg;     /* Z Gravity       */
} ;

/* Rainbow of colors */
static GLfloat colors[12][3] =
{
        { 1.0f,  0.5f,  0.5f},
	{ 1.0f,  0.75f, 0.5f},
	{ 1.0f,  1.0f,  0.5f},
	{ 0.75f, 1.0f,  0.5f},
        { 0.5f,  1.0f,  0.5f},
	{ 0.5f,  1.0f,  0.75f},
	{ 0.5f,  1.0f,  1.0f},
	{ 0.5f,  0.75f, 1.0f},
        { 0.5f,  0.5f,  1.0f},
	{ 0.75f, 0.5f,  1.0f},
	{ 1.0f,  0.5f,  1.0f},
	{ 1.0f,  0.5f,  0.75f}
};

/* This is our SDL surface */
SDL_Surface *surface;

GLfloat z = -5.0f; /* Depth Into The Screen */

/* Ambient Light Values */
GLfloat LightAmbient[]  = { 0.5f, 0.5f, 0.5f, 1.0f };
/* Diffuse Light Values */
GLfloat LightDiffuse[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
/* Light Position */
GLfloat LightPosition[] = { 0.0f, 0.0f, 2.0f, 1.0f };

GLUquadricObj *quadratic;     /* Storage For Our Quadratic Objects */

GLuint cylinder_side_tex;
GLuint cylinder_spinner_tex;
GLuint reels_tex;
GLuint fireworks_tex;
GLuint logo_tex;

int
play_wav(const char* name)
{
	
	Mix_Chunk *sound = Mix_LoadWAV(name);
	if(sound == NULL) {
		fprintf(stderr, "Unable to load WAV file: %s - %s\n", name, Mix_GetError() );
		return -1;
	} else {
		int channel=Mix_PlayChannel(-1, sound, 0);
		if(channel == -1) {
			fprintf(stderr, "Unable to play WAV file: %s - %s\n", name, Mix_GetError() );
			return -1;
		}
		return channel;
		
	}
}


struct Firework {
	
	/* Our beloved array of particles */
	Particle particles[MAX_PARTICLES];
	int rainbow;
	float slowdown; 
	float xspeed;          /* Base X Speed (To Allow Keyboard Direction Of Tail) */
	float yspeed;          /* Base Y Speed (To Allow Keyboard Direction Of Tail) */
	float zoom;   /* Used To Zoom Out                                   */

	GLuint loop;           /* Misc Loop Variable                                 */
	GLuint col;        /* Current Color Selection                            */
	GLuint delay;          /* Rainbow Effect Delay                               */

	Firework( GLfloat angle ) :
		rainbow( true ),
		slowdown( 3.0f ), /* Slow Down Particles                                */
		yspeed( 200.0f ),
		xspeed( angle ),
		zoom( -40.0f ),
		loop( 0 ),
		col( 0 ),
		delay( 0 )
		
		{ 
			this->reset();
		}


	void cycle_colors(){
		col = ( ++col ) % 12;
	}

	void reset(){
		/* Reset all the particles */
		for ( loop = 0; loop < MAX_PARTICLES; loop++ ) {
			int color = ( loop + 1 ) / ( MAX_PARTICLES / 12 );
			float xi, yi, zi;
			xi =  ( float )( ( rand( ) % 50 ) - 26.0f ) * 10.0f;
			yi = zi = ( float )( ( rand( ) % 50 ) - 25.0f ) * 10.0f;
			
			this->reset_particle( loop, color, xi, yi, zi );
		}
	}
/* function to reset one particle to initial state */
/* NOTE: I added this function to replace doing the same thing in several
 * places and to also make it easy to move the pressing of numpad keys
 * 2, 4, 6, and 8 into handleKeyPress function.
 */
	void
	reset_particle( int num, int color, float xDir, float yDir, float zDir )
		{
			/* Make the particels active */
			particles[num].active = TRUE;
			/* Give the particles life */
			particles[num].life = 7.0f;
			/* Random Fade Speed */
			particles[num].fade = ( float )( rand( ) %100 ) / 1000.0f + 0.003f;
			/* Select Red Rainbow Color */
			particles[num].r = colors[color][0];
			/* Select Green Rainbow Color */
			particles[num].g = colors[color][1];
			/* Select Blue Rainbow Color */
			particles[num].b = colors[color][2];
			/* Set the position on the X axis */
			particles[num].x = 0.0f;
			/* Set the position on the Y axis */
			particles[num].y = 0.0f;
			/* Set the position on the Z axis */
			particles[num].z = 0.0f;
			/* Random Speed On X Axis */
			particles[num].xi = xDir;
			/* Random Speed On Y Axi */
			particles[num].yi = yDir;
			/* Random Speed On Z Axis */
			particles[num].zi = zDir;
			/* Set Horizontal Pull To Zero */
			particles[num].xg = 0.0f;
			/* Set Vertical Pull Downward */
			particles[num].yg = -0.4f;
			/* Set Pull On Z Axis To Zero */
			particles[num].zg = 0.0f;

			return;
		}

	void tick(){
		cycle_colors();

	/* Modify each of the particles */
	for ( loop = 0; loop < MAX_PARTICLES; loop++ ) {
		if ( particles[loop].active ) {
			/* Grab Our Particle X Position */
			float x = particles[loop].x;
			/* Grab Our Particle Y Position */
			float y = particles[loop].y;
			/* Particle Z Position + Zoom */
			float z = particles[loop].z + zoom;

			glBindTexture( GL_TEXTURE_2D, fireworks_tex );

			/* Draw The Particle Using Our RGB Values,
			 * Fade The Particle Based On It's Life
			 */
			glColor4f( particles[loop].r,
				   particles[loop].g,
				   particles[loop].b,
				   particles[loop].life );
			
			/* Build Quad From A Triangle Strip */
			glBegin( GL_TRIANGLE_STRIP );

			/* Top Right */
			glTexCoord2d( 1, 1 );
			glVertex3f( x + 0.5f, y + 0.5f, z );
			/* Top Left */
			glTexCoord2d( 0, 1 );
			glVertex3f( x - 0.5f, y + 0.5f, z );
			/* Bottom Right */
			glTexCoord2d( 1, 0 );
			glVertex3f( x + 0.5f, y - 0.5f, z );
			/* Bottom Left */
			glTexCoord2d( 0, 0 );
			glVertex3f( x - 0.5f, y - 0.5f, z );
			glEnd( );

			/* Move On The X Axis By X Speed */
			particles[loop].x += particles[loop].xi /
				( slowdown * 1000 );
			/* Move On The Y Axis By Y Speed */
			particles[loop].y += particles[loop].yi /
				( slowdown * 1000 );
			/* Move On The Z Axis By Z Speed */
			particles[loop].z += particles[loop].zi /
				( slowdown * 1000 );

			/* Take Pull On X Axis Into Account */
			particles[loop].xi += particles[loop].xg;
			/* Take Pull On Y Axis Into Account */
			particles[loop].yi += particles[loop].yg;
			/* Take Pull On Z Axis Into Account */
			particles[loop].zi += particles[loop].zg;

			/* Reduce Particles Life By 'Fade' */
			particles[loop].life -= particles[loop].fade;

			/* If the particle dies, revive it */
			if ( particles[loop].life < 0.0f )
			{
				float xi, yi, zi;
				xi = xspeed +
					( float )( ( rand( ) % 60 ) - 32.0f );
				yi = yspeed +
					( float)( ( rand( ) % 60 ) - 30.0f );
				zi = ( float )( ( rand( ) % 60 ) - 30.0f );
				reset_particle( loop, col, xi, yi, zi );
                        }
		}
	}
	}

};


Firework left_firework(   180.0f );
Firework right_firework( -180.0f );

struct Reel;

static Reel* reels[3];


GLfloat
get_rand( GLfloat max ){
	return ( 1 + (float) ( max * (rand() / (RAND_MAX + 1.0))) );
}

static int AnointedStopPosition=-1;

struct Reel {

	enum {
		BIG_WINNER=2
	};
	
	GLfloat speed;
	GLfloat degree;
	bool stopped;
	GLfloat begin;

	static void stop(){
		int r=0;
		for ( int x=0; x<3; ++x ){
			if ( reels[x]->speed < reels[r]->speed ){
				r=x;
			}
		}
		reels[r]->stopped = true;
	}

	static int winner(){
		int win = reels[0]->position();
		for ( int x=1; x<3; ++x ){
			if ( win != reels[x]->position() ){
				return -1;
			}
		}
		return win;
	}

	static int num_stopped(){
		int num = 0;
		for ( int x=0; x<3; ++x ){
			if ( reels[x]->stopped ){
				++num;
			}
		}
		return num;
	}

	static void restart() {
		AnointedStopPosition=-1;
		for ( int x=0; x<3; ++x ){
			reels[x]->start();
		}
	}
	
	
	static void little_winner(){
		int winner=reels[0]->position();
		if ( winner == BIG_WINNER ){
			winner++;
		}
		AnointedStopPosition=winner;
		cout << "LITTLE WINNER: " << winner << endl;
	}


	static void big_winner(){
		AnointedStopPosition=BIG_WINNER;
		
		cout << "BIG WINNER" << endl;
	}
	
	static bool all_stopped(){
		return num_stopped() == 3;
	}

	static int stop_position( Reel *reel ){
		int in_position = reel->position();
		if ( AnointedStopPosition != -1 ){
			return AnointedStopPosition;
		}
		cout << "Checking: " << Reel::num_stopped() << endl;
		switch ( Reel::num_stopped() ) {
		case 0:
			return in_position;
			break;
		case 1:
			for ( int x=0; x<3; ++x ){
				if ( reels[ x ]->stopped && reels[ x ]->position() != BIG_WINNER ){
					return reels[x]->position();
				}
			}
			return in_position;
			break;
		default: // two stopped.. see if we won
			int other_stopped;
			for ( int x=0; x<3; ++x ){
				if ( reels[ x ]->stopped ){
					other_stopped = reels[x]->position() ;
				}
			}
			cout << "Two, other: " << other_stopped << endl;

			// if the others are stopped on BIG_WINNER,
			// then don't influence the outcome
			if ( other_stopped == BIG_WINNER ) {
				return in_position;
			}

			int w=(int)get_rand(3);
			cout << "win?: " << (w==1) << endl;
			if ( 1 == w ){ // yep.. 
				return AnointedStopPosition = other_stopped;
			} else { // nope, not this time
				if ( in_position == other_stopped ) { // is it in a winning position
					if ( ++in_position > 9 ){
						in_position=0;
					}
				}
				return AnointedStopPosition=in_position;
			}
		}
	}


	void tick(){
		if ( this->stopped ) {
			return;
		}
		if ( ( speed < 0.2 ) ){
			if ( this->at_stop_position() ){
				if ( Reel::stop_position( this ) == this->position() ) {
					stopped=true;
					cout << "Stopped on: " << this->position() << endl;
					if ( Reel::all_stopped() ){
						int winner = Reel::winner();
						cout << "Winner:     " << winner << endl;
						if ( winner > -1 ) {
							if ( winner == Reel::BIG_WINNER ){
								int channel = play_wav("klaxon.wav");
								while(Mix_Playing(channel) != 0);
							}
							play_wav("jackpot.wav");
						}
					}
				}
			}
		} else {
			speed -= 0.001;
		}
		degree += speed;
		if ( degree > 360 ){
			degree -= 360;
		}

	}


	Reel()
		{
			//		start();
		}

	void
	start() {
		stopped=false;
		speed=get_rand( 2 );
		degree=get_rand( 360 );
		begin=0;
		begin=speed;
	}

	int position(){
		return (int)roundf((degree-30)/40);
	}
	bool at_stop_position(){
		return ( (GLint)roundf( degree+10 ) % 40 == 0 );
	}
};



/* function to release/destroy our resources and restoring the old desktop */
void Quit( int returnCode )
{

    /* Clean up our quadratic */
    gluDeleteQuadric( quadratic );

    /* Clean up our textures */
    glDeleteTextures( 1, &cylinder_side_tex );
    glDeleteTextures( 1, &cylinder_spinner_tex );
    glDeleteTextures( 1, &reels_tex );
    glDeleteTextures( 1, &fireworks_tex );
    /* clean up the window */
    SDL_Quit( );

    records.close();

    /* and exit appropriately */
    exit( returnCode );
}

bool
loadSDLTexture( const char *name, GLuint *addr ){
	SDL_Surface *surface;
	if ( (surface = SDL_LoadBMP(name)) ) { 
 
		// Check that the image's width is a power of 2
		if ( (surface->w & (surface->w - 1)) != 0 ) {
			printf("warning: image %s's width is not a power of 2\n", name );
		}
	
		// Also check if the height is a power of 2
		if ( (surface->h & (surface->h - 1)) != 0 ) {
			printf("warning: image %s's height is not a power of 2\n", name );
		}
        
		// Have OpenGL generate a texture object handle for us
		glGenTextures( 1, addr );

//  		// Bind the texture object
  		glBindTexture( GL_TEXTURE_2D, *addr );
 
		// Set the texture's stretching properties
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
 
		// Edit the texture object's image data using the information SDL_Surface gives us
		glTexImage2D( GL_TEXTURE_2D, 0, 3, surface->w, surface->h, 0,
			      GL_BGR, GL_UNSIGNED_BYTE, surface->pixels );
	} 
	else {
		printf("SDL could not load image %s: %s\n", name, SDL_GetError());
		SDL_Quit();
		return 1;
	}

	if ( surface ) { 
		SDL_FreeSurface( surface );
	}


}
/* function to load in bitmap as a GL texture */
int LoadGLTextures( )
{

	loadSDLTexture( "cyl_side_tex.bmp", &cylinder_side_tex );
	loadSDLTexture( "cyl_spinner_tex.bmp", &cylinder_spinner_tex );
	loadSDLTexture( "reels_tex.bmp", &reels_tex );
	loadSDLTexture( "firework_tex.bmp", &fireworks_tex );
	loadSDLTexture( "alliance-long-blue.bmp", &logo_tex );


    return true;



}


/* function to reset our viewport after a window resize */
int resizeWindow( int width, int height )
{
    /* Height / width ration */
    GLfloat ratio;
 
    /* Protect against a divide by zero */
    if ( height == 0 )
	height = 1;

    ratio = ( GLfloat )width / ( GLfloat )height;

    /* Setup our viewport. */
    glViewport( 0, 0, ( GLint )width, ( GLint )height );

    /* change to the projection matrix and set our viewing volume. */
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity( );

    /* Set our perspective */
    gluPerspective( 45.0f, ratio, 0.1f, 100.0f );

    /* Make sure we're chaning the model view and not the projection */
    glMatrixMode( GL_MODELVIEW );

    /* Reset The View */
    glLoadIdentity( );

    return( TRUE );
}

/* function to handle key press events */
void handleKeyPress( SDL_keysym *keysym )
{
	static bool in_scan=false;
	static std::string scan;

	char ch = keysym->unicode & 0x7F;
	if ( ch == 0 ){
		return;
	}

//  	cout << ch;
//  	cout.flush();

	if ( ch == '%' ){
		in_scan=true;
		return;
	}

	if ( in_scan ){
//		cout << "IN SCAN" << endl;
		if ( boost::ifind_first( scan, ";E?# " ) ){
			cout << "ERR" << endl;
			in_scan=false;
			scan.clear();
			records << time(0) << scan << endl;
			play_wav("error.wav");
		} else if ( boost::ifind_first( scan, "end]" ) ){
			cout << "Scanned: " << scan << endl;
			scan.clear();
			Reel::restart();
			in_scan = false;
		} else {
			scan.push_back( ch );
		}
		return;
	}

	switch ( keysym->sym ){
	case SDLK_ESCAPE:
		/* ESC key was pressed */
		Quit( 0 );
		break;
	case SDLK_w:
		if ( keysym->mod & KMOD_SHIFT ) {
			Reel::big_winner();
		} else {
			Reel::little_winner();
		}
		break;
// 	case SDLK_RETURN:
// 		Reel::stop();
// 		break;
	case SDLK_BACKSPACE:
		Reel::restart();
		break;
	case SDLK_f:
		/* 'f' key was pressed
		 * this toggles fullscreen mode
		 */
		SDL_WM_ToggleFullScreen( surface );
		break;
	default:
		break;
	}

	return;
}

/* general OpenGL initialization function */
int initGL( GLvoid )
{
    /* Load in the texture */
    if ( !LoadGLTextures( ) )
	return FALSE;

    /* Enable smooth shading */
    glShadeModel( GL_SMOOTH );

    /* Set the background black */
    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );

    /* Depth buffer setup */
    glClearDepth( 1.0f );

    glEnable(GL_DEPTH_TEST);
    
    glEnable( GL_LINE_SMOOTH );
    glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
    glLineWidth(1.5);

    /* Really Nice Perspective Calculations */
    glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );
    /* Really Nice Point Smoothing */
    glHint( GL_POINT_SMOOTH_HINT, GL_NICEST );

    /* Enable Texture Mapping ( NEW ) */
    glEnable( GL_TEXTURE_2D );

    /* Select Our Texture */
    glBindTexture( GL_TEXTURE_2D, fireworks_tex );

    /* Setup The Ambient Light */
    glLightfv( GL_LIGHT1, GL_AMBIENT, LightAmbient );

    /* Setup The Diffuse Light */
    glLightfv( GL_LIGHT1, GL_DIFFUSE, LightDiffuse );

    /* Position The Light */
    glLightfv( GL_LIGHT1, GL_POSITION, LightPosition );

    /* Enable Light One */
    glEnable( GL_LIGHT1 );

    /* Create A Pointer To The Quadric Object */
    quadratic = gluNewQuadric( );

    gluQuadricDrawStyle( quadratic, GLU_FILL );
    /* Create Smooth Normals */
    gluQuadricNormals( quadratic, GLU_SMOOTH );
    /* Create Texture Coords */
    gluQuadricTexture( quadratic, GL_TRUE );

    left_firework.reset();
    right_firework.reset();

    /* Set up Waving Logo */
    for ( int x = 0; x < 45; x++ )
        {
	    /* Loop Through The Y Plane */
	    for ( int y = 0; y < 45; y++ )
                {
		    /* Apply The Wave To Our Mesh */
		    points[x][y][0] = ( float )( ( x / 5.0f ) - 4.5f );
		    points[x][y][1] = ( float )( ( y / 20.0f ) - 4.5f );
		    points[x][y][2] = ( float )( sin( ( ( ( x / 5.0f ) * 40.0f ) / 360.0f ) * 3.141592654 * 2.0f ) );
                }
        }

    return( TRUE );
}


void
drawLogo(){

	int x, y;                   /* Loop Variables */
	float f_x, f_y, f_xb, f_yb; /* Used To Break The Flag Into Tiny Quads */

	glPushMatrix();

	glBindTexture( GL_TEXTURE_2D, logo_tex ); /* Select Our Texture */


	glEnable( GL_LIGHTING );
	/* Enable Blending */
	glEnable( GL_BLEND );
	
	/* Type Of Blending To Perform */
	glBlendFunc( GL_SRC_ALPHA, GL_ONE );

	glColor4f(1,1,1,1);//glColor4f(1.0f,1.0f,1.0f,0.5f);			// Full Brightness, 50% Alpha ( NEW )

 	glTranslatef( -0.0f, 0.4f, -12.1f );

	glRotatef( flag_x_axis, 1.0f, 0.0f, -0.0f ); /* Rotate On The X Axis */

	/* Start Drawing Our Quads */
	glBegin( GL_QUADS );

	/* Loop Through The X Plane 0-44 (45 Points) */
	for( x = 0; x < 44; x++ ) {
		/* Loop Through The Y Plane 0-44 (45 Points) */
		for( y = 0; y < 44; y++ ) {
			/* Create A Floating Point X Value */
			f_x = ( float )x / 44.0f;
			/* Create A Floating Point Y Value */
			f_y = ( float )y / 44.0f;
			/* Create A Floating Point Y Value+0.0227f */
			f_xb = ( float )( x + 1 ) / 44.0f;
			/* Create A Floating Point Y Value+0.0227f */
			f_yb = ( float )( y + 1 ) / 44.0f;

			/* First Texture Coordinate (Bottom Left) */
			glTexCoord2f( f_x, f_y );
			glVertex3f( points[x][y][0],
				    points[x][y][1],
				    points[x][y][2] );

			/* Second Texture Coordinate (Top Left) */
			glTexCoord2f( f_x, f_yb );
			glVertex3f( points[x][y + 1][0],
				    points[x][y + 1][1],
				    points[x][y + 1][2] );
			
			/* Third Texture Coordinate (Top Right) */
			glTexCoord2f( f_xb, f_yb );
			glVertex3f( points[x + 1][y + 1][0],
				    points[x + 1][y + 1][1],
				    points[x + 1][y + 1][2] );
			
			/* Fourth Texture Coordinate (Bottom Right) */
			glTexCoord2f( f_xb, f_y );
			glVertex3f( points[x + 1][y][0],
				    points[x + 1][y][1],
				    points[x + 1][y][2] );
                }
        }
	glEnd( );


	/* Used To Slow Down The Wave (Every 2nd Frame Only) */
	if( wiggle_count == 2 ) {
		/* Loop Through The Y Plane */
		for( y = 0; y < 45; y++ ) {
			/* Store Current Value One Left Side Of Wave */
			hold = points[0][y][2];
			/* Loop Through The X Plane */
			for( x = 0; x < 44; x++) {
				/* Current Wave Value Equals Value To The Right */
				points[x][y][2] = points[x + 1][y][2];
                        }
			/* Last Value Becomes The Far Left Stored Value */
			points[44][y][2] = hold;
                }
		wiggle_count = 0; /* Set Counter Back To Zero */
        }
	wiggle_count++; /* Increase The Counter */

	if ( flag_x_axis > 200 ) {
		flag_up = false;
	} else if ( flag_x_axis < 160 ){
		flag_up = true;
	}


	if ( flag_up ){
		flag_x_axis += 0.3f;
	} else {
		flag_x_axis -= 0.3f;
	}
	glPopMatrix();
}

void
drawFireWorks(){

	glPushMatrix();
	/* Enable Blending */
	glEnable( GL_BLEND );
	
	/* Enables Depth Testing */
	glDisable( GL_DEPTH_TEST );

	glDisable( GL_LIGHTING );
	/* Type Of Blending To Perform */
	glBlendFunc( GL_SRC_ALPHA, GL_ONE );

 	glTranslatef( -28.0f, -2.0f, -4.0f );          // move 5 units into the screen.
 	left_firework.tick();

 	glTranslatef( 57.0f, 0.0f, -4.0f );          // move 5 units into the screen.
 	right_firework.tick();

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glPopMatrix();
}

void
drawReels(){

	glEnable( GL_LIGHTING );
	glDisable( GL_BLEND );

	glTranslatef(1.0f,-0.4f,-3.9);


	for ( int x=0; x<3; ++x ){
		Reel *reel = reels[x];
		reel->tick();
	}


	glColor4f( 128,
		   128,
		   128,
		   128 );


	// spinner rod
	glPushMatrix();
	    glRotatef( 90,0.0f,1.0f,0.0f);		  // Rotate On The Y Axis
 	    glTranslatef(0.0f,0.0f,-3.4f);	          // Center the cylinder 
	    glBindTexture( GL_TEXTURE_2D, cylinder_spinner_tex );
	    gluCylinder(quadratic,0.2f,0.2f,5.0f,9,3);    // Draw Our Cylinder 
	glPopMatrix();

	// disc to hide right cylinder insides
	glPushMatrix();
	    glRotatef( reels[0]->degree,1.0f,0.0f,0.0f);	  // Rotate On The X Axis
	    glRotatef( 90,0.0f,1.0f,0.0f);		  // Rotate On The Y Axis
 	    glTranslatef(0.0f,0.0f,-0.20f);	          // Center the cylinder 
	    glBindTexture( GL_TEXTURE_2D, cylinder_side_tex );
	    gluDisk(quadratic,0.2f,1.0f,9,3);             // Draw A Disc (CD Shape)
	glPopMatrix();

	// right cylinder
	glPushMatrix();
	    glRotatef( reels[0]->degree,1.0f,0.0f,0.0f);	  // Rotate On The X Axis
	    glRotatef( 90,0.0f,1.0f,0.0f);		  // Rotate On The Y Axis
 	    glTranslatef(0.0f,0.0f,-0.20f);	          // Center the cylinder 
	    glBindTexture( GL_TEXTURE_2D, reels_tex );
	    gluCylinder(quadratic,1.0f,1.0f,0.7f,9,3);    // Draw Our Cylinder 
	glPopMatrix();

	// center cylinder
	glPushMatrix();
	    glRotatef( reels[1]->degree, 1.0f,0.0f,0.0f);	  // Rotate On The X Axis
	    glRotatef( 90,0.0f,1.0f,0.0f);		  // Rotate On The Y Axis
 	    glTranslatef( 0.0f, 0.0f,-1.38f );	          // Center the cylinder
	    glBindTexture( GL_TEXTURE_2D, reels_tex );
	    gluCylinder(quadratic, 1.0f, 1.0f, 0.7f, 9, 1);    // Draw Our Cylinder
	glPopMatrix();

	// disc to hide left cylinder insides
	glPushMatrix();
	    glRotatef( reels[2]->degree,1.0f,0.0f,0.0f);
	    glRotatef( 90,0.0f,1.0f,0.0f);
	    glTranslatef( 0.0f,0.0f,-1.9f );
	    glBindTexture( GL_TEXTURE_2D, cylinder_side_tex );
	    gluDisk(quadratic,0.2f,1.0f,9,3);             // Draw A Disc (CD Shape)
	glPopMatrix();
	
	// left cylinder
	glPushMatrix();
	    glRotatef( reels[2]->degree,1.0f,0.0f,0.0f);
	    glRotatef( 90,0.0f,1.0f,0.0f);
	    glTranslatef( 0.0f,0.0f,-2.5f );
	    glBindTexture( GL_TEXTURE_2D, reels_tex );
	    gluCylinder(quadratic,1.0f,1.0f,0.6f,9,3);
	glPopMatrix();
}
/* Here goes our drawing code */
int drawGLScene( GLvoid )
{
	/* Clear The Screen And The Depth Buffer */
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	/* Reset the view */
	glLoadIdentity( );

	/* These are to calculate our fps */
	static GLint T0     = 0;
	static GLint Frames = 0;

// 	
	drawLogo();
	drawReels();
//	drawFireWorks();
 	if ( Reel::all_stopped() && Reel::winner() != -1 ){
 		drawFireWorks();
 	}

	/* Draw it to the screen */
	SDL_GL_SwapBuffers( );

	/* Gather our frames per second */
	Frames++;
	{
		GLint t = SDL_GetTicks();
		if (t - T0 >= 5000) {
			GLfloat seconds = (t - T0) / 1000.0;
			GLfloat fps = Frames / seconds;
//			printf("%d frames in %g seconds = %g FPS\n", Frames, seconds, fps);
			T0 = t;
			Frames = 0;
		}
	}

	return( TRUE );
}

int main( int argc, char **argv )
{
	unsigned int r=0;
	ifstream f( "/dev/random",ios::in );
	f.read(reinterpret_cast < char * > (&r), sizeof(r));
	f.close();
	cout << "Rand seed: " << r << endl;
	srand( r );

	for (  int x=0; x<3; ++x ){
		reels[x] = new Reel;
	}

	typedef std::vector< boost::iterator_range<string::iterator> > find_vector_type;
	typedef vector< string > split_vector_type;
    
	std::fstream log( RECORDS_LOG_FILE,std::ios::in);
	if ( log ){
		while( !log.eof()) {
			char line[200];
			log.getline( line, 200 );
			split_vector_type record; // #2: Search for tokens
			boost::split( record, line, boost::is_any_of("\\") );
			if ( record.size() > 2 ){
				try {
					patrons[ boost::lexical_cast<unsigned int>( record[0] ) ] =
						boost::lexical_cast<time_t>( record[1] );
				}
				catch ( const boost::bad_lexical_cast & ) { }
			}
		}
	} else {
		cout << "Unable to open " << RECORDS_LOG_FILE << " to read history from" <<endl;
	}

	records.open( RECORDS_LOG_FILE, ios::out );
	if ( ! records ){
		cerr << "Unable to open " << RECORDS_LOG_FILE << " to write history to" << endl;
	}
    /* Flags to pass to SDL_SetVideoMode */
    int videoFlags;
    /* main loop variable */
    int done = FALSE;
    /* used to collect events */
    SDL_Event event;
    /* this holds some info about our display */
    const SDL_VideoInfo *videoInfo;
    /* whether or not the window is active */
    int isActive = TRUE;

    /* initialize SDL */
    if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO ) < 0 )
	{
	    fprintf( stderr, "SDL initialization failed: %s\n",
		     SDL_GetError( ) );
	    Quit( 1 );
	}
    int audio_rate = 22050;
    Uint16 audio_format = AUDIO_S16SYS;
    int audio_channels = 2;
    int audio_buffers = 4096;

    SDL_ShowCursor(0);
    if( Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers) != 0) {
	    fprintf(stderr, "Unable to initialize audio: %s\n", Mix_GetError());
//	    exit(1);
    }

    Mix_Chunk *sound = NULL;
 // sound = Mix_LoadWAV("click.wav");
//     if(sound == NULL) {
// 	fprintf(stderr, "Unable to load WAV file: %s\n", Mix_GetError());
//     }

    sound = Mix_LoadWAV("crank.wav");
    if(sound == NULL) {
	fprintf(stderr, "Unable to load WAV file: %s\n", Mix_GetError());
    }
    int channel;

//      channel = Mix_PlayChannel(-1, sound, 0);
//      if(channel == -1) {
//  	    fprintf(stderr, "Unable to play WAV file: %s\n", Mix_GetError());
//      }
//      while(Mix_Playing(channel) != 0);
 
     Mix_FreeChunk(sound);

// sound = Mix_LoadWAV("click.wav");
//     if(sound == NULL) {
// 	fprintf(stderr, "Unable to load WAV file: %s\n", Mix_GetError());
//     }

//     channel = Mix_PlayChannel(-1, sound, 0);
//     if(channel == -1) {
// 	    fprintf(stderr, "Unable to play WAV file: %s\n", Mix_GetError());
//     }



    /* Fetch the video info */
    videoInfo = SDL_GetVideoInfo( );

    if ( !videoInfo )
	{
	    fprintf( stderr, "Video query failed: %s\n",
		     SDL_GetError( ) );
	    Quit( 1 );
	}

    /* the flags to pass to SDL_SetVideoMode                            */
    videoFlags  = SDL_OPENGL;          /* Enable OpenGL in SDL          */
    videoFlags |= SDL_GL_DOUBLEBUFFER; /* Enable double buffering       */
    videoFlags |= SDL_HWPALETTE;       /* Store the palette in hardware */
    videoFlags |= SDL_RESIZABLE;       /* Enable window resizing        */

    /* This checks to see if surfaces can be stored in memory */
    if ( videoInfo->hw_available )
	videoFlags |= SDL_HWSURFACE;
    else
	videoFlags |= SDL_SWSURFACE;

    /* This checks if hardware blits can be done */
    if ( videoInfo->blit_hw )
	videoFlags |= SDL_HWACCEL;

    /* Sets up OpenGL double buffering */
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

    /* get a SDL surface */
    surface = SDL_SetVideoMode( SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP,
				videoFlags );

    /* Verify there is a surface */
    if ( !surface )
	{
	    fprintf( stderr,  "Video mode set failed: %s\n", SDL_GetError( ) );
	    Quit( 1 );
	}

    SDL_EnableUNICODE(true);
    /* Enable key repeat */
    if ( ( SDL_EnableKeyRepeat( 100, SDL_DEFAULT_REPEAT_INTERVAL ) ) )
	{
	    fprintf( stderr, "Setting keyboard repeat failed: %s\n",
		     SDL_GetError( ) );
	    Quit( 1 );
	}

    /* initialize OpenGL */
    if ( initGL( ) == FALSE )
	{
	    fprintf( stderr, "Could not initialize OpenGL.\n" );
	    Quit( 1 );
	}

    /* resize the initial window */
    resizeWindow( SCREEN_WIDTH, SCREEN_HEIGHT );

    /* wait for events */
    while ( !done )
	{
	    /* handle the events in the queue */

	    while ( SDL_PollEvent( &event ) )
		{
		    switch( event.type )
			{
			case SDL_ACTIVEEVENT:
			    /* Something's happend with our focus
			     * If we lost focus or we are iconified, we
			     * shouldn't draw the screen
			     */
			    if ( event.active.gain == 0 )
				isActive = FALSE;
			    else
				isActive = TRUE;
			    break;
			case SDL_VIDEORESIZE:
			    /* handle resize event */
			    surface = SDL_SetVideoMode( event.resize.w,
							event.resize.h,
							16, videoFlags );
			    if ( !surface )
				{
				    fprintf( stderr, "Could not get a surface after resize: %s\n", SDL_GetError( ) );
				    Quit( 1 );
				}
			    resizeWindow( event.resize.w, event.resize.h );
			    break;
			case SDL_KEYDOWN:
			    /* handle key presses */
			    handleKeyPress( &event.key.keysym );
			    break;
			case SDL_QUIT:
			    /* handle quit requests */
			    done = TRUE;
			    break;
			default:
			    break;
			}
		}


	    /* If rainbow coloring is turned on, cycle the colors */
//	    if ( rainbow && ( delay > 25 ) )


	    /* draw the scene */
	    //    if ( isActive )
		drawGLScene( );

	}
    
    Mix_CloseAudio();

    /* clean ourselves up and exit */
    Quit( 0 );

    /* Should never get here */
    return( 0 );
}
