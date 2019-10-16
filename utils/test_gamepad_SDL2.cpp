/*
 * Test gamepad axis/buttons with SDL
 * Also tests new SDL2 features: game controller, joystick/gamepad 
 * hotplug and haptics/rumble.
 * 
 * (c) Wintermute0110 <wintermute0110@gmail.com> December 2014
 */
#include <SDL2/SDL.h>

#define __SDL2_ENABLE_CONTROLLER_HOTPLUG

SDL_Joystick *joy = NULL;
int SDL_joystick_has_hat = 0; // Game controller do not have hats or balls (only joysticks)

SDL_GameController *gamepad = NULL;
SDL_JoystickID instanceID = -1; // Joystick instance ID. Changes if there are hotplug events!!!
int device_index_in_use = -1; // This is the devic number in use
const int SDL_wanted_joystick_number = 0; // Joystick device index user wants to use
SDL_Haptic *haptic = NULL;
int SDL_joystick_is_gamepad = 0; // True if joystick is a SDL2 recognised gamepad

int SDL_dead_zone = 5000;

void SDL2_Init_Haptic_From_Joystick(void)
{
	// Test for haptic num_devices
	// NOTE: mouses may have haptics, and not the joystick in use
	printf( "Sys_InitInput: %d haptic devices detected.\n", SDL_NumHaptics());

	// Try to open haptic from used joystick
	if( joy ) {
		if( SDL_JoystickIsHaptic(joy) ) {
			haptic = SDL_HapticOpenFromJoystick( joy );
			if( haptic == NULL ) {
				printf( "SDL_HapticOpenFromJoystick() failed: %s\n", SDL_GetError() );
			} else {
				if( SDL_HapticRumbleSupported(haptic) == SDL_FALSE) {
					printf( "WARNING: Rumble not supported!\n");
					SDL_HapticClose(haptic);
					haptic = NULL;
				} else {
					if (SDL_HapticRumbleInit(haptic) != 0) {
						printf( "WARNING: to initialize rumble: %s\n", SDL_GetError());
						SDL_HapticClose(haptic);
						haptic = NULL;
					} else {
						printf( "Sys_InitInput: Rumble initialization OK\n");
					}
				}
			}
		} else {
			printf("Joystick does not support haptics/rumble");
			haptic = NULL;
		}
	}
}

int main(int argn, char** argv)
{
	int numJoysticks, i;

	SDL_version compiled;
	SDL_version linked;

	SDL_VERSION(&compiled);
	printf("Sys_InitInput: Compiled with SDL version %d.%d.%d\n", compiled.major, compiled.minor, compiled.patch);
	SDL_GetVersion(&linked);
	printf("Sys_InitInput: Linked   with SDL version %d.%d.%d\n", linked.major, linked.minor, linked.patch);
	
	//
	// Joystick initialisation
	//
	printf( "Sys_InitInput: Joystick subsystem init\n" );
	// NOTE: In SDL2, and contrary to what happens in SDL1, joystick events are
	// received even if video is not initialisated.
	// NOTE: Very interesting. If video is initialised in SDL2 then only button 
	// UP events work! (Tested with a Logitech F710 wireless)!!!
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
	for( i = 0; i < numJoysticks; i++ ) {
		joy = SDL_JoystickOpen( i );
		if( joy ) {
			char guid[64];
			
			SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joy), guid, sizeof (guid));
			
			
			printf( " Joystick number %i (%s)\n", i, SDL_JoystickName( joy ) );
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
	if( numJoysticks > 0 )
	{
		int gamepad_idx_to_open = SDL_wanted_joystick_number;
		
		// Try to open joystick as controller
		gamepad = SDL_GameControllerOpen( gamepad_idx_to_open );
		if( gamepad == NULL ) {
			SDL_joystick_is_gamepad = 0;
			// If fails open joystick as joystick (old SDL joystick API)
			printf("SDL_GameControllerOpen failed: %s\n", SDL_GetError());
			printf("Opening joystick with old SDL joystick API\n");

			joy = SDL_JoystickOpen( gamepad_idx_to_open );
			if (joy == NULL ) {
				printf( "SDL_JoystickOpen failed: %s\n", SDL_GetError() );
				printf( "Couldn't open joystick %i\n", gamepad_idx_to_open );
				joy = NULL;
				instanceID = -1;
				device_index_in_use = -1;
			} else {
				int num_hats;
				char guid[64];

				instanceID = SDL_JoystickInstanceID(joy);
				device_index_in_use = gamepad_idx_to_open;
				SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joy), guid, sizeof (guid));
				num_hats = SDL_JoystickNumHats( joy );
				printf( "Opened joystick device index %i (%s)\n", 
								                device_index_in_use, SDL_JoystickNameForIndex( device_index_in_use ) );
				printf( "        axes: %d\n", SDL_JoystickNumAxes( joy ) );
				printf( "     buttons: %d\n", SDL_JoystickNumButtons( joy ) );
				printf( "        hats: %d\n", num_hats );
				printf( "       balls: %d\n", SDL_JoystickNumBalls( joy ) );
				printf( " instance id: %d\n", SDL_JoystickInstanceID( joy ) );
				printf( "        guid: %s\n", guid);
				
				SDL_joystick_has_hat = 0;
				if( num_hats )
					SDL_joystick_has_hat = 1;
			}
		} else {
			SDL_joystick_is_gamepad = 1;
			joy = SDL_GameControllerGetJoystick( gamepad );
			instanceID = SDL_JoystickInstanceID(joy);
			device_index_in_use = gamepad_idx_to_open;

			// Remember that gamepads do not have hats in SDL2
			int num_axes = 0;
			int num_buttons = 0;
			char guid[64];

			SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joy), guid, sizeof (guid));
			printf( "Opened gamepad device index %i (%s)\n", 
							             device_index_in_use, SDL_GameControllerNameForIndex( device_index_in_use ) );
			printf( "        axes: %d\n", SDL_JoystickNumAxes( joy ) );
			printf( "     buttons: %d\n", SDL_JoystickNumButtons( joy ) );
			printf( " instance id: %d\n", SDL_JoystickInstanceID( joy ) );
			printf( "        guid: %s\n", guid);
		}
	} else {
		gamepad = NULL;
		joy = NULL;
		instanceID = -1;
		device_index_in_use = -1;
	}

	// Start haptic from opened joystick
	SDL2_Init_Haptic_From_Joystick();
	
	//
	// If no joystick found then exit
	// NOTE: in SDL2 joysticks can be hot-plugged

	//
	// Get joystick events and print information
	//
	int run_loop = 1;
	SDL_Event ev;
	
