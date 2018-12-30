#pragma once

#define EPSILON 0.0000001

struct point {
	double x;
	double y;
	double z;
	
	point() : x(0), y(0), z(0) {}

	point(double a, double b, double c) {
		x = a;
		y = b;
		z = c;
	}

	point(const double arr[3]) {
		x = arr[0];
		y = arr[1];
		z = arr[2];
	}
};

struct vec {
	double x;
	double y;
	double z;

	vec() : x(0), y(0), z(0) {}

	vec(double a, double b, double c) {
		x = a;
		y = b;
		z = c;
	}

	vec(const double arr[3]) {
		x = arr[0];
		y = arr[1];
		z = arr[2];
	}

	double Length() {
		return sqrt(x * x + y * y + z * z);
	}

	double LengthSq() {
		return x * x + y * y + z * z;
	}
	
	vec Normalize() {
		double len = Length();
		x = x / len;
		y = y / len;
		z = z / len;
		return *this;
	}

	bool IsZero() {
		return (x == 0.0 && y == 0.0 && z == 0.0);
	}

	vec operator+= (vec v) {
		x = x + v.x;
		y = y + v.y;
		z = z + v.z;
		return *this;
	}

	vec operator*= (double s) {
		x = x * s;
		y = y * s;
		z = z * s;
		return *this;
	}

	vec operator/= (double s) {
		x = x / s;
		y = y / s;
		z = z / s;
		return *this;
	}
};

point operator+ (point a, vec b) {
	return point(a.x + b.x, a.y + b.y, a.z + b.z);
}

point operator- (point a, vec b) {
	return point(a.x - b.x, a.y - b.y, a.z - b.z);
}

bool operator== (point a, point b) {
	return a.x == b.x && a.y == b.y && a.z == b.z;
}

bool operator!= (point a, point b) {
	return !(a == b);
}

vec operator- (point a, point b) {
	return vec(a.x - b.x, a.y - b.y, a.z - b.z);
}

vec operator+ (vec a, vec b) {
	return vec(a.x + b.x, a.y + b.y, a.z + b.z);
}

vec operator- (vec a, vec b) {
	return vec(a.x - b.x, a.y - b.y, a.z - b.z);
}

vec operator* (double s, vec a) {
	return vec(a.x * s, a.y * s, a.z * s);
}

vec operator* (vec a, double s) {
	return s * a;
}

vec operator/ (vec a, double s) {
	return vec(a.x / s, a.y / s, a.z / s);
}

vec normalize(vec p) {
	p.Normalize();
	return p;
}

double dot(const vec& a, const vec& b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}


vec cross(const vec& a, const vec& b) {
	vec ret;
	ret.x = a.y * b.z - a.z * b.y;
	ret.y = a.z * b.x - a.x * b.z;
	ret.z = a.x * b.y - a.y * b.x;
	return ret;
}

vec getForward(const double& u, const point& p0, const point& p1, const point& p2, const point& p3) {
	vec p;
	if (u < 0.0f || u > 1.0f) {
		throw 1;
	}

	double u2 = u * u;
	p.x = 0.5f * ((-p0.x + 3 * p1.x - 3 * p2.x + p3.x) * 3 * u2 + (2 * p0.x - 5 * p1.x + 4 * p2.x - p3.x) * 2 * u + (-p0.x + p2.x));
	p.y = 0.5f * ((-p0.y + 3 * p1.y - 3 * p2.y + p3.y) * 3 * u2 + (2 * p0.y - 5 * p1.y + 4 * p2.y - p3.y) * 2 * u + (-p0.y + p2.y));
	p.z = 0.5f * ((-p0.z + 3 * p1.z - 3 * p2.z + p3.z) * 3 * u2 + (2 * p0.z - 5 * p1.z + 4 * p2.z - p3.z) * 2 * u + (-p0.z + p2.z));
	return p;
}

point CatmullRom(double u, point p0, point p1, point p2, point p3) {
	point p;
	if (u < 0.0f || u > 1.0f) {
		throw 1;
	}

	double u2 = u * u;
	double u3 = u * u * u;

	p.x = 0.5f * ((-p0.x + 3 * p1.x - 3 * p2.x + p3.x) * u3 + (2 * p0.x - 5 * p1.x + 4 * p2.x - p3.x) * u2 + (-p0.x + p2.x) * u + 2 * p1.x);
	p.y = 0.5f * ((-p0.y + 3 * p1.y - 3 * p2.y + p3.y) * u3 + (2 * p0.y - 5 * p1.y + 4 * p2.y - p3.y) * u2 + (-p0.y + p2.y) * u + 2 * p1.y);
	p.z = 0.5f * ((-p0.z + 3 * p1.z - 3 * p2.z + p3.z) * u3 + (2 * p0.z - 5 * p1.z + 4 * p2.z - p3.z) * u2 + (-p0.z + p2.z) * u + 2 * p1.z);

	return p;
}

double lerp(const double& a, const double& b, const double& r) {
	return a * (1 - r) + b * r;
}

double clamp(const double& a, const double& min, const double& max) {
	return (a < min) ? min : (a > max) ? max : a;
}

double dist(const point& a, const point& b) {
	return (a - b).Length();
}