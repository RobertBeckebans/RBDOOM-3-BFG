/*
 * Create joystick gamepad mapping for SDL2
 * 
 * (c) Wintermute0110 <wintermute0110@gmail.com> December 2014
 * Inspired by SDL2 example controllermap.c by Sam Lantinga <slouken@libsdl.org>
 */
#include <SDL2/SDL.h>

// IMPORTANT: SDL gets only keyboard events from a Window it has created. This
// means no keyboad events can be usde in thi application.

// #define __DEBUG_SDL_EVENTS

#define MARKER_BUTTON 1
#define MARKER_AXIS 2

typedef struct MappingStep
{
	int marker;
	char field[256];
	int axis, button, hat, hat_value;
	char mapping[4096];
}MappingStep;

SDL_Joystick *joy = NULL;
SDL_JoystickID instanceID = -1; // Joystick instance ID. Changes if there are hotplug events!!!
int device_index_in_use = -1; // This is the devic number in use
int SDL_joystick_is_gamepad = 0; // True if joystick is a SDL2 recognised gamepad

int SDL_dead_zone = 10000;

void* my_callback_param = NULL;
Uint32 my_callbackfunc(Uint32 interval, void *param)
{
    SDL_Event event;
    SDL_UserEvent userevent;

    /* In this example, our callback pushes an SDL_USEREVENT event
    into the queue, and causes our callback to be called again at the
    same interval: */

    userevent.type = SDL_USEREVENT;
    userevent.code = 0;
    userevent.data1 = NULL;
    userevent.data2 = NULL;

    event.type = SDL_USEREVENT;
    event.user = userevent;

    SDL_PushEvent(&event);
		
		// Cancel timer. Will be activated again for next key press
    return 0;
}

int main(int argn, char** argv)
{
	int numJoysticks, i;
	SDL_version compiled;
	SDL_version linked;
	SDL_Event ev;
	
	const char *name = NULL;
	MappingStep *step;
	MappingStep steps[] = {
		{MARKER_BUTTON, "x", -1, -1, -1, -1, ""},
		{MARKER_BUTTON, "a", -1, -1, -1, -1, ""},
		{MARKER_BUTTON, "b", -1, -1, -1, -1, ""},
		{MARKER_BUTTON, "y", -1, -1, -1, -1, ""},
		{MARKER_BUTTON, "back", -1, -1, -1, -1, ""},
		{MARKER_BUTTON, "guide", -1, -1, -1, -1, ""},
		{MARKER_BUTTON, "start", -1, -1, -1, -1, ""},        
		{MARKER_BUTTON, "dpleft", -1, -1, -1, -1, ""},
		{MARKER_BUTTON, "dpdown", -1, -1, -1, -1, ""},
		{MARKER_BUTTON, "dpright", -1, -1, -1, -1, ""},
		{MARKER_BUTTON, "dpup", -1, -1, -1, -1, ""},
		{MARKER_BUTTON, "leftshoulder", -1, -1, -1, -1, ""},
		{MARKER_BUTTON, "lefttrigger", -1, -1, -1, -1, ""},
		{MARKER_BUTTON, "rightshoulder", -1, -1, -1, -1, ""},
		{MARKER_BUTTON, "righttrigger", -1, -1, -1, -1, ""},
		{MARKER_BUTTON, "leftstick", -1, -1, -1, -1, ""},
		{MARKER_BUTTON, "rightstick", -1, -1, -1, -1, ""},
		{MARKER_AXIS, "leftx", -1, -1, -1, -1, ""},
		{MARKER_AXIS, "lefty", -1, -1, -1, -1, ""},        
		{MARKER_AXIS, "rightx", -1, -1, -1, -1, ""},
		{MARKER_AXIS, "righty", -1, -1, -1, -1, ""},
	};
		
	SDL_VERSION(&compiled);
	printf("Sys_InitInput: Compiled with SDL version %d.%d.%d\n", compiled.major, compiled.minor, compiled.patch);
	SDL_GetVersion(&linked);
	printf("Sys_InitInput: Linked   with SDL version %d.%d.%d\n", linked.major, linked.minor, linked.patch);

	//
	// Joystick initialisation
	//
	printf( "Sys_InitInput: Joystick subsystem init\n" );
	if( SDL_Init( SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC ) )
	{
		printf( "Sys_InitInput: SDL_Init() failed: %s\n", SDL_GetError());
		
		return 0;
	}
	
	//
	// Load controller mappings
	//
	int num_devices = SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt");
	if( num_devices < 0 ) {
		printf( "Sys_InitInput: SDL_GameControllerAddMappingsFromFile() failed: %s\n", SDL_GetError());
	} else {
		printf( "Sys_InitInput: SDL_GameControllerAddMappingsFromFile() added %i controller maps\n", num_devices );
	}

	//
	// Print joystick information
	//
	numJoysticks = SDL_NumJoysticks();
	printf( "Sys_InitInput: Joystick subsytem - Found %i joysticks at startup\n", numJoysticks );
	printf("Joysticks present at startup\n");
	for( i = 0; i < numJoysticks; i++ ) {
		joy = SDL_JoystickOpen( i );
		if( joy ) {
			char guid[64];
			
			SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joy), guid, sizeof (guid));
			
			
			printf( " Number %i (%s)\n", i, SDL_JoystickName( joy ) );
			printf( "  Joystick number %i is %s game controller\n", 
							i, SDL_IsGameController( i ) ? "a" : "not a");
			printf( "  Axes %02d / Buttons %02d / Hats %02d / Balls %02d\n", 
							SDL_JoystickNumAxes( joy ), SDL_JoystickNumButtons( joy ),
							SDL_JoystickNumHats( joy ), SDL_JoystickNumBalls( joy ) );
			printf( "  Instance id %d\n", SDL_JoystickInstanceID( joy ) );
			printf( "  Guid %s\n", guid);
			SDL_JoystickClose( joy );
		} else {
			printf( "Sys_InitInput: SDL_JoystickOpen() failed: %s\n", SDL_GetError());
		}
	}

	//
	// Open first available joystick and use it
	//
	int gamepad_idx_to_open = 0;
	
	if( numJoysticks > 0 )
	{
		joy = SDL_JoystickOpen( gamepad_idx_to_open );
		if (joy == NULL ) {
			printf( "SDL_JoystickOpen failed: %s\n", SDL_GetError() );
			printf( "Couldn't open joystick %i\n", gamepad_idx_to_open );
			joy = NULL;
			device_index_in_use = -1;
		} else {
			int num_hats;
			char guid[64];

			name = SDL_JoystickName( joy );
			instanceID = SDL_JoystickInstanceID(joy);
			device_index_in_use = gamepad_idx_to_open;
			SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joy), guid, sizeof (guid));
			num_hats = SDL_JoystickNumHats( joy );
			printf( "Watching joystick device index %i (%s)\n", device_index_in_use, name );
			printf( "        axes: %d\n", SDL_JoystickNumAxes( joy ) );
			printf( "     buttons: %d\n", SDL_JoystickNumButtons( joy ) );
			printf( "        hats: %d\n", num_hats );
			printf( "       balls: %d\n", SDL_JoystickNumBalls( joy ) );
			printf( " instance id: %d\n", SDL_JoystickInstanceID( joy ) );
			printf( "        guid: %s\n", guid);
		}
	} else {
		printf("No joysticks found. Exiting\n");
		
		return 0;
	}

	//
	// Ask user to press joystick buttons/axis to get mapping
	//
	char mapping[4096], temp[4096];
	int s, _s;
	SDL_bool done = SDL_FALSE, next=SDL_FALSE;
	Uint32 delay = (3000 / 10) * 10;  /* To round it down to the nearest 10 ms */
	SDL_TimerID my_timer_id = 0;

	printf("\
====================================================================================\n\
* Press the buttons/axes on your controller when indicated\n\
* To skip a button wait 3 seconds for a timeout\n\
* To exit cancelling everything, press CTRL+C\n\
====================================================================================\n");
	
	/* Initialize mapping with GUID and name */
	SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joy), temp, SDL_arraysize(temp));
	SDL_snprintf(mapping, SDL_arraysize(mapping), "%s,%s,platform:%s,",
			temp, name ? name : "Unknown Joystick", SDL_GetPlatform());

	// Flush events
	// My Logitech F710 produces a couple of random axis events at startup...
	while(SDL_PollEvent(&ev)) {};
	
	
	/* Loop, getting joystick events! */
	for(s = 0; s < SDL_arraysize(steps) && !done;) 
	{
		/* Print button/axis to map */
		step = &steps[s];
		SDL_strlcpy(step->mapping, mapping, SDL_arraysize(step->mapping));
		step->axis = -1;
		step->button = -1;
		step->hat = -1;
		step->hat_value = -1;
		
		printf("Press button %s\n", step->field);
		fflush(stdout);
		
		// Start timeout timer, and cancel previous timeout.
		SDL_RemoveTimer(my_timer_id);
		my_timer_id = SDL_AddTimer(delay, my_callbackfunc, my_callback_param);
	
		next = SDL_FALSE;
		while (!done && !next) 
		{
			// SDL_PollEvent / SDL_WaitEvent
			if(SDL_WaitEvent(&ev) ) 
			{
				switch (ev.type) 
				{
					case SDL_JOYAXISMOTION:
						// Deadzone is very high to avoid mistakes
						if (ev.jaxis.value > 20000 || ev.jaxis.value < -20000) {
							for (_s = 0; _s < s; _s++) {
								if (steps[_s].axis == ev.jaxis.axis) {
										break;
								}
							}
							if (_s == s) {
								step->axis = ev.jaxis.axis;
								SDL_strlcat(mapping, step->field, SDL_arraysize(mapping));
								SDL_snprintf(temp, SDL_arraysize(temp), ":a%u,", ev.jaxis.axis);
								SDL_strlcat(mapping, temp, SDL_arraysize(mapping));
								s++;
								next = SDL_TRUE;
							}
						}
						break;
						
					case SDL_JOYHATMOTION:
#ifdef __DEBUG_SDL_EVENTS
						printf("Joystick %02i / Hat = %02i / State = ", ev.jhat.which, ev.jhat.hat);
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
#endif
						// Hat SDL_HAT_CENTERED must be ignored. They are the equivalent to
						// button UP events
						if( ev.jhat.value == SDL_HAT_CENTERED)
							break;
						
						// Ignore already mapped hat value
						for (_s = 0; _s < s; _s++) {
							if (steps[_s].hat == ev.jhat.hat && steps[_s].hat_value == ev.jhat.value) {
								break;
							}
						}
						if (_s == s) {
							step->hat = ev.jhat.hat;
							step->hat_value = ev.jhat.value;
							SDL_strlcat(mapping, step->field, SDL_arraysize(mapping));
							SDL_snprintf(temp, SDL_arraysize(temp), ":h%u.%u,", ev.jhat.hat, ev.jhat.value );
							SDL_strlcat(mapping, temp, SDL_arraysize(mapping));
							s++;
							next = SDL_TRUE;
						}
						break;
						
					case SDL_JOYBALLMOTION:
						break;
						
					case SDL_JOYBUTTONUP:
						// Ignore this event
						break;
						
					case SDL_JOYBUTTONDOWN:
						// Ignore already mapped buttons
						for (_s = 0; _s < s; _s++) {
							if (steps[_s].button == ev.jbutton.button) {
								break;
							}
						}
						if (_s == s) {
							step->button = ev.jbutton.button;
							SDL_strlcat(mapping, step->field, SDL_arraysize(mapping));
							SDL_snprintf(temp, SDL_arraysize(temp), ":b%u,", ev.jbutton.button);
							SDL_strlcat(mapping, temp, SDL_arraysize(mapping));
							s++;
							next=SDL_TRUE;
						}
						break;

					case SDL_USEREVENT:
						// Timeout. The gamepad has not this button/axis
#ifdef __DEBUG_SDL_EVENTS
						printf("Skipping button/axis (timeout)\n");
#endif
						s++;
						next = SDL_TRUE;
						break;
						
#if 0
					// See NOTE at the beginning of the code about keyboad events in SDL
					// console applications.
					case SDL_KEYDOWN:
						printf("Key event.\n");
						fflush(stdout);
						
						if (ev.key.keysym.sym == SDLK_BACKSPACE || ev.key.keysym.sym == SDLK_AC_BACK) {
							/* Undo! */
							if (s > 0) {
								printf("Undoing last button/axis. (SDLK_BACKSPACE / SDLK_AC_BACK)\n");
								SDL_strlcpy(mapping, step->mapping, SDL_arraysize(step->mapping));
								s--;
								next = SDL_TRUE;
							}
							break;
						}
						else if (ev.key.keysym.sym == SDLK_SPACE) {
							/* Skip this step */
							printf("Skipping button/axis. (SDLK_SPACE)\n");
							s++;
							next = SDL_TRUE;
							break;
						}
						else if ((ev.key.keysym.sym != SDLK_ESCAPE)) {
							printf("Exiting. (SDLK_SPACE)\n");
							break;
						}
						break;
#endif
						
					/* Fall through to signal quit */
					case SDL_QUIT:
#ifdef __DEBUG_SDL_EVENTS
						printf("Exiting. (SDL_QUIT)\n");
#endif
						done = SDL_TRUE;
						break;
						
					default:
#ifdef __DEBUG_SDL_EVENTS
						printf("Unknown SDL event %#06x\n", ev.type);
#endif
						break;
				}
			}
			
			fflush(stdout);
		}
	}

	if (s == SDL_arraysize(steps) ) {
		/* Print to stdout as well so the user can cat the output somewhere */
		printf("-- Mapping ---------------------------------------\n");
		printf("%s\n", mapping);
		printf("--------------------------------------------------\n");
	}
	
	// Flush events
	while(SDL_PollEvent(&ev)) {};
	
	//
	// Close joystick
	//
	if( joy ) {
		printf( "Sys_ShutdownInput: closing SDL joystick.\n" );
		SDL_JoystickClose( joy );
		joy = NULL;
	}
	
	//
	// Shutdown SDL2
	//
	SDL_QuitSubSystem( SDL_INIT_JOYSTICK );
	
	return 0;
}