#ifdef __SDL2_ENABLE_CONTROLLER_HOTPLUG
	printf("SDL2: Joytick hotplug supported\n");
#endif
	printf("Waiting for joystick events. Press CTRL+C to exit.\n");
	while(run_loop) {
		// SDL_PollEvent() poll event returns inmediately if no events. It consuments 100% CPU!!!
		// SDL_WaitEvent() waits until next event
		if( SDL_WaitEvent( &ev ) ) {
			switch( ev.type ) {
				// SDL joystick API events /////////////////////////////////////////////////////////
				case SDL_JOYAXISMOTION:
					// NOTE: jaxis.which is the SDL_JoystickID, not the device index!!!
					if( ev.jaxis.value > SDL_dead_zone || ev.jaxis.value < -SDL_dead_zone) {
						printf("Joystick   %02i axis %02i value %i\n", 
									 ev.jaxis.which, ev.jaxis.axis, ev.jaxis.value);
					}
					break;

				case SDL_JOYBUTTONDOWN:
				case SDL_JOYBUTTONUP:
					// NOTE: jbutton.which is the SDL_JoystickID, not the device index!!!
					printf("Joystick   %02i button %02i state %i\n", 
								 ev.jbutton.which, ev.jbutton.button, ev.jbutton.state);
					break;
					
				case SDL_JOYHATMOTION:
					// NOTE: jhat.which is the SDL_JoystickID, not the device index!!!
					printf("Joystick   %02i hat %02i state ", ev.jhat.which, ev.jhat.hat);
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

				// SDL2 joystick hotplug events
				case SDL_JOYDEVICEADDED:
					// WARNING: which is the joystick device index for the SDL_CONTROLLERDEVICEADDED event 
					// but it is the instance id for the SDL_CONTROLLERDEVICEREMOVED 
					// or SDL_CONTROLLERDEVICEREMAPPED event.
					printf("SDL_JOYDEVICEADDED jdevice.which %02i (%s) [DEVICE INDEX]\n", 
								 ev.jdevice.which, SDL_JoystickNameForIndex(ev.jdevice.which));
#ifdef __SDL2_ENABLE_CONTROLLER_HOTPLUG
					if( gamepad ) {
						// If a valid controller is already opened do nothing
						printf( " Gamepad %02i (%s) in use\n", 
										                  device_index_in_use, SDL_GameControllerNameForIndex(device_index_in_use));
						printf( " Ignoring plugged-in joystick device number %02i (%s)\n", 
										ev.jdevice.which, SDL_JoystickNameForIndex(ev.jdevice.which));
					} else if( joy ) {
						// If a valid joystick is already opened do nothing
						printf( " Joystick %02i (%s) in use\n", 
										                  device_index_in_use, SDL_JoystickNameForIndex(device_index_in_use));
						printf( " Ignoring plugged-in joystick device number %02i (%s)\n", 
										ev.jdevice.which, SDL_JoystickNameForIndex(ev.jdevice.which));
					} else {
						int gamepad_idx_to_open = ev.jdevice.which;

						// Open joystick for use
						SDL_joystick_is_gamepad = 0;
						// If fails open joystick as joystick (old SDL joystick API)
						printf("Opening joystick with joystick API\n");
						printf( " Device %02i %s a game controller\n", 
									  gamepad_idx_to_open, SDL_IsGameController(gamepad_idx_to_open) ? "is" : "is not" );

						joy = SDL_JoystickOpen( gamepad_idx_to_open );
						if (joy == NULL ) {
							printf( "SDL_JoystickOpen failed: %s\n", SDL_GetError() );
							printf( "Couldn't open joystick %i\n", gamepad_idx_to_open );
							joy = NULL;
							instanceID = -1;
							device_index_in_use = -1;
						} else {
							int num_hats;
							char guid[64];

							instanceID = SDL_JoystickInstanceID(joy);
							device_index_in_use = gamepad_idx_to_open;
							SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joy), guid, sizeof (guid));
							num_hats = SDL_JoystickNumHats( joy );
							printf( "Opened joystick device index %i (%s)\n", 
											device_index_in_use, SDL_JoystickNameForIndex( device_index_in_use ) );
							printf( "        axes: %d\n", SDL_JoystickNumAxes( joy ) );
							printf( "     buttons: %d\n", SDL_JoystickNumButtons( joy ) );
							printf( "        hats: %d\n", num_hats );
							printf( "       balls: %d\n", SDL_JoystickNumBalls( joy ) );
							printf( " instance id: %d\n", SDL_JoystickInstanceID( joy ) );
							printf( "        guid: %s\n", guid);

							SDL_joystick_has_hat = 0;
							if( num_hats )
								SDL_joystick_has_hat = 1;
						}
					}
#endif
					// Start haptic from opened joystick
					SDL2_Init_Haptic_From_Joystick();
					break;

				case SDL_JOYDEVICEREMOVED:
					// WARNING: which is the joystick device index for the SDL_CONTROLLERDEVICEADDED event 
					// but it is the instance id for the SDL_CONTROLLERDEVICEREMOVED 
					// or SDL_CONTROLLERDEVICEREMAPPED event.
					printf("SDL_JOYDEVICEREMOVED jdevice.which %02i [INSTANCE ID]\n", ev.jdevice.which);
#ifdef __SDL2_ENABLE_CONTROLLER_HOTPLUG
					if( joy && instanceID == ev.jdevice.which ) {
						// If a joystick is in use opened check if the user unplugged it
						printf(" Joystick device number %02i in use was unplugged. Closing it\n", device_index_in_use );
						SDL_JoystickClose( joy );
						joy = NULL;
						instanceID = -1;
						device_index_in_use = -1;
					} else {
						// If not in use do nothing
						printf( " Unplugged joystick %02i not in use. Doing nothing\n", ev.jdevice.which );
					}
#endif
					break;
					
				// SDL controller API events ///////////////////////////////////////////////////////
				case SDL_CONTROLLERAXISMOTION:
					if( ev.caxis.value > SDL_dead_zone || ev.caxis.value < -SDL_dead_zone) {
						printf("Controller %02i axis %02i value %02i axis name %s\n", 
									ev.caxis.which, ev.caxis.axis, ev.caxis.value,
									SDL_GameControllerGetStringForAxis((SDL_GameControllerAxis)ev.caxis.axis) );
					}
					break;

				case SDL_CONTROLLERBUTTONDOWN:
				case SDL_CONTROLLERBUTTONUP:
					printf("Controller %02i button %02i state %i button name %s\n", 
								 ev.cbutton.which, ev.cbutton.button, ev.cbutton.state, 
								 SDL_GameControllerGetStringForButton((SDL_GameControllerButton)ev.cbutton.button) );
					break;
					
				case SDL_CONTROLLERDEVICEADDED:
					// WARNING: which is the joystick device index for the SDL_CONTROLLERDEVICEADDED event 
					// but it is the instance id for the SDL_CONTROLLERDEVICEREMOVED 
					// or SDL_CONTROLLERDEVICEREMAPPED event.
					printf("SDL_CONTROLLERDEVICEADDED cdevice.which %02i (%s) [DEVICE INDEX]\n", 
								 ev.cdevice.which, SDL_GameControllerNameForIndex(ev.cdevice.which));
#ifdef __SDL2_ENABLE_CONTROLLER_HOTPLUG
					if( gamepad ) { 
						// If a valid controller is already opened do nothing
						printf( " Gamepad %02i (%s) already in use\n", 
										device_index_in_use, SDL_GameControllerNameForIndex(device_index_in_use));
						printf( " Ignoring plugged-in gamepad %02i (%s)\n", 
										ev.cdevice.which, SDL_GameControllerNameForIndex(ev.cdevice.which) );
					} else {
						int gamepad_idx_to_open = ev.cdevice.which;
					
						// Open and start using the newly connected device
						gamepad = SDL_GameControllerOpen( gamepad_idx_to_open );
						if( gamepad == NULL ) {
							// If fails open joystick as joystick (old SDL joystick API)
							printf(" SDL_GameControllerOpen failed: %s\n", SDL_GetError());
							printf(" Opening joystick with old SDL joystick API\n");
							SDL_joystick_is_gamepad = 0;
							// Don't try to open the gamepad as a joystick
							// If a controller is hotplugged two events are received, first a
							// gamepad event and then a joystick event for the same device. 
							// Open the joystick in the joystick plugged event and not here.
						} else {
							SDL_joystick_is_gamepad = 1;
							joy = SDL_GameControllerGetJoystick( gamepad );
							instanceID = SDL_JoystickInstanceID(joy);
							device_index_in_use = gamepad_idx_to_open;

							// Remember that gamepads do not have hats in SDL2
							int num_axes = 0;
							int num_buttons = 0;
							char guid[64];

							SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joy), guid, sizeof (guid));
							printf( "Opened gamepad device index %i (%s)\n", 
											gamepad_idx_to_open, SDL_GameControllerNameForIndex( gamepad_idx_to_open ) );
							printf( "        axes: %d\n", SDL_JoystickNumAxes( joy ) );
							printf( "     buttons: %d\n", SDL_JoystickNumButtons( joy ) );
							printf( " instance id: %d\n", SDL_JoystickInstanceID( joy ) );
							printf( "        guid: %s\n", guid);
						}
					}
#endif
					// Start haptic from opened joystick
					SDL2_Init_Haptic_From_Joystick();
					break;

				case SDL_CONTROLLERDEVICEREMOVED:
					// WARNING: which is the joystick device index for the SDL_CONTROLLERDEVICEADDED event 
					// but it is the instance id for the SDL_CONTROLLERDEVICEREMOVED 
					// or SDL_CONTROLLERDEVICEREMAPPED event.
					printf("SDL_CONTROLLERDEVICEREMOVED cdevice.which %02i [INSTANCE ID]\n", ev.cdevice.which );
#ifdef __SDL2_ENABLE_CONTROLLER_HOTPLUG
					if( gamepad && instanceID == ev.cdevice.which) {
						// If a gamepad is in use opened check if the user unplugged it
						printf(" Gamepad %02i in use was unplugged. Closing it\n", device_index_in_use );
						SDL_GameControllerClose( gamepad );
						gamepad = NULL;
						// NOTE: what happens when a gamepad is closed? Is the joystick associated
						// with this gamepad still valid?
						// ANSWER: SDL_GameControllerClose() calls SDL_JoystickClose() internally
						joy = NULL;
						instanceID = -1;
						device_index_in_use = -1;
					} else {
						// If not in use do nothing
						printf( " Unplugged gamepad %02i not in use\n. Doing nothing\n", ev.cdevice.which );
					}
#endif
					break;
					
				case SDL_CONTROLLERDEVICEREMAPPED:
					printf("SDL_CONTROLLERDEVICEREMAPPED \n");
					printf("SDL event SDL_CONTROLLERDEVICEREMAPPED not implemented\n");
					break;
					
				case SDL_KEYDOWN:
					printf("SDL_KEYDOWN: SDL_QUIT\n");
					if (ev.key.keysym.sym == SDLK_ESCAPE)
						run_loop = 0;
					break;
					
				case SDL_QUIT:
					printf( "SDL_QUIT\n" );
					run_loop = 0;
					break;
				
				default:
					printf( "Sys_GetEvent: unknown SDL event %u\n", ev.type );
					break;
			}
		}
		
		fflush(stdout);
	}

	// 
	// Close haptics
	//
	if( haptic ) {
		SDL_HapticRumbleStop(haptic);
		SDL_HapticClose( haptic );
		haptic = NULL;
	}

	//
	// Close joystick
	//
	if( gamepad ) {
		printf( "Sys_ShutdownInput: closing SDL gamepad.\n" );
		SDL_GameControllerClose( gamepad );
		gamepad = NULL;
		joy = NULL;
		instanceID = -1;
		device_index_in_use = -1;
	} else if( joy ) {
		printf( "Sys_ShutdownInput: closing SDL joystick.\n" );
		SDL_JoystickClose( joy );
		gamepad = NULL;
		joy = NULL;
		instanceID = -1;
		device_index_in_use = -1;
	} else {
		printf( "Sys_ShutdownInput: SDL joystick not initialized. Nothing to close.\n" );
	}
	
	//
	// Shutdown SDL2
	//
	SDL_QuitSubSystem( SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC );
	
	return 0;
}
