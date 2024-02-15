#pragma once

#include "IDManager.hpp"

namespace pd
{
	class Recterer;

	class GUIManager
	{
	public:
		struct Dependencies
		{
			Recterer * recterer;
		};

		GUIManager ( Dependencies const & );
		
		void HandleEvent ( SDL_Event const & );

		int CreateButton ();

	private:
		Dependencies dependencies;

		IDManager idManager;
	};
}