/*
 * Test gamepad axis/buttons with SDL
 * 
 */
#include <SDL/SDL.h>

SDL_Joystick* joy = NULL;
int SDL_joystick_has_hat = 0;

int main(int argn, char** argv)
{
	int numJoysticks, i;

	SDL_version compiled;
	SDL_version linked;

	SDL_VERSION(&compiled);
	printf("Sys_InitInput: Compiled with SDL version %d.%d.%d\n", compiled.major, compiled.minor, compiled.patch);
	// Only available in SDL2
//	SDL_GetVersion(&linked);
//	printf("Sys_InitInput: Linked   with SDL version %d.%d.%d\n", linked.major, linked.minor, linked.patch);
	
	//
	// Joystick initialisation
	//
	printf( "Sys_InitInput: Joystick subsystem init\n" );
	if( SDL_Init( SDL_INIT_JOYSTICK ) )
	{
		printf( "Sys_InitInput: Joystick Init ERROR!\n" );
	}
	
	numJoysticks = SDL_NumJoysticks();
	printf( "Sys_InitInput: Joystick - Found %i joysticks\n", numJoysticks );
	for( i = 0; i < numJoysticks; i++ )
		printf( " Joystick %i name '%s'\n", i, SDL_JoystickName( i ) );

	// Open first available joystick and use it
	if( SDL_NumJoysticks() > 0 )
	{
		joy = SDL_JoystickOpen( 0 );
		
		if( joy )
		{
			int num_hats;
			
			num_hats = SDL_JoystickNumHats( joy );
			printf( "Opened joystick number %i (%s)\n", 0, SDL_JoystickName( 0 ) );
			printf( "Name: %s\n", SDL_JoystickName( 0 ) );
			printf( "Number of Axes: %d\n", SDL_JoystickNumAxes( joy ) );
			printf( "Number of Buttons: %d\n", SDL_JoystickNumButtons( joy ) );
			printf( "Number of Hats: %d\n", num_hats );
			printf( "Number of Balls: %d\n", SDL_JoystickNumBalls( joy ) );
			
			SDL_joystick_has_hat = 0;
			if( num_hats )
			{
				SDL_joystick_has_hat = 1;
			}
		}
		else
		{
			joy = NULL;
			printf( "Couldn't open joystick 0\n" );
		}
	}
	else
	{
		joy = NULL;
	}

	//
	// If no joystick found then exit
	//
	if (joy == NULL )
		return 0;

	//
	// Get joystick events and print information
	//
	int run_loop = 1;
	SDL_Event ev;

	while(run_loop) {
		if( SDL_PollEvent( &ev ) ) {
			switch( ev.type ) {
				case SDL_JOYBUTTONDOWN:
					printf("SDL_JOYBUTTONDOWN: jbutton.button = %i / jbutton.state = %i\n", ev.jbutton.button, ev.jbutton.state);
					break;
					
				case SDL_JOYBUTTONUP:
					printf("SDL_JOYBUTTONUP  : jbutton.button = %i / jbutton.state = %i\n", ev.jbutton.button, ev.jbutton.state);
					break;
					
				case SDL_JOYHATMOTION:
					printf("SDL_JOYBUTTONDOWN: jhat.value = %i\n", ev.jhat.value);
					break;
					
				case SDL_JOYAXISMOTION:
					printf("SDL_JOYBUTTONDOWN: jaxis.axis = %i / jaxis.value = %i\n", ev.jaxis.axis, ev.jaxis.value);
					break;
					
				case SDL_QUIT:
					printf("SDL_Event: SDL_QUIT\n");
					run_loop = 0;
					break;
				
				default:
					printf( "Sys_GetEvent: unknown SDL event %u", ev.type );
					break;
			}
		}
	}

	//
	// Close joystick
	//
	if( joy )
	{
		printf( "Sys_ShutdownInput: closing SDL joystick.\n" );
		SDL_JoystickClose( joy );
	}
	else
	{
		printf( "Sys_ShutdownInput: SDL joystick not initialized. Nothing to close.\n" );
	}
	
	return 0;
}
