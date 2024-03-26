#include "BetterType.hpp"

namespace pd::bt
{
	Library::Library ()
	{
		FT_Init_FreeType ( &library );
	}

	Library::~Library ()
	{
		FT_Done_FreeType ( library );
	}

	Face::Face ( Library & library, std::string const & path )
		: library ( library )
	{
		FT_New_Face ( library.library, path.data (), 0, &face );
	}

	Face::~Face ()
	{
		FT_Done_Face ( face );
	}

	Text::Text ( Face & face, std::string const & text, int height, int linePadding )
		: face ( face )
	{
		if ( linePadding == 0 )
			linePadding = height / 2;

		std::vector < std::vector < FT_Glyph_Metrics > > lineGlyphMetrics { {} };
		std::vector < std::vector < FT_Bitmap > > lineGlyphBitmaps { {} };

		// Get glyph bitmap and metrics
		for ( char ch : text )
		{
			if ( ch == '\n' )
			{
				lineGlyphMetrics.push_back ( {} );
				lineGlyphBitmaps.push_back ( {} );
				continue;
			}

			FT_Set_Pixel_Sizes ( face.face, 0, height );
			FT_Load_Glyph ( face.face, FT_Get_Char_Index ( face.face, ch ), 0 );

			if ( face.face->glyph->format != FT_GLYPH_FORMAT_BITMAP )
				FT_Render_Glyph ( face.face->glyph, FT_RENDER_MODE_NORMAL );

			lineGlyphMetrics.back().push_back (face.face->glyph->metrics);

			lineGlyphBitmaps.back ().push_back ( {} );
			FT_Bitmap_Copy ( face.library.library, &face.face->glyph->bitmap, &lineGlyphBitmaps.back ().back() );
		}

		std::vector <float> lineMaxAscents;
		std::vector <float> lineMaxDescents;

		for ( auto const & glyphMetrics : lineGlyphMetrics )
		{
			lineMaxAscents.push_back ( { - std::numeric_limits <float>::infinity () } );
			lineMaxDescents.push_back ( { -std::numeric_limits <float>::infinity () } );

			for ( auto const & metrics : glyphMetrics )
			{
				lineMaxAscents.back () = glm::max ( lineMaxAscents.back (), static_cast < float > ( metrics.horiBearingY / 64 ) );
				lineMaxDescents.back () = glm::max <float> ( lineMaxDescents.back (), ( metrics.height - metrics.horiBearingY ) / 64 );
			}
		}
		
		int lineCount { static_cast <int> ( std::count ( text.begin (), text.end (), '\n' ) ) + 1 };
		this->size = { 0.0f, 0.0f };
		glm::vec2 penPosition { 0.0f, lineMaxAscents.front () };


		// Calculate glyph positions
		auto lineAdvance {
			( face.face->ascender - face.face->descender ) / 64 * 0.5 + linePadding
		};

		int lineIndex { 0 };
		for ( auto const & glyphMetrics : lineGlyphMetrics )
		{
			float lineWidth { 0.0f };
			int glyphIndex { 0 };
			for ( auto const & metrics : glyphMetrics )
			{
				glm::vec2 position { penPosition + glm::vec2 {
					static_cast < float > ( metrics.horiBearingX / 64 ),
					-static_cast < float > ( metrics.horiBearingY / 64 )
				} };

				glm::vec2 size {
					static_cast < float > ( metrics.width / 64 ),
					static_cast < float > ( metrics.height / 64 )
				};

				glyphs.push_back ( { lineGlyphBitmaps [lineIndex] [ glyphIndex ], position, size } );

				penPosition.x += metrics.horiAdvance / 64;
				lineWidth += static_cast < float > ( metrics.horiAdvance / 64 );

				++glyphIndex;
			}

			this->size.x = glm::max ( this->size.x, lineWidth );
			penPosition.x = 0.0f;
			penPosition.y += lineAdvance;
			
			++lineIndex;
		}

		this->size.y = penPosition.y - lineAdvance + lineMaxDescents.back ();
	}

	Text::~Text ()
	{
		for ( auto & glyph : glyphs )
			FT_Bitmap_Done ( face.library.library, &glyph.bitmap );
	}
}