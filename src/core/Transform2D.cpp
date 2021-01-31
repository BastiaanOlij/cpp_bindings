#include <Transform2D.hpp>

namespace godot {

void Transform2D::invert() {
	// FIXME: this function assumes the basis is a rotation matrix, with no scaling.
	// Transform2D::affine_inverse can handle matrices with scaling, so GDScript should eventually use that.
	SWAP(elements[0][1], elements[1][0]);
	elements[2] = basis_xform(-elements[2]);
}

Transform2D Transform2D::inverse() const {
	Transform2D inv = *this;
	inv.invert();
	return inv;
}

void Transform2D::affine_invert() {
	real_t det = basis_determinant();
#ifdef MATH_CHECKS
	ERR_FAIL_COND(det == 0);
#endif
	real_t idet = 1.0 / det;

	SWAP(elements[0][0], elements[1][1]);
	elements[0] *= Vector2(idet, -idet);
	elements[1] *= Vector2(-idet, idet);

	elements[2] = basis_xform(-elements[2]);
}

Transform2D Transform2D::affine_inverse() const {
	Transform2D inv = *this;
	inv.affine_invert();
	return inv;
}

void Transform2D::rotate(real_t p_phi) {
	*this = Transform2D(p_phi, Vector2()) * (*this);
}

real_t Transform2D::get_skew() const {
	real_t det = basis_determinant();
	return Math::acos(elements[0].normalized().dot(Math::sign(det) * elements[1].normalized())) - Math_PI * 0.5;
}

void Transform2D::set_skew(float p_angle) {
	real_t det = basis_determinant();
	elements[1] = Math::sign(det) * elements[0].rotated((Math_PI * 0.5 + p_angle)).normalized() * elements[1].length();
}

real_t Transform2D::get_rotation() const {
	return Math::atan2(elements[0].y, elements[0].x);
}

void Transform2D::set_rotation(real_t p_rot) {
	Size2 scale = get_scale();
	real_t cr = Math::cos(p_rot);
	real_t sr = Math::sin(p_rot);
	elements[0][0] = cr;
	elements[0][1] = sr;
	elements[1][0] = -sr;
	elements[1][1] = cr;
	set_scale(scale);
}

Transform2D::Transform2D(real_t p_rot, const Vector2 &p_pos) {
	real_t cr = Math::cos(p_rot);
	real_t sr = Math::sin(p_rot);
	elements[0][0] = cr;
	elements[0][1] = sr;
	elements[1][0] = -sr;
	elements[1][1] = cr;
	elements[2] = p_pos;
}

Size2 Transform2D::get_scale() const {
	real_t det_sign = Math::sign(basis_determinant());
	return Size2(elements[0].length(), det_sign * elements[1].length());
}

void Transform2D::set_scale(const Size2 &p_scale) {
	elements[0].normalize();
	elements[1].normalize();
	elements[0] *= p_scale.x;
	elements[1] *= p_scale.y;
}

void Transform2D::scale(const Size2 &p_scale) {
	scale_basis(p_scale);
	elements[2] *= p_scale;
}

void Transform2D::scale_basis(const Size2 &p_scale) {
	elements[0][0] *= p_scale.x;
	elements[0][1] *= p_scale.y;
	elements[1][0] *= p_scale.x;
	elements[1][1] *= p_scale.y;
}

void Transform2D::translate(real_t p_tx, real_t p_ty) {
	translate(Vector2(p_tx, p_ty));
}

void Transform2D::translate(const Vector2 &p_translation) {
	elements[2] += basis_xform(p_translation);
}

void Transform2D::orthonormalize() {
	// Gram-Schmidt Process

	Vector2 x = elements[0];
	Vector2 y = elements[1];

	x.normalize();
	y = (y - x * (x.dot(y)));
	y.normalize();

	elements[0] = x;
	elements[1] = y;
}

Transform2D Transform2D::orthonormalized() const {
	Transform2D on = *this;
	on.orthonormalize();
	return on;
}

bool Transform2D::is_equal_approx(const Transform2D &p_transform) const {
	return elements[0].is_equal_approx(p_transform.elements[0]) && elements[1].is_equal_approx(p_transform.elements[1]) && elements[2].is_equal_approx(p_transform.elements[2]);
}

bool Transform2D::operator==(const Transform2D &p_transform) const {
	for (int i = 0; i < 3; i++) {
		if (elements[i] != p_transform.elements[i]) {
			return false;
		}
	}

	return true;
}

bool Transform2D::operator!=(const Transform2D &p_transform) const {
	for (int i = 0; i < 3; i++) {
		if (elements[i] != p_transform.elements[i]) {
			return true;
		}
	}

	return false;
}

void Transform2D::operator*=(const Transform2D &p_transform) {
	elements[2] = xform(p_transform.elements[2]);

	real_t x0, x1, y0, y1;

	x0 = tdotx(p_transform.elements[0]);
	x1 = tdoty(p_transform.elements[0]);
	y0 = tdotx(p_transform.elements[1]);
	y1 = tdoty(p_transform.elements[1]);

	elements[0][0] = x0;
	elements[0][1] = x1;
	elements[1][0] = y0;
	elements[1][1] = y1;
}

Transform2D Transform2D::operator*(const Transform2D &p_transform) const {
	Transform2D t = *this;
	t *= p_transform;
	return t;
}

Transform2D Transform2D::scaled(const Size2 &p_scale) const {
	Transform2D copy = *this;
	copy.scale(p_scale);
	return copy;
}

Transform2D Transform2D::basis_scaled(const Size2 &p_scale) const {
	Transform2D copy = *this;
	copy.scale_basis(p_scale);
	return copy;
}

Transform2D Transform2D::untranslated() const {
	Transform2D copy = *this;
	copy.elements[2] = Vector2();
	return copy;
}

Transform2D Transform2D::translated(const Vector2 &p_offset) const {
	Transform2D copy = *this;
	copy.translate(p_offset);
	return copy;
}

Transform2D Transform2D::rotated(real_t p_phi) const {
	Transform2D copy = *this;
	copy.rotate(p_phi);
	return copy;
}

real_t Transform2D::basis_determinant() const {
	return elements[0].x * elements[1].y - elements[0].y * elements[1].x;
}

Transform2D Transform2D::interpolate_with(const Transform2D &p_transform, real_t p_c) const {
	//extract parameters
	Vector2 p1 = get_origin();
	Vector2 p2 = p_transform.get_origin();

	real_t r1 = get_rotation();
	real_t r2 = p_transform.get_rotation();

	Size2 s1 = get_scale();
	Size2 s2 = p_transform.get_scale();

	//slerp rotation
	Vector2 v1(Math::cos(r1), Math::sin(r1));
	Vector2 v2(Math::cos(r2), Math::sin(r2));

	real_t dot = v1.dot(v2);

	dot = Math::clamp(dot, (real_t)-1.0, (real_t)1.0);

	Vector2 v;

	if (dot > 0.9995) {
		v = v1.lerp(v2, p_c).normalized(); //linearly interpolate to avoid numerical precision issues
	} else {
		real_t angle = p_c * Math::acos(dot);
		Vector2 v3 = (v2 - v1 * dot).normalized();
		v = v1 * Math::cos(angle) + v3 * Math::sin(angle);
	}

	//construct matrix
	Transform2D res(Math::atan2(v.y, v.x), p1.lerp(p2, p_c));
	res.scale_basis(s1.lerp(s2, p_c));
	return res;
}

Transform2D::operator String() const {
	return elements[0].operator String() + ", " + elements[1].operator String() + ", " + elements[2].operator String();
}

} // namespace godot
