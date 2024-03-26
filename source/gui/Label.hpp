#pragma once

namespace pd
{
	class Texterer;
	class Recterer;

	class Label
	{
	public:
		Label ();
		Label ( Recterer & recterer, Texterer & texterer );

		Label & SetText ( std::string const & );
		Label & SetPosition ( glm::vec2 const & position );

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
		
		std::string text {};
		glm::vec2 position { 0.0f, 0.0f };
		glm::vec2 size { 0.0f, 0.0f };
		std::function <void ()> callback;
		bool active { false };
	};



	// Implementation
	inline std::string const & Label::GetText () const { return text; }
	inline glm::vec2 const & Label::GetPosition () const { return position; }
}