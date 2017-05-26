#ifndef GRAPPLEMAP_PLAYERDRAWER_HPP
#define GRAPPLEMAP_PLAYERDRAWER_HPP

#include "positions.hpp"
#include "icosphere.hpp"

namespace GrappleMap
{
	struct BasicVertex
	{
		V3f pos, norm;
		V4f color;
	};

	class SphereDrawer
	{
		icosphere::IndexedMesh const fine_icomesh, course_icomesh;

		public:

			SphereDrawer();

			template<typename F>
			void draw(V3 center, double radius, bool fine, F out) const;
	};

	void drawSphere(SphereDrawer const &, V3 center, double radius, bool fine = true);
	void drawSphere(SphereDrawer const &, V4f color, V3 center, double radius, bool fine, std::vector<BasicVertex> & out);

	class PlayerDrawer
	{
		static constexpr unsigned faces = 10;

		std::array<std::pair<double, double>, faces + 1> angles; // pillar

		template<typename F>
		void drawPillar(V3 from, V3 to, double from_radius, double to_radius, F handle_vertex) const;

		void drawPillar(V3 from, V3 to, double from_radius, double to_radius) const;
		void drawLimbs(Position const &, optional<PlayerNum> first_person_player) const;
		void drawPillar(V3 color, V3 from, V3 to, double from_radius, double to_radius, std::vector<BasicVertex> & out) const;
		void drawLimbs(Position const &, optional<PlayerNum> first_person_player, std::vector<BasicVertex> & out) const;
		void drawJoints(Position const &,
			PerPlayerJoint<optional<V3>> const & colors,
			optional<PlayerNum> first_person_player) const;
		void drawJoints(Position const &,
			PerPlayerJoint<optional<V3>> const & colors,
			optional<PlayerNum> first_person_player,
			std::vector<BasicVertex> & out) const;

	public:

		SphereDrawer sphereDrawer;

		PlayerDrawer();

		void drawPlayers(Position const &,
			PerPlayerJoint<optional<V3>> const & colors,
			optional<PlayerNum> first_person_player) const;

		void drawPlayers(Position const &,
			PerPlayerJoint<optional<V3>> const & colors,
			optional<PlayerNum> first_person_player,
			std::vector<BasicVertex> & out) const;
	};
}

#endif
