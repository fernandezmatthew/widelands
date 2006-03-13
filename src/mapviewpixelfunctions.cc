/*
 * Copyright (C) 2002-2004, 2006 by the Widelands Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "mapviewpixelfunctions.h"

// sqrt(1/3)
#define V3 static_cast<float>(0.57735)

#define LIGHT_FACTOR 75

/**
 * Calculate brightness based upon the slopes.
 */
float MapviewPixelFunctions::calc_brightness
(const int l, const int r, const int tl, const int tr, const int bl, const int br)
{
	static Vector sun_vect = Vector(V3, -V3, -V3);	// |sun_vect| = 1

	Vector normal;

// find normal
// more guessed than thought about
// but hey, results say i'm good at guessing :)
// perhaps i'll paint an explanation for this someday
// florian
#define COS60	0.5
#define SIN60	0.86603
#ifdef _MSC_VER
// don't warn me about fuckin float conversion i know what i'm doing
#pragma warning(disable:4244)
#endif
	normal = Vector(0, 0, TRIANGLE_WIDTH);
	normal.x -= l * HEIGHT_FACTOR;
	normal.x += r * HEIGHT_FACTOR;
	normal.x -= static_cast<float>(tl * HEIGHT_FACTOR) * COS60;
	normal.y -= static_cast<float>(tl * HEIGHT_FACTOR) * SIN60;
	normal.x += static_cast<float>(tr * HEIGHT_FACTOR) * COS60;
	normal.y -= static_cast<float>(tr * HEIGHT_FACTOR) * SIN60;
	normal.x -= static_cast<float>(bl * HEIGHT_FACTOR) * COS60;
	normal.y += static_cast<float>(bl * HEIGHT_FACTOR) * SIN60;
	normal.x += static_cast<float>(br * HEIGHT_FACTOR) * COS60;
	normal.y += static_cast<float>(br * HEIGHT_FACTOR) * SIN60;
	normal.normalize();
#ifdef _MSC_VER
#pragma warning(default:4244)
#endif

	float b = normal * sun_vect;
	b *= -LIGHT_FACTOR;

	return b;
}

/*
===============
Calculate the pixel (currently Manhattan) distance between the two points,
taking wrap-arounds into account.
===============
*/
unsigned int MapviewPixelFunctions::calc_pix_distance
(const Map & map, Point a, Point b)
{
	normalize_pix(map, a);
	normalize_pix(map, b);
	unsigned int dx = abs(a.x - b.x), dy = abs(a.y - b.y);
	{
		const uint map_end_screen_x = get_map_end_screen_x(map);
		if (dx > map_end_screen_x / 2) dx = -(dx - map_end_screen_x);
	}
	{
		const uint map_end_screen_y = get_map_end_screen_y(map);
		if (dy > map_end_screen_y / 2) dy = -(dy - map_end_screen_y);
	}
	return dx + dy;
}


/**
 * Documentation exists in HTML-format with figures. If you want it and can not
 * find it, ask Erik Sigra.
 */
