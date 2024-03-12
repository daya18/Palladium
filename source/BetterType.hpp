#pragma once

namespace pd::bt
{
	class Library
	{
	public:
		Library ();
		~Library ();

	private:
		FT_Library library;

		friend class Face;
		friend class Text;
	};

	class Face
	{
	public:
		Face ( Library &, std::string const & path );
		~Face ();

	private:
		Library & library;
		FT_Face face;
		
		friend class Text;
	};

	class Text
	{
	public:
		struct Glyph
		{
			FT_Bitmap bitmap;
			glm::vec2 position;
			glm::vec2 size;
		};

		Text ( Face &, std::string const & text, int height );
		~Text ();

		std::vector <Glyph> const & GetGlyphs () const;
		glm::vec2 const & GetSize () const;

	private:
		Face & face;
		glm::vec2 size;
		std::vector <Glyph> glyphs;
	};


	// Implementation
	inline glm::vec2 const & Text::GetSize () const { return size; }
	inline std::vector <Text::Glyph> const & Text::GetGlyphs () const { return glyphs; }
}