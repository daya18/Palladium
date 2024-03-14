#include "Button.hpp"

#include "../Texterer.hpp"
#include "../Recterer.hpp"

namespace pd
{
	glm::vec2 const Button::textPadding { 40, 40 };
	glm::vec4 const Button::activeBackgroundColor { 0.8f, 0.8f, 0.8f, 1.0f };
	glm::vec4 const Button::inactiveBackgroundColor { 1.0f, 1.0f, 1.0f, 1.0f };

	Button::Button () {}

	Button::Button ( Recterer & recterer, Texterer & texterer )
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

	void Button::HandleEvent ( SDL_Event const & event )
	{
		bool containsPoint { false };

		switch ( event.type )
		{
		case SDL_MOUSEMOTION:
			containsPoint = CheckContainsPoint ( { event.motion.x, event.motion.y } );
			
			if ( ! active && containsPoint )
			{
				active = true;
				recterer->SetRectangleColor ( backgroundId, activeBackgroundColor );
			}
			else if ( active && !containsPoint )
			{
				active = false;	
				recterer->SetRectangleColor ( backgroundId, inactiveBackgroundColor );
			}
			
			break;

		case SDL_MOUSEBUTTONDOWN:
			if ( active )
				recterer->SetRectangleColor ( backgroundId, inactiveBackgroundColor );
			break;

		case SDL_MOUSEBUTTONUP:
			if ( active )
			{
				recterer->SetRectangleColor ( backgroundId, activeBackgroundColor );
				if ( callback ) callback ();
			}
			break;
		}
	}

	Button & Button::SetText ( std::string const & text )
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
	
	Button & Button::SetPosition ( glm::vec2 const & position )
	{
		this->position = position;
		
		texterer->SetTextPosition (textId, position + textPadding * 0.5f);
		glm::vec2 textSize { texterer->GetTextSize ( textId ) };

		recterer->SetRectangleTransform ( backgroundId,
			CreateTransformMatrix ( { position, -1 }, { textSize + textPadding, 1.0f } ) );

		this->size = textSize + textPadding;

		return *this;
	}

	bool Button::CheckContainsPoint ( glm::vec2 const & point )
	{
		if ( point.x < position.x || point.x > position.x + size.x )
			return false;

		if ( point.y < position.y || point.y > position.y + size.y ) 
			return false;

		return true;
	}
}