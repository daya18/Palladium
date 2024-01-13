#include <iostream>

#include <SDL2/SDL.h>

int main ( int argc, char * args [] )
{
	SDL_Init ( 0 );
	
	auto window { SDL_CreateWindow ( "Palladium", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, 0 ) };

	bool quit { false };

	while ( ! quit )
	{
		SDL_Event event;

		while ( SDL_PollEvent ( &event ) )
		{
			switch ( event.type )
			{
			case SDL_WINDOWEVENT:
				switch ( event.window.event )
				{
				case SDL_WINDOWEVENT_CLOSE: 
					quit = true;
					break;
				}
				break;
			}
		}
	}
	
	SDL_DestroyWindow ( window );
	SDL_Quit ();

	return 0;
}
