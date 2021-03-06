/*
 * Copyright (C) 2017 Université Clermont Auvergne, CNRS/IN2P3, LPC
 * Author: Valentin NIESS (niess@in2p3.fr)
 *
 * Geomagnetic UtiLities Library (GULL)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#ifndef GULL_H
#define GULL_h
#endif
#ifdef __cplusplus
extern "C" {
#endif

#ifndef FILE
#include <stdio.h>
#endif

/**
 * Return Codes used by GULL.
 */
enum gull_return {
        /** The operation succeeded. */
        GULL_RETURN_SUCCESS = 0,
        /** Some input value is out of its validity range. */
        GULL_RETURN_DOMAIN_ERROR,
        /** A provided file has a wrong format. */
        GULL_RETURN_FORMAT_ERROR,
        /** Some memory couldn't be allocated. */
        GULL_RETURN_MEMORY_ERROR,
        /** No valid data could be found. */
        GULL_RETURN_MISSING_DATA,
        /** A file couldn't be opened or found. */
        GULL_RETURN_PATH_ERROR,
        /** The number of GULL error codes. */
        GULL_N_RETURNS
};

/**
 * Generic function pointer.
 *
 * This is a generic function pointer used to identify the library functions,
 * e.g. for error handling.
 */
typedef void gull_function_t(void);

/**
 * Callback for error handling.
 *
 * @param rc         The GULL return code.
 * @param caller     The caller API function where the error occured.
 * @param message    A brief message describing the error.
 *
 * The user might provide its own error handler. It will be called at the
 * return of any GULL library function providing an error code.
 *
 * __Warnings__
 *
 * This callback **must** be thread safe if GULL is used in multithreaded
 * applications.
 */
typedef void gull_handler_cb(
    enum gull_return rc, gull_function_t * caller, const char * message);

/**
 * Opaque structure for handling snapshots of the geomagnetic field.
 */
struct gull_snapshot;

/**
 * Create a snapshot of a geomagnetic model.
 *
 * @param snapshot   A handle to the snapshot.
 * @param path       The file containing the geomagnetic model data.
 * @param day        The day in the month, i.e. in [1,31].
 * @param month      The month of the year, i.e. in [1, 12].
 * @param year       The year number, e.g. 2018.
 * @return On success `GULL_RETURN_SUCCESS` is returned otherwise an error
 * code is returned as detailed below.
 *
 * Create a snapshot of a geomagnetic model at the given date (*day*, *month*,
 * *year*) from the spherical harmonic coefficients loaded from *path*. See
 * below for a list of supported data formats.
 *
 * __Data formats__
 *
 *     *.COF    From **geomag70**. List of coefficients grouped by year, with
 * a header, e.g. IGRF12.COF or WWM2015.COF.
 *
 * __Error codes__
 *
 *     GULL_RETURN_MEMORY_ERROR     Couldn't allocate memory.
 *
 *     GULL_RETURN_PATH_ERROR       The data file couldn't be found/opened.
 *
 *     GULL_RETURN_MISSING_DATA     There is no valid data for the
 * requested date.
 */
enum gull_return gull_snapshot_create(struct gull_snapshot ** snapshot,
    const char * path, int day, int month, int year);

/**
 * Destroy a snapshot.
 *
 * @param snapshot   A handle to the snapshot.
 *
 * Fully destroy a snapshot and free any allocated memory.
 */
void gull_snapshot_destroy(struct gull_snapshot ** snapshot);

/**
 * Compute the geomagnetic field.
 *
 * @param snapshot     A handle to the snapshot.
 * @param latitude     The geodetic latitude (deg).
 * @param longitude    The geodetic longitude (deg).
 * @param altitude     The altitude (m) above the reference ellipsoid (WGS84).
 * @param field        The corresponding magnetic field E, N, U components (T).
 * @param workspace    A pointer to the temporary worspace or `NULL`.
 * @return On success `GULL_RETURN_SUCCESS` is returned otherwise an error
 * code is returned as detailed below.
 *
 * Compute the geomagnetic field components in East, North, Upward (ENU) from
 * a snapshot, at a given location on Earth. The computation requires extensive
 * temporary storage allocated on the heap. If *workspace* is not `NULL` the
 * corresponding memory address is returned to the user in order to be used
 * again in subsequent calls. Otherwise the temporary workspace is freed at
 * exit.
 *
 * This function is thread safe provided that each thread manages its own
 * *workspace*, which is in particular true if *workspace* is set to `NULL`.
 *
 * __Error codes__
 *
 *     GULL_RETURN_DOMAIN_ERROR    The provided altitude is not valid.
 *
 *     GULL_RETURN_MEMORY_ERROR    The temporary workspace could not be
 * (re)allocated.
 */
enum gull_return gull_snapshot_field(struct gull_snapshot * snapshot,
    double latitude, double longitude, double altitude, double magnet[3],
    double ** worspace);

/**
 * Information on a geomagnetic snapshot.
 *
 * @param snapshot        A handle to the snapshot.
 * @param order           The order used for spherical harmonics.
 * @param altitude_min    The minimum altitude (m) at which the model is valid.
 * @param altitude_max    The maximum altitude (m) at which the model is valid.
 *
 * Get some basic information on a geomagnetic snapshot. Note that any output
 * parameter can be set to `NULL` if the corresponding property is not needed.
 */
void gull_snapshot_info(struct gull_snapshot * snapshot, int * order,
    double * altitude_min, double * altitude_max);

/**
 * Return a string describing a GULL library function.
 *
 * @param function    The library function.
 * @return a static string.
 *
 * This function is meant for verbosing when handling errors.
 */
const char * gull_error_function(gull_function_t * function);

/**
 * Get the current error handler.
 *
 * @return The current error handler or `NULL` if none.
 */
gull_handler_cb * gull_error_handler_get(void);

/**
 * Set or clear the error handler.
 *
 * @param handler    The error handler to set or `NULL`.
 *
 * Set the error handler callback for GULL library functions. If *handler* is
 * set to `NULL` error callbacks are disabled.
 *
 * __Warnings__
 *
 * This function is **not** thread safe.
 */
void gull_error_handler_set(gull_handler_cb * handler);

#ifdef __cplusplus
}
#endif
