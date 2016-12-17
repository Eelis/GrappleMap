#ifndef GRAPPLEMAP_PLAYERDRAWER_HPP
#define GRAPPLEMAP_PLAYERDRAWER_HPP

#include "positions.hpp"
#include "icosphere.hpp"

namespace GrappleMap
{
	class PlayerDrawer
	{
		static constexpr unsigned faces = 10;

		std::array<std::pair<double, double>, faces + 1> angles; // pillar
		icosphere::IndexedMesh const fine_icomesh, course_icomesh;

		void drawPillar(V3 from, V3 to, double from_radius, double to_radius) const;
		void drawSphere(V3 center, double radius, bool fine = true) const;
		void drawLimbs(Position const &, optional<PlayerNum> first_person_player) const;
		void drawJoints(Position const &,
			PerPlayerJoint<optional<V3>> const & colors,
			optional<PlayerNum> first_person_player) const;

	public:

		PlayerDrawer();

		void drawPlayers(Position const &,
			PerPlayerJoint<optional<V3>> const & colors,
			optional<PlayerNum> first_person_player) const;
	};
}

#endif