Node_and_Triangle MapviewPixelFunctions::calc_node_and_triangle
(const Map & map, unsigned int x, unsigned int y)
{
	const unsigned short mapwidth = map.get_width();
	const unsigned short mapheight = map.get_height();
	const uint map_end_screen_x = get_map_end_screen_x(map);
	const uint map_end_screen_y = get_map_end_screen_y(map);
	while (x >= map_end_screen_x) x -= map_end_screen_x;
	while (y >= map_end_screen_y) y -= map_end_screen_y;
	Node_and_Triangle result;

	const unsigned short col_number = x / (TRIANGLE_WIDTH / 2);
	unsigned short row_number = y /  TRIANGLE_HEIGHT, next_row_number;
	assert(row_number < mapheight);
	const unsigned int left_col = col_number / 2;
	unsigned short right_col = (col_number + 1) / 2;
	if (right_col == mapwidth) right_col = 0;
	bool slash = (col_number + row_number) & 1;

	//  Find out which two nodes the mouse is between in the y-dimension, taking
	//  the height factor into account.
	//  There are 2 cases. One where the node in the left col has greater map
	//  y-coordinate than the right node. This is called slash because the edge
	//  between them goes in the direction of the '/' character. When slash is
	//  false, the edge goes in the direction of the '\' character.
	unsigned short screen_y_base = row_number * TRIANGLE_HEIGHT;
	int upper_screen_dy, lower_screen_dy =
		screen_y_base
		-
		map.get_field(slash ? right_col : left_col, row_number).get_height()
		*
		HEIGHT_FACTOR
		-
		y;
	for (;;) {
		screen_y_base += TRIANGLE_HEIGHT;
		next_row_number = row_number + 1;
		if (next_row_number == mapheight) next_row_number = 0;
		upper_screen_dy = lower_screen_dy;
		lower_screen_dy =
			screen_y_base
			-
			map.get_field(slash ? left_col : right_col, next_row_number)
			.get_height()
			*
			HEIGHT_FACTOR
			-
			y;
		if (lower_screen_dy < 0) {
			row_number = next_row_number;
			slash = not slash;
		} else break;
	}

	{//  Calculate which of the 2 nodes (x, y) is closest to.
		unsigned short upper_x, lower_x, upper_screen_dx, lower_screen_dx;
		if (slash) {
			upper_x = right_col;
			lower_x = left_col;
			lower_screen_dx = x - col_number * (TRIANGLE_WIDTH / 2);
			upper_screen_dx = (TRIANGLE_WIDTH / 2) - lower_screen_dx;
		} else {
			upper_x = left_col;
			lower_x = right_col;
			upper_screen_dx = x - col_number * (TRIANGLE_WIDTH / 2);
			lower_screen_dx = (TRIANGLE_WIDTH / 2) - upper_screen_dx;
		}
		if
			(upper_screen_dx * upper_screen_dx + upper_screen_dy * upper_screen_dy
			 <
			 lower_screen_dx * lower_screen_dx + lower_screen_dy * lower_screen_dy)
			result.node = Coords(upper_x, row_number);
		else result.node = Coords(lower_x, next_row_number);
	}

	//  Find out which of the 4 possible triangles (x, y) is in.
	if (slash) {
		int Y_a =
			screen_y_base - TRIANGLE_HEIGHT
			-
			map.get_field((right_col == 0 ? mapwidth : right_col) - 1, row_number)
			.get_height()
			*
			HEIGHT_FACTOR;
		int Y_b =
			screen_y_base - TRIANGLE_HEIGHT
			-
			map.get_field(right_col, row_number).get_height()
			*
			HEIGHT_FACTOR;
		int ldy = Y_b - Y_a, pdy = Y_b - y;
		int pdx = (col_number + 1) * (TRIANGLE_WIDTH / 2) - x;
		assert(pdx > 0);
		if (pdy * TRIANGLE_WIDTH > ldy * pdx) {
			//  (x, y) is in the upper triangle.
			result.triangle =
				TCoords
				(Coords(left_col, (row_number == 0 ? mapheight : row_number) - 1),
				 TCoords::D);
		} else {
			Y_a =
				screen_y_base
				-
				map.get_field(left_col, next_row_number).get_height()
				*
				HEIGHT_FACTOR;
			ldy = Y_b - Y_a;
			if (pdy * (TRIANGLE_WIDTH / 2) > ldy * pdx) {
				//  (x, y) is in the second triangle.
				result.triangle =
					TCoords
					(Coords((right_col == 0 ? mapwidth : right_col) - 1, row_number),
					 TCoords::R);
			} else {
				Y_b =
					screen_y_base
					-
					map.get_field
					(left_col + 1 == mapwidth ? 0 : left_col + 1, next_row_number)
					.get_height()
					*
					HEIGHT_FACTOR;
				ldy = Y_b - Y_a, pdy = Y_b - y;
				pdx = (col_number + 2) * (TRIANGLE_WIDTH / 2) - x;
				if (pdy * TRIANGLE_WIDTH > ldy * pdx) {
					//  (x, y) is in the third triangle.
					result.triangle =
						TCoords(Coords(right_col, row_number), TCoords::D);
				} else {
					//  (x, y) is in the lower triangle.
					result.triangle =
						TCoords(Coords(left_col, next_row_number), TCoords::R);
				}
			}
		}
	} else {
		int Y_a =
			screen_y_base - TRIANGLE_HEIGHT
			-
			map.get_field(left_col, row_number).get_height()
			*
			HEIGHT_FACTOR;
		int Y_b =
			screen_y_base - TRIANGLE_HEIGHT
			-
			map.get_field(left_col + 1 == mapwidth ? 0 : left_col + 1, row_number)
			.get_height()
			*
			HEIGHT_FACTOR;
		int ldy = Y_b - Y_a, pdy = Y_b - y;
		int pdx = (col_number + 2) * (TRIANGLE_WIDTH / 2) - x;
		assert(pdx > 0);
		if (pdy * TRIANGLE_WIDTH > ldy * pdx) {
			//  (x, y) is in the upper triangle.
			result.triangle =
				TCoords
				(Coords(right_col, (row_number == 0 ? mapheight : row_number) -1),
				 TCoords::D);
		} else {
			Y_b =
				screen_y_base
				-
				map.get_field(right_col, next_row_number).get_height()
				*
				HEIGHT_FACTOR;
			ldy = Y_b - Y_a, pdy = Y_b - y;
			pdx = (col_number + 1) * (TRIANGLE_WIDTH / 2) - x;
			if (pdy * (TRIANGLE_WIDTH / 2) > ldy * pdx) {
				//  (x, y) is in the second triangle.
				result.triangle.x = left_col;
				result.triangle.y = row_number;
				result.triangle.t = TCoords::R;
			} else {
				Y_a =
					screen_y_base
					-
					map.get_field
					((right_col == 0 ? mapwidth : right_col) - 1, next_row_number)
					.get_height()
					*
					HEIGHT_FACTOR;
				ldy = Y_b - Y_a;
				if (pdy * TRIANGLE_WIDTH > ldy * pdx) {
					//  (x, y) is in the third triangle.
					result.triangle =
						TCoords(Coords(left_col, row_number), TCoords::D);
				} else {
					//  (x, y) is in the lower triangle.
					result.triangle =
						TCoords
						(Coords
						 ((right_col == 0 ? mapwidth : right_col) - 1,
						 next_row_number),
						 TCoords::R);
				}
			}
		}
	}

	return result;
}


/*
===============
Normalize pixel points of the map.
===============
*/
void MapviewPixelFunctions::normalize_pix(const Map & map, Point & p) {
	{
		const int map_end_screen_x = get_map_end_screen_x(map);
		while (p.x >= map_end_screen_x) p.x -= map_end_screen_x;
		while (p.x < 0)                 p.x += map_end_screen_x;
	}
	{
		const int map_end_screen_y = get_map_end_screen_y(map);
		while (p.y >= map_end_screen_y) p.y -= map_end_screen_y;
		while (p.y < 0)                 p.y += map_end_screen_y;
	}
}
