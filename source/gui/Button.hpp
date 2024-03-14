#pragma once

namespace pd
{
	class Texterer;
	class Recterer;

	class Button
	{
	public:
		Button ();
		Button ( Recterer & recterer, Texterer & texterer );

		Button & SetText ( std::string const & );
		Button & SetPosition ( glm::vec2 const & position );
		Button & SetCallback ( std::function < void () > callback );

		std::string const & GetText () const;
		glm::vec2 const & GetPosition () const;

		void HandleEvent ( SDL_Event const & );

	private:
		static glm::vec2 const textPadding;
		static glm::vec4 const activeBackgroundColor;
		static glm::vec4 const inactiveBackgroundColor;

		bool CheckContainsPoint ( glm::vec2 const & );
		
		Recterer * recterer;
		Texterer * texterer;

		int textId;
		int backgroundId;
		std::function <void ()> pressCallback;
		
		std::string text {};
		glm::vec2 position { 0.0f, 0.0f };
		glm::vec2 size { 0.0f, 0.0f };
		std::function <void ()> callback;
		bool active { false };
	};



	// Implementation
	inline std::string const & Button::GetText () const { return text; }
	inline glm::vec2 const & Button::GetPosition () const { return position; }
	inline Button & Button::SetCallback ( std::function < void () > callback ) { this->callback = callback; return *this; }
}