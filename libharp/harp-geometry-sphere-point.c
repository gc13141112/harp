/*
 * Copyright (C) 2015-2016 S[&]T, The Netherlands.
 *
 * This file is part of HARP.
 *
 * HARP is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * HARP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HARP; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "harp-geometry.h"

/* Check if two spherical points are equal */
int harp_spherical_point_equal(const harp_spherical_point *pointa, const harp_spherical_point *pointb)
{
    harp_vector3d vectora, vectorb;

    harp_vector3d_from_spherical_point(&vectora, pointa);
    harp_vector3d_from_spherical_point(&vectorb, pointb);

    return (harp_vector3d_equal(&vectora, &vectorb));
}

/* Check spherical point */
void harp_spherical_point_check(harp_spherical_point *point)
{
    static int lat_is_negative;

    /* Check if latitude is negative */
    lat_is_negative = (point->lat < 0.0) ? (1) : (0);

    point->lat = point->lat - floor(point->lat / (2.0 * M_PI)) * (2.0 * M_PI);
    point->lon = point->lon - floor(point->lon / (2.0 * M_PI)) * (2.0 * M_PI);

    if (point->lon < 0.0)
    {
        point->lon += 2.0 * M_PI;
    }

    if (point->lat > M_PI)
    {
        point->lat -= 2.0 * M_PI;
    }
    if (point->lat > M_PI_2)
    {
        point->lat = M_PI - point->lat;
        point->lon += ((point->lon < M_PI) ? (M_PI) : (-M_PI));
    }
    if (point->lat < -M_PI_2)
    {
        point->lat = (double)(-M_PI) - point->lat;
        point->lon += ((point->lon < (double)M_PI) ? ((double)M_PI) : ((double)(-M_PI)));
    }

    if (HARP_GEOMETRY_FPeq(point->lat, M_PI_2) && lat_is_negative)
    {
        point->lat = -M_PI_2;
    }

    if (HARP_GEOMETRY_FPeq(point->lon, 2.0 * M_PI))
    {
        point->lon = 0.0;
    }

    if (HARP_GEOMETRY_FPzero(point->lon))
    {
        point->lon = 0.0;
    }

    if (HARP_GEOMETRY_FPzero(point->lat))
    {
        point->lat = 0.0;
    }
}

/* Convert spherical point (lat,lon) to
 * point (x,y,z) in Cartesian coordinates
 *
 * Input:
 *   Spherical point p = (p.lat, p.lon) in [rad]
 *
 * Output:
 *   3D vector vector = (vector.x, vector.y, vector.z) [dl]
 *
 * Details:
 *
 *   Convert (lat,lon) coordinates on unit sphere to
 *   Cartesian coordinates (x,y,z) with:
 *
 *     x = cos(lat) * cos(lon);
 *     y = cos(lat) * sin(lon);
 *     z = sin(lat)
 *
 *   Here, (lat,lon) are in [rad].
 */
void harp_vector3d_from_spherical_point(harp_vector3d *vector, const harp_spherical_point *point)
{
    vector->x = cos(point->lat) * cos(point->lon);
    vector->y = cos(point->lat) * sin(point->lon);
    vector->z = sin(point->lat);
}

/* Convert point (x,y,z) in Cartesian coordinates to
 * spherical point (lat,lon)
 *
 * Input:
 *   3D vector vector = (vector.x, vector.y, vector.z) [dl]
 *
 * Output:
 *   Spherical point p = (p.lat, p.lon) in [rad]
 */
void harp_spherical_point_from_vector3d(harp_spherical_point *point, const harp_vector3d *vector)
{
    /* Calculate the radius in the (x,y)-plane */
    double rho = pow(vector->x * vector->x + vector->y * vector->y, 0.5);

    if (rho == 0.0)
    {
        if (vector->z == 0.0)
        {
            point->lat = 0.0;
        }
        else if (vector->z > 0.0)
        {
            point->lat = M_PI_2;
        }
        else if (vector->z < 0.0)
        {
            point->lat = -M_PI_2;
        }
    }
    else
    {
        point->lat = atan(vector->z / rho);
    }
    point->lon = atan2(vector->y, vector->x);
}

/* Convert unit of spherical point (lat,lon)
 * from [deg] to [rad]
 *
 * Input:
 *   Spherical point p = (p.lat, p.lon) in [deg]
 *
 * Output:
 *   Same point in [rad]

 * Details:
 *   Conversion factor to convert from
 *   [deg] to [rad] = pi/180
 */
void harp_spherical_point_rad_from_deg(harp_spherical_point *point)
{
    point->lat = point->lat * (double)(CONST_DEG2RAD);
    point->lon = point->lon * (double)(CONST_DEG2RAD);
}

/* Convert unit of spherical point (lat,lon)
 * from [rad] to [deg]
 *
 * Input:
 *   Spherical point p = (p.lat, p.lon) in [rad]
 *
 * Output:
 *   Same point in [deg]
 *
 * Details:
 *   Conversion factor to convert from
 *   [rad] to [deg] = 180/pi
*/
void harp_spherical_point_deg_from_rad(harp_spherical_point *point)
{
    point->lat = point->lat * (double)(CONST_RAD2DEG);
    point->lon = point->lon * (double)(CONST_RAD2DEG);
}

/* Calculate the surface_distance between two points
 * on the surface of a sphere */
double harp_spherical_point_distance(const harp_spherical_point *pointp, const harp_spherical_point *pointq)
{
    double distance =
        (acos
         (sin(pointp->lat) * sin(pointq->lat) + cos(pointp->lat) * cos(pointq->lat) * cos(pointp->lon - pointq->lon)));

    if (HARP_GEOMETRY_FPzero(distance))
    {
        return 0.0;
    }
    else
    {
        return distance;
    }
}

/* Calculate the surface_distance between two points
 * on the surface of a sphere (having the Earth radius) */
double harp_spherical_point_distance_in_meters(const harp_spherical_point *pointp, const harp_spherical_point *pointq)
{
    return harp_spherical_point_distance(pointp, pointq) * CONST_EARTH_RADIUS_WGS84_SPHERE;
}

/* Return point distance in meters */
int harp_spherical_point_distance_from_longitude_latitude(double longitude_a, double latitude_a,
                                                          double longitude_b, double latitude_b, double *point_distance)
{
    harp_spherical_point point_a, point_b;

    point_a.lat = latitude_a * (double)(CONST_DEG2RAD);
    point_a.lon = longitude_a * (double)(CONST_DEG2RAD);
    point_b.lat = latitude_b * (double)(CONST_DEG2RAD);
    point_b.lon = longitude_b * (double)(CONST_DEG2RAD);

    harp_spherical_point_check(&point_a);
    harp_spherical_point_check(&point_b);

    *point_distance = harp_spherical_point_distance_in_meters(&point_a, &point_b);

    return 0;
}

/* Obtain a point b that is a distance 'radius' [m] away and with given azimuth angle from point a.
 * The azimuth angle is defined clockwise when looking downward. */
int harp_spherical_point_at_distance_and_angle(const harp_spherical_point *point_a, double radius,
                                               double azimuth_angle, harp_spherical_point *point_b)
{
    double earth_radius = (double)(CONST_EARTH_RADIUS_WGS84_SPHERE);    /* [m] */
    harp_euler_transformation se;

    /* Convert the azimuth angles from [deg] to [rad] */
    azimuth_angle *= CONST_DEG2RAD;

    /* Derive the point which lies a distance 'radius' (normalized to [rad]) north of this point */
    point_b->lon = 0.0;
    point_b->lat = radius / (2.0 * M_PI * earth_radius);
    harp_spherical_point_check(point_b);

    /* Perform translation of (0,0) to point a to get point b relative to point a */
    se.phi_axis = 'X';
    se.theta_axis = 'Y';
    se.psi_axis = 'Z';
    se.phi = azimuth_angle;
    se.theta = -point_a->lat;
    se.psi = point_a->lon;
    harp_spherical_point_apply_euler_transformation(point_b, point_b, &se);

    return 0;
}

/*#################
 * Array of points
 *#################*/

/* Create a new spherical point array */
int harp_spherical_point_array_new(harp_spherical_point_array **new_point_array)
{
    harp_spherical_point_array *point_array = NULL;

    point_array = (harp_spherical_point_array *)malloc(sizeof(harp_spherical_point_array));
    if (point_array == NULL)
    {
        harp_set_error(HARP_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)\n",
                       (long)sizeof(harp_spherical_point_array), __FILE__, __LINE__);
        return -1;
    }

    point_array->numberofpoints = 0;
    point_array->point = NULL;

    *new_point_array = point_array;
    return 0;
}

/* Add point to point array */
int harp_spherical_point_array_add_point(harp_spherical_point_array *point_array, const harp_spherical_point *point_in)
{
    point_array->point = realloc(point_array->point, (point_array->numberofpoints + 1) * sizeof(harp_spherical_point));
    if (point_array->point == NULL)
    {
        harp_set_error(HARP_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)\n",
                       (point_array->numberofpoints + 1) * sizeof(harp_spherical_point), __FILE__, __LINE__);
        return -1;
    }
    point_array->point[point_array->numberofpoints].lon = point_in->lon;
    point_array->point[point_array->numberofpoints].lat = point_in->lat;
    point_array->numberofpoints++;
    return 0;
}

/* Remove point at the specified index from the point array. */
void harp_spherical_point_array_remove_point_at_index(harp_spherical_point_array *point_array, int32_t index)
{
    int32_t i;

    assert(point_array != NULL);
    assert(index >= 0 && index < point_array->numberofpoints);

    for (i = index + 1; i < point_array->numberofpoints; i++)
    {
        point_array->point[i - 1] = point_array->point[i];
    }
    point_array->numberofpoints--;
}

/* Delete a spherical point array */
void harp_spherical_point_array_delete(harp_spherical_point_array *point_array)
{
    if (point_array == NULL)
    {
        return;
    }
    if (point_array->point != NULL)
    {
        free(point_array->point);
    }
    free(point_array);
}
