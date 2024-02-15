#include "IDManager.hpp"

#include <numeric>

namespace pd
{
	IDManager::IDManager ( int begin, int end )
	{
		auto count { end - begin + 1 };

		std::vector <int> ids ( count );
		std::iota ( ids.begin (), ids.end (), begin );

		idInfos.reserve ( count );

		for ( auto const id : ids )
			idInfos.push_back ( { id, false } );
	}

	int IDManager::GetID ()
	{
		for ( auto & idInfo : idInfos )
		{
			if ( ! idInfo.used )
			{
				idInfo.used = true;
				return idInfo.id;
			}
		}

		throw std::runtime_error { "No id available" };
	}

	void IDManager::FreeID ( int id )
	{
		for ( auto & idInfo : idInfos )
		{
			if ( idInfo.id == id )
			{
				idInfo.used = false;
				return;
			}
		}

		throw std::runtime_error { "Couldn't find id" };
	}
}