#include "GUIManager.hpp"

#include "../Recterer.hpp"
#include "../Texterer.hpp"

namespace pd
{
	void GUIManager::Initialize ( Recterer * recterer, Texterer * texterer )
	{
		this->recterer = recterer;
		this->texterer = texterer;
	}
	
	void GUIManager::HandleEvent ( SDL_Event const & event )
	{

	}

	Button & GUIManager::CreateButton ()
	{
		return buttons.emplace_back ( Button { *this } );
	}
}