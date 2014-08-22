#include "Common.h"
#include "Network.h"
#include <time.h>
#include <stdlib.h>

#ifdef CLIENT
#include <GL/glew.h>
#include <GLUT/glut.h>
#endif

using namespace protocol;

#ifdef CLIENT

void init() // Called before main loop to set up the program
{
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
}

// Called at the start of the program, after a glutPostRedisplay() and during idle
// to display a frame
void display()
{
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glLoadIdentity();

    glBegin( GL_TRIANGLES );
        glVertex3f(0.0, 0.0, -10.0);
        glVertex3f(1.0, 0.0, -10.0);
        glVertex3f(0.0, 1.0, -10.0);
    glEnd();

    glutSwapBuffers();
}

// Called every time a window is resized to resize the projection matrix
void reshape(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-0.1, 0.1, -float(h)/(10.0*float(w)), float(h)/(10.0*float(w)), 0.5, 1000.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

int main( int argc, char ** argv )
{
    srand( time( nullptr ) );

    if ( !InitializeNetwork() )
    {
        printf( "failed to initialize network\n" );
        return 1;
    }

    PROTOCOL_ASSERT( IsNetworkInitialized() );

    glutInit( &argc, argv );

/*
    GLenum err = glewInit();
    if ( GLEW_OK != err )
    {
        printf( "error: glewInit failed\n" );
        return 1;
    }
*/
    printf( "successfully created game window\n" );

    glutInitDisplayMode( GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA );

    glutInitWindowSize( 1000, 600 );

    glutInitWindowPosition( 220, 150 );

    glutCreateWindow( "Client" );

    // passes reshape and display functions to the OpenGL machine for callback
    glutReshapeFunc( reshape );
    glutDisplayFunc( display );
    glutIdleFunc( display );

    init();

    // Starts the program.
    glutMainLoop();

    ShutdownNetwork();

    return 0;
}

#else

int main( int argc, char ** argv )
{
    printf( "hello world server\n" );
    return 0;
}

#endif
