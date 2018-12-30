#pragma once

struct point {
	double x;
	double y;
	double z;
};

struct vec {
	double x;
	double y;
	double z;
};

point operator+ (point a, vec b) {
	point ret;
	ret.x = a.x + b.x;
	ret.y = a.y + b.y;
	ret.z = a.z + b.z;
	return ret;
}

point operator- (point a, vec b) {
	point ret;
	ret.x = a.x - b.x;
	ret.y = a.y - b.y;
	ret.z = a.z - b.z;
	return ret;
}

vec operator+ (vec a, vec b) {
	vec ret;
	ret.x = a.x + b.x;
	ret.y = a.y + b.y;
	ret.z = a.z + b.z;
	return ret;
}

vec operator- (vec a, vec b) {
	vec ret;
	ret.x = a.x - b.x;
	ret.y = a.y - b.y;
	ret.z = a.z - b.z;
	return ret;
}

vec operator* (double s, vec a) {
	vec ret;
	ret.x = a.x * s;
	ret.y = a.y * s;
	ret.z = a.z * s;
	return ret;
}

vec normalize(vec p) {
	vec ret;
	double sum = p.x * p.x + p.y * p.y + p.z * p.z;
	sum = sqrt(sum);
	ret.x = p.x / sum;
	ret.y = p.y / sum;
	ret.z = p.z / sum;
	return ret;
}

double dot(vec a, vec b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

double dotNorm(vec a, vec b) {
	a = normalize(a);
	b = normalize(b);
	return dot (a, b);
}

vec cross(vec a, vec b) {
	vec ret;
	ret.x = a.y * b.z - a.z * b.y;
	ret.y = a.z * b.x - a.x * b.z;
	ret.z = a.x * b.y - a.y * b.x;
	return ret;
}

bool IsZero(vec p) {
	return (p.x == 0.0 && p.y == 0.0 && p.z == 0.0);
}

vec getForward(double u, point p0, point p1, point p2, point p3) {
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

double lerp(double a, double b, double r) {
	return a * (1 - r) + b * r;
}

double clamp(double a, double min, double max) {
	return (a < min) ? min : (a > max) ? max : a;
}

double size(vec a) {
	return sqrt(dot(a, a));
}