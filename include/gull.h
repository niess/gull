/*
 *  Geomagnetic UtiLities Library (GULL)
 *  Copyright (C) 2016  Valentin Niess
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GULL_H
#define GULL_h
#endif
#ifdef __cplusplus
extern "C" {
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
	/** No valid model could be found. */
	GULL_RETURN_MISSING_MODEL,
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
 * @param rc        The GULL return code.
 * @param caller    The caller function where the error occured.
 * @param file      The faulty file or `NULL`.
 * @param line      The faulty line or `0`.
 *
 * The user might provide its own error handler. It will be called at the
 * return of any GULL library function providing an error code.
 *
 * __Warnings__
 *
 * This callback **must** be thread safe if GULL is used in multithreaded
 * applications.
 */
typedef void gull_handler_cb(enum gull_return rc, gull_function_t * caller,
	const char * file, int line);

/** Opaque structure for handling magnetic models.
 */
struct gull_model;

enum gull_return gull_model_create(struct gull_model ** model,
	const char * path, int day, int month, int year, int * line);
void gull_model_destroy(struct gull_model ** model);
enum gull_return gull_model_magnet(struct gull_model * model, double latitude,
	double longitude, double altitude, double magnet[3]);

const char * gull_error_string(enum gull_return rc);
const char * gull_error_function(gull_function_t * function);
gull_handler_cb * gull_error_handler_get(void);
void gull_error_handler_set(gull_handler_cb * handler);
void gull_error_print(FILE*  stream, enum gull_return rc,
	gull_function_t * function, const char * file, int line);

#ifdef __cplusplus
}
#endif
