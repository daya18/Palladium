#pragma once

namespace pd
{
	class IDManager
	{
	public:
		IDManager ( int begin = 0, int end = 100 );

		int GetID ();
		void FreeID ( int );

	private:
		struct IDInfo
		{
			int id;
			bool used { false };
		};

		std::vector <IDInfo> idInfos;
	};
}