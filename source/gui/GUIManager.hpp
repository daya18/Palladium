#pragma once

#include "Button.hpp"

namespace pd
{
	class Recterer;
	class Texterer;

	class GUIManager
	{
	public:
		void Initialize ( Recterer * recterer, Texterer * texterer );
		
		void HandleEvent ( SDL_Event const & );

		Button & CreateButton ();
		
		Recterer & GetRecterer ();
		Texterer & GetTexterer ();

	private:
		Recterer * recterer;
		Texterer * texterer;

		std::vector <Button> buttons;
	};



	// Implementation
	inline Recterer & GUIManager::GetRecterer () { return *recterer; }
	inline Texterer & GUIManager::GetTexterer () { return *texterer; }
}