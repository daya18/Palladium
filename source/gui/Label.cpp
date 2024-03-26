#include "Label.hpp"

#include "../Texterer.hpp"
#include "../Recterer.hpp"

namespace pd
{
	glm::vec2 const Label::textPadding { 40, 40 };
	glm::vec4 const Label::activeBackgroundColor { 0.8f, 0.8f, 0.8f, 1.0f };
	glm::vec4 const Label::inactiveBackgroundColor { 1.0f, 1.0f, 1.0f, 1.0f };

	Label::Label () {}

	Label::Label ( Recterer & recterer, Texterer & texterer )
	: 
		recterer ( & recterer ),
		texterer ( & texterer ),
		textId ( texterer.CreateText () ),
		backgroundId ( recterer.CreateRectangle () )
	{
		texterer.SetTextColor ( textId, { 0.0f, 0.0f, 0.0f, 1.0f } );
		texterer.SetTextHeight ( textId, 20 );
		recterer.SetRectangleBorderColor ( backgroundId, { 1.0f, 1.0f, 1.0f, 1.0f } );
	}

	void Label::HandleEvent ( SDL_Event const & event )
	{
	}

	Label & Label::SetText ( std::string const & text )
	{
		this->text = text;

		texterer->SetText ( textId, text );
		texterer->SetTextPosition ( textId, position + textPadding * 0.5f );
		glm::vec2 textSize { texterer->GetTextSize ( textId ) };

		recterer->SetRectangleTransform ( backgroundId,
			CreateTransformMatrix ( { position, -1 }, { textSize + textPadding, 1.0f } ) );
		
		this->size = textSize + textPadding;

		return *this;
	}
	
	Label & Label::SetPosition ( glm::vec2 const & position )
	{
		this->position = position;
		
		texterer->SetTextPosition (textId, position + textPadding * 0.5f);
		glm::vec2 textSize { texterer->GetTextSize ( textId ) };

		recterer->SetRectangleTransform ( backgroundId,
			CreateTransformMatrix ( { position, -1 }, { textSize + textPadding, 1.0f } ) );

		this->size = textSize + textPadding;

		return *this;
	}

	bool Label::CheckContainsPoint ( glm::vec2 const & point )
	{
		if ( point.x < position.x || point.x > position.x + size.x )
			return false;

		if ( point.y < position.y || point.y > position.y + size.y ) 
			return false;

		return true;
	}
}