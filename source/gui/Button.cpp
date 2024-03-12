#include "Button.hpp"

#include "GUIManager.hpp"
#include "../Texterer.hpp"
#include "../Recterer.hpp"

namespace pd
{
	glm::vec2 const Button::textPadding { 40, 40 };

	Button::Button ( GUIManager & guiManager )
	: 
		guiManager ( &guiManager ),
		textId ( guiManager.GetTexterer().CreateText () ),
		backgroundId ( guiManager.GetRecterer ().CreateRectangle () )
	{
		guiManager.GetTexterer ().SetTextColor ( textId, { 0.0f, 0.0f, 0.0f, 1.0f } );
		guiManager.GetTexterer ().SetTextHeight ( textId, 20 );
		guiManager.GetRecterer ().SetRectangleBorderColor ( backgroundId, { 1.0f, 1.0f, 1.0f, 1.0f } );
	}

	Button & Button::SetText ( std::string const & text )
	{
		this->text = text;

		guiManager->GetTexterer ().SetText ( textId, text );
		guiManager->GetTexterer ().SetTextPosition ( textId, position + textPadding * 0.5f );
		glm::vec2 textSize { guiManager->GetTexterer ().GetTextSize ( textId ) };

		guiManager->GetRecterer ().SetRectangleTransform ( backgroundId,
			CreateTransformMatrix ( { position, -1 }, { textSize + textPadding, 1.0f } ) );

		return *this;
	}
	
	Button & Button::SetPosition ( glm::vec2 const & position )
	{
		this->position = position;
		
		guiManager->GetTexterer ().SetTextPosition (textId, position + textPadding * 0.5f);
		glm::vec2 textSize { guiManager->GetTexterer ().GetTextSize ( textId ) };

		guiManager->GetRecterer ().SetRectangleTransform ( backgroundId,
			CreateTransformMatrix ( { position, -1 }, { textSize + textPadding, 1.0f } ) );

		return *this;
	}
}