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

	Text::Text ( Face & face, std::string const & text, int height )
		: face ( face )
	{
		std::vector <FT_Glyph_Metrics> glyphMetrics;
		std::vector <FT_Bitmap> glyphBitmaps;

		// Get glyph bitmap and metrics
		for ( char ch : text )
		{
			FT_Set_Pixel_Sizes ( face.face, 0, height );
			FT_Load_Glyph ( face.face, FT_Get_Char_Index ( face.face, ch ), 0 );

			if ( face.face->glyph->format != FT_GLYPH_FORMAT_BITMAP )
				FT_Render_Glyph ( face.face->glyph, FT_RENDER_MODE_NORMAL );

			glyphMetrics.push_back ( face.face->glyph->metrics );

			glyphBitmaps.push_back ( {} );
			FT_Bitmap_Copy ( face.library.library, &face.face->glyph->bitmap, &glyphBitmaps.back () );
		}

		float maxAscent { -std::numeric_limits <float>::infinity () };
		float maxDescent { -std::numeric_limits <float>::infinity () };

		for ( auto const & metrics : glyphMetrics )
		{
			maxAscent = glm::max ( maxAscent, static_cast < float > ( metrics.horiBearingY / 64 ) );
			maxDescent = glm::max <float> ( maxDescent, ( metrics.height - metrics.horiBearingY ) / 64 );
		}

		this->size = { 0.0f, glm::abs ( maxAscent ) + glm::abs ( maxDescent ) };

		glm::vec2 penPosition { 0.0f, maxAscent };

		int glyphIndex { 0 };
		// Calculate glyph positions
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

			glyphs.push_back ( { glyphBitmaps [ glyphIndex ], position, size } );

			penPosition.x += metrics.horiAdvance / 64;
			this->size.x += static_cast < float > ( metrics.horiAdvance / 64 );

			++glyphIndex;
		}
	}

	Text::~Text ()
	{
		for ( auto & glyph : glyphs )
			FT_Bitmap_Done ( face.library.library, &glyph.bitmap );
	}
}