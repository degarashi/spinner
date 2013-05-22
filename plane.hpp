#include "vector.hpp"
#include <tuple>

namespace spn {
	struct Plane {
		float	a,b,c,d;

		Plane();
		Plane(const Plane& p) = default;
		Plane(float fa, float fb, float fc, float fd);
		Plane(const Vec3& orig, float dist);

		static Plane FromPtDir(const Vec3& pos, const Vec3& dir);
		static Plane FromPts(const Vec3& p0, const Vec3& p1, const Vec3& p2);
		static std::tuple<Vec3,bool> ChokePoint(const Plane& p0, const Plane& p1, const Plane& p2);
		static std::tuple<Vec3,Vec3,bool> CrossLine(const Plane& p0, const Plane& p1);

		float dot(const Vec3& p) const;
		void move(float d);
		const Vec3& getNormal() const;
	};
}
