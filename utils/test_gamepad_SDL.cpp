/*
 * Test gamepad axis/buttons with SDL
 * 
 * (c) Wintermute0110 <wintermute0110@gmail.com> December 2014
 */
#include <SDL/SDL.h>

SDL_Joystick* joy = NULL;
int SDL_joystick_has_hat = 0;

int SDL_dead_zone = 5000;

int main(int argn, char** argv)
{
	int numJoysticks, i;

	SDL_version compiled;
	SDL_version linked;

	SDL_VERSION(&compiled);
	printf("Sys_InitInput: Compiled with SDL version %d.%d.%d\n", compiled.major, compiled.minor, compiled.patch);
	// SDL_GetVersion(&linked) only available in SDL2
	
	//
	// Joystick initialisation
	//
	printf( "Sys_InitInput: Joystick subsystem init\n" );
	// NOTE: in order to receive Joystick events in SDL1 the video subsystem
	// must be initialised. This will prevent the application from running outside
	// XWindow.
	if( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_JOYSTICK ) ) {
		printf( "Sys_InitInput: SDL_Init() failed: %s\n", SDL_GetError());
		return 0;
	}

	//
	// Print joystick information
	//
	numJoysticks = SDL_NumJoysticks();
	printf( "Sys_InitInput: Joystick subsytem - Found %i joysticks at startup\n", numJoysticks );
	for( i = 0; i < numJoysticks; i++ ) {
		joy = SDL_JoystickOpen( i );
		if( joy ) {
			printf( " Joystick number %i (%s)\n", i, SDL_JoystickName( i ) );
			printf( "  Axes %02d / Buttons %02d / Hats %02d / Balls %02d\n", 
							SDL_JoystickNumAxes( joy ), SDL_JoystickNumButtons( joy ),
							SDL_JoystickNumHats( joy ), SDL_JoystickNumBalls( joy ) );
			SDL_JoystickClose( joy );
		} else {
			printf( "Sys_InitInput: SDL_JoystickOpen() failed: %s\n", SDL_GetError());
		}
	}

	//
	// Open first available joystick and use it
	//
	if( SDL_NumJoysticks() > 0 )
	{
		joy = SDL_JoystickOpen( 0 );
		
		if( joy )
		{
			int num_hats;
			
			num_hats = SDL_JoystickNumHats( joy );
			printf( "Opened joystick number %i (%s)\n", 0, SDL_JoystickName( 0 ) );
			printf( "    axes: %d\n", SDL_JoystickNumAxes( joy ) );
			printf( " buttons: %d\n", SDL_JoystickNumButtons( joy ) );
			printf( "    hats: %d\n", num_hats );
			printf( "   balls: %d\n", SDL_JoystickNumBalls( joy ) );
			
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

	printf("Waiting for joystick events. Press CTRL+C to exit.\n");
	// SDL_EventState(SDL_JOYAXISMOTION, SDL_ENABLE);
	while(run_loop) {
		// SDL_PollEvent() poll event returns inmediately if no events. It consuments 100% CPU!!!
		// SDL_WaitEvent() waits until next event.
		if( SDL_WaitEvent( &ev ) ) {
			switch( ev.type ) {
				case SDL_JOYAXISMOTION:
					// NOTE: jaxis.which is the SDL_JoystickID, not the device index!!!
					if( ev.jaxis.value > SDL_dead_zone || ev.jaxis.value < -SDL_dead_zone) {
						printf("Joystick %02i axis %02i value %i\n", 
									 ev.jaxis.which, ev.jaxis.axis, ev.jaxis.value);
					}
					break;

				case SDL_JOYBUTTONDOWN:
				case SDL_JOYBUTTONUP:
					// NOTE: jbutton.which is the SDL_JoystickID, not the device index!!!
					printf("Joystick %02i button %02i state %i\n", 
								 ev.jbutton.which, ev.jbutton.button, ev.jbutton.state);
					break;
					
				case SDL_JOYHATMOTION:
					// NOTE: jhat.which is the SDL_JoystickID, not the device index!!!
					printf("Joystick %02i hat %02i state ", ev.jhat.which, ev.jhat.hat);
					if( ev.jhat.value & SDL_HAT_UP )
						printf("SDL_HAT_UP ");
					if( ev.jhat.value & SDL_HAT_RIGHT )
						printf("SDL_HAT_RIGHT ");
					if( ev.jhat.value & SDL_HAT_DOWN )
						printf("SDL_HAT_DOWN ");
					if( ev.jhat.value & SDL_HAT_LEFT )
						printf("SDL_HAT_LEFT ");
					if( ev.jhat.value == SDL_HAT_CENTERED )
						printf("SDL_HAT_CENTERED ");
					printf("\n");
					break;
					
				case SDL_QUIT:
					printf("SDL_Event: SDL_QUIT\n");
					run_loop = 0;
					break;
				
				default:
					printf( "Sys_GetEvent: unknown SDL event %u\n", ev.type );
					break;
			}
			fflush(stdout);
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
	
	//
	// Shutdown SDL2
	//
	SDL_QuitSubSystem( SDL_INIT_JOYSTICK );
	
	return 0;
}
