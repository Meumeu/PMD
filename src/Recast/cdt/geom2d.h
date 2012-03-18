/*****************************************************************************
**
** geom2d.h: an include file containing class definitions and inline 
** functions related to 2D geometrical primitives: vectors/points and lines.
**
** Copyright (C) 1995 by Dani Lischinski 
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
******************************************************************************/

#ifndef GEOM2D_H
#define GEOM2D_H

#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include "common.h"
#include "../Recast.h"

#define EPS 1e-6

namespace Delaunay {

typedef float Real;

class Vector2d {
	Real x, y;
public:
	Vector2d()                          { x = 0; y = 0; }
	Vector2d(Real a, Real b)            { x = a; y = b; }
	Vector2d(const Vector2d &v)         { *this = v; }
	Vector2d(const Recast::FloatVertex & v): x(v.x), y(v.z) {}
	Real const& getX() const {return x;}
	Real const& getY() const {return y;}
	Real norm() const;
	Real normalize();
	bool operator==(const Vector2d&) const;
	Vector2d operator+(const Vector2d&) const;
	Vector2d operator-(const Vector2d&) const;
	Real     operator|(const Vector2d&) const;
	friend Vector2d operator*(Real, const Vector2d&);
	friend Vector2d operator/(const Vector2d&, Real);
	friend Real dot(const Vector2d&, const Vector2d&);
};

typedef Vector2d Point2d;

class Line {
public:
	Line(const Point2d&, const Point2d&);
	void set(const Point2d&, const Point2d&);
	Real eval(const Point2d&) const;
	int classify(const Point2d&) const;
	Point2d intersect(const Point2d&, const Point2d&) const;
private:
	Real a, b, c;
};

// Vector2d:

inline Real Vector2d::norm() const
{
	return sqrt(x * x + y * y);
}

inline Real Vector2d::normalize()
{
	Real len = norm();

	if (len == 0.0)
		throw std::range_error("Can't normalize a null vector");
	else {
		x /= len;
		y /= len;
	}
	return len;
}

inline Vector2d Vector2d::operator+(const Vector2d& v) const
{
	return Vector2d(x + v.x, y + v.y);
}

inline Vector2d Vector2d::operator-(const Vector2d& v) const
{
	return Vector2d(x - v.x, y - v.y);
}

inline bool Vector2d::operator==(const Vector2d& v) const
{
	return (*this - v).norm() <= EPS;
}

inline Real Vector2d::operator|(const Vector2d& v) const
// dot product (cannot overload the . operator)
{
	return x * v.x + y * v.y;
}

inline Vector2d operator*(Real c, const Vector2d& v)
{
	return Vector2d(c * v.x, c * v.y);
}

inline Vector2d operator/(const Vector2d& v, Real c)
{
	return Vector2d(v.x / c, v.y / c);
}

inline Real dot(const Vector2d& u, const Vector2d& v)
// another dot product
{
        return u.x * v.x + u.y * v.y;
}

// Line:

inline Line::Line(const Point2d& p, const Point2d& q)
// Computes the normalized line equation through the points p and q.
{
	Vector2d t = q - p;
	Real len = t.norm();

	a =   t.getY() / len;
	b = - t.getX() / len;
	// c = -(a*p.getX() + b*p.getY());

	// less efficient, but more robust -- seth.
	c = -0.5 * ((a*p.getX() + b*p.getY()) + (a*q.getX() + b*q.getY()));
}

inline void Line::set(const Point2d& p, const Point2d& q)
{
	*this = Line(p, q);
}

inline Real Line::eval(const Point2d& p) const
// Plugs point p into the line equation.
{
	return (a * p.getX() + b* p.getY() + c);
}

inline Point2d Line::intersect(const Point2d& p1, const Point2d& p2) const
// Returns the intersection of the line with the segment (p1,p2)
{
        // assumes that segment (p1,p2) crosses the line
        Vector2d d = p2 - p1;
        Real t = - eval(p1) / (a*d.getX() + b*d.getY());
        return (p1 + t*d);
}

inline bool operator==(const Point2d& point, const Line& line)
// Returns TRUE if point is on the line (actually, on the EPS-slab
// around the line).
{
	Real tmp = line.eval(point);
	return(std::abs(tmp) <= EPS);
}

inline bool operator<(const Point2d& point, const Line& line)
// Returns TRUE if point is to the left of the line (left to
// the EPS-slab around the line).
{
	return (line.eval(point) < -EPS);
}

}
#endif
