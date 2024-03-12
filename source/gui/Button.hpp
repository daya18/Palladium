#pragma once

namespace pd
{
	class GUIManager;

	class Button
	{
	public:
		Button ( GUIManager & );

		Button & SetText ( std::string const & );
		Button & SetPosition ( glm::vec2 const & position );

		std::string const & GetText () const;
		glm::vec2 const & GetPosition () const;

	private:
		static glm::vec2 const textPadding;

		GUIManager * guiManager;

		int textId;
		int backgroundId;
		std::function <void ()> pressCallback;
		
		std::string text {};
		glm::vec2 position { 0.0f, 0.0f };
	};



	// Implementation
	inline std::string const & Button::GetText () const { return text; }
	inline glm::vec2 const & Button::GetPosition () const { return position; }
}