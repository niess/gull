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

#include "gull.h"
/* C89 standard library */
#include <float.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
/* Define pi, if unknown. */
#define M_PI 3.14159265358979323846
#endif

/* The user supplied error handler, if any. */
static gull_handler_cb * _handler;

/* Getter for the error handler. */
gull_handler_cb * gull_error_handler_get(void) { return _handler; }

/* Setter for the error handler. */
void gull_error_handler_set(gull_handler_cb * handler) { _handler = handler; }

struct error_context {
        enum gull_return code;
        gull_function_t * function;
#define ERROR_MSG_LENGTH 1024
        char message[ERROR_MSG_LENGTH];
};

/* Helper macros for returning an encapsulated error code. */
#define GULL_ERROR_INITIALISE(caller)                                          \
        struct error_context error_data = { .function =                        \
                                                (gull_function_t *)caller };   \
        struct error_context * error_ = &error_data;

#define GULL_ERROR_MESSAGE(rc, message)                                        \
        error_format(error_, rc, __FILE__, __LINE__, message),                 \
            error_raise(error_)

#define GULL_ERROR_FORMAT(rc, format, ...)                                     \
        error_format(error_, rc, __FILE__, __LINE__, format, __VA_ARGS__),     \
            error_raise(error_)

#define GULL_ERROR_REGISTER(rc, message)                                       \
        error_format(error_, rc, __FILE__, __LINE__, message)

#define GULL_ERROR_VREGISTER(rc, format, ...)                                  \
        error_format(error_, rc, __FILE__, __LINE__, format, __VA_ARGS__)

#define GULL_ERROR_RAISE() error_raise(error_)

/* Utility function for formating errors. */
static enum gull_return error_format(struct error_context * context,
    enum gull_return rc, const char * file, int line, const char * format, ...)
{
        context->code = rc;
        if ((_handler == NULL) || (rc == GULL_RETURN_SUCCESS)) return rc;

        /* Format the error message */
        const int n =
            snprintf(context->message, ERROR_MSG_LENGTH, "{ %s [#%d], %s:%d } ",
                gull_error_function(context->function), rc, file, line);
        if (n < ERROR_MSG_LENGTH - 1) {
                va_list ap;
                va_start(ap, format);
                vsnprintf(
                    context->message + n, ERROR_MSG_LENGTH - n, format, ap);
                va_end(ap);
        }

        return rc;
}

/* Utility function for handling errors. */
static enum gull_return error_raise(struct error_context * context)
{
        if ((_handler != NULL) && (context->code != GULL_RETURN_SUCCESS))
                _handler(context->code, context->function, context->message);
        return context->code;
}

/* Get a library function name as a string. */
const char * gull_error_function(gull_function_t * caller)
{
#define REGISTER_FUNCTION(function)                                            \
        if (caller == (gull_function_t *)function) return #function;

        /* API functions with error codes. */
        REGISTER_FUNCTION(gull_snapshot_create)
        REGISTER_FUNCTION(gull_snapshot_field)

        /* Other API functions. */
        REGISTER_FUNCTION(gull_snapshot_destroy)
        REGISTER_FUNCTION(gull_snapshot_info)
        REGISTER_FUNCTION(gull_error_function)
        REGISTER_FUNCTION(gull_error_handler_get)
        REGISTER_FUNCTION(gull_error_handler_set)

        return NULL;
#undef REGISTER_FUNCTION
}

/* Utility function for converting a calendar date to a decimal year. */
static enum gull_return date_decimal(
    int day, int month, int year, double * date, struct error_context * error_)
{
        *date = 0.;

        /* Start day in year of a month. */
        const int start_day[13] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273,
                304, 334, 365 };

        /* Check the input parameters. */
        if ((month < 1) || (month > 12)) {
                return GULL_ERROR_VREGISTER(
                    GULL_RETURN_DOMAIN_ERROR, "invalid month `%d`", month);
        }

        const int leap_year =
            (((year % 4) == 0) && (((year % 100) != 0) || ((year % 400) == 0)));
        const int days_in_month =
            start_day[month] - start_day[month - 1] + (month == 2) * leap_year;
        if ((day < 1) || (day > days_in_month)) {
                return GULL_ERROR_VREGISTER(GULL_RETURN_DOMAIN_ERROR,
                    "invalid day `%d` for month `%d`", day, month);
        }

        /* Compute the decimal year. */
        double day_in_year =
            (start_day[month - 1] + day + (month > 2 ? leap_year : 0));
        *date = ((double)year + (day_in_year / (365. + leap_year)));

        return GULL_RETURN_SUCCESS;
}

/* Low level data structure for hosting a snapshot of a geomagnetic model. */
struct gull_snapshot {
        /* The order of the spherical harmonics. */
        int order;
        /* The minimum allowed altitude, in km. */
        double altmin;
        /* The maximum allowed altitude, in km */
        double altmax;
        /* The spherical harmonic coefficients. */
        double coeff[];
};

/*
 * Utility function for accessing the spherical harmonic coefficents given a
 * set of indices (i, j <= i).
 */
static inline double * get_coeff(struct gull_snapshot * snapshot, int i, int j)
{
        i--;
        return snapshot->coeff + 2 * i * (i + 3) + 4 * j;
}

enum gull_return gull_snapshot_create(struct gull_snapshot ** snapshot,
    const char * path, int day, int month, int year)
{
        GULL_ERROR_INITIALISE(gull_snapshot_create);
        *snapshot = NULL;

        /* Get the decimal year. */
        double date;
        enum gull_return rc;
        if ((rc = date_decimal(day, month, year, &date, error_)) !=
            GULL_RETURN_SUCCESS)
                return GULL_ERROR_RAISE();

#define LINE_WIDTH 81
        /* Open the data file. */
        FILE * fid;
        if ((fid = fopen(path, "r")) == NULL) {
                return GULL_ERROR_FORMAT(
                    GULL_RETURN_PATH_ERROR, "could not open file `%s`", path);
        }

        /* Locate the relevant data set(s). */
        char buffer[LINE_WIDTH + 14];
        long position[2];
        int line_start[2];
        int nmax1[2], nmax2[2];
        double epoch[2], altmin[2], altmax[2];
        int ndat = 0;
        int line = 0;
        while (fgets(buffer, LINE_WIDTH + 12, fid)) {
                line++;
                if (strlen(buffer) != LINE_WIDTH) goto exit_on_syntax_error;

                if (strncmp(buffer, "   ", 3) == 0) {
                        /* This is a new data set. Let's parse and check the
                         * data set header.
                         */
                        double yrmin, yrmax;
                        int nread =
                            sscanf(buffer, "%*s %lf %d %d %*d %lf %lf %lf %lf",
                                epoch + ndat, nmax1 + ndat, nmax2 + ndat,
                                &yrmin, &yrmax, altmin + ndat, altmax + ndat);
                        if (nread != 7) goto exit_on_syntax_error;
                        if ((ndat == 0) && ((date < yrmin) || (date > yrmax)))
                                continue;

                        /* This is a valid data set. Let's backup its
                         * position.
                         */
                        position[ndat] = ftell(fid);
                        line_start[ndat] = line;
                        ndat++;
                        if ((ndat == 2) || (nmax2[0] > 0)) break;
                }
        }
        if ((ndat == 0) || ((ndat == 1) && (nmax2[0] <= 0))) {
                fclose(fid);
                return GULL_ERROR_FORMAT(
                    GULL_RETURN_MISSING_DATA, "missing data in file `%s`",
                    path);
        }

        /* Allocate the new and temporary memory. */
        int order;
        if (ndat == 1)
                order = (nmax1[0] > nmax2[0]) ? nmax1[0] : nmax2[0];
        else
                order = (nmax1[0] > nmax1[1]) ? nmax1[0] : nmax1[1];
        int size = sizeof(struct gull_snapshot) +
            2 * order * (order + 3) * sizeof(double);
        *snapshot = malloc(size);
        if (*snapshot == NULL) {
                GULL_ERROR_REGISTER(
                    GULL_RETURN_MEMORY_ERROR, "could not allocate memory");
                goto exit_on_runtime_error;
        }
        memset(*snapshot, 0x0, size);
        (*snapshot)->order = order;

        /*
         * This is a valid data set. Let's read the spherical harmonic
         * coefficients.
         */
        int idat;
        for (idat = 0; idat < ndat; idat++) {
                fseek(fid, position[idat], SEEK_SET);
                line = line_start[idat];

                const int nc = (ndat == 1) ?
                    (order * (order + 3)) / 2 :
                    (nmax1[idat] * (nmax1[idat] + 3)) / 2;
                int ic;
                for (ic = 0; ic < nc; ic++) {
                        /* Read a new line. */
                        line++;
                        if (!fgets(buffer, LINE_WIDTH + 12, fid))
                                goto exit_on_syntax_error;
                        if (strlen(buffer) != LINE_WIDTH)
                                goto exit_on_syntax_error;

                        /* Parse the line. */
                        int i, j;
                        double g1, h1, g2, h2;
                        int nread = sscanf(buffer, "%d %d %lf %lf %lf %lf", &i,
                            &j, &g1, &h1, &g2, &h2);
                        if ((nread != 6) || (j > i) || (i > order))
                                goto exit_on_syntax_error;
                        double * p = get_coeff(*snapshot, i, j);
                        if (ndat == 1) {
                                if ((p[0] != 0.) || (p[1] != 0.) ||
                                    (p[2] != 0.) || (p[3] != 0.))
                                        /* Check for a duplicated line. */
                                        goto exit_on_syntax_error;
                                p[0] = g1;
                                p[1] = h1;
                                p[2] = g2;
                                p[3] = h2;
                        } else if (idat == 0) {
                                if ((p[0] != 0.) || (p[1] != 0.))
                                        /* Check for a duplicated line. */
                                        goto exit_on_syntax_error;
                                p[0] = g1;
                                p[1] = h1;
                        } else {
                                if ((p[2] != 0.) || (p[3] != 0.))
                                        /* Check for a duplicated line. */
                                        goto exit_on_syntax_error;
                                p[2] = g1;
                                p[3] = h1;
                        }
                }
        }
        fclose(fid);
        fid = NULL;

        /* Interpolate or extrapolate for the required date. */
        if (ndat == 1) {
                /* Extrapolate. */
                const double h = date - epoch[0];
                const int nc = (order * (order + 3)) / 2;
                int ic;
                double *c0, *c1;
                for (ic = 0, c0 = c1 = (*snapshot)->coeff; ic < nc;
                     ic++, c0 += 4, c1 += 2) {
                        c1[0] = c0[0] + c0[2] * h;
                        c1[1] = c0[1] + c0[3] * h;
                }

                /* Set the altitude range. */
                (*snapshot)->altmin = altmin[0];
                (*snapshot)->altmax = altmax[0];
        } else {
                /* Interpolate. */
                const double h = (date - epoch[0]) / (epoch[1] - epoch[0]);
                const int nc = (order * (order + 3)) / 2;
                int ic;
                double *c0, *c1;
                for (ic = 0, c0 = c1 = (*snapshot)->coeff; ic < nc;
                     ic++, c0 += 4, c1 += 2) {
                        c1[0] = c0[0] * (1. - h) + c0[2] * h;
                        c1[1] = c0[1] * (1. - h) + c0[3] * h;
                }

                /* Set the altitude range. */
                (*snapshot)->altmin =
                    (altmin[0] > altmin[1]) ? altmin[0] : altmin[1];
                (*snapshot)->altmax =
                    (altmax[0] < altmax[1]) ? altmax[0] : altmax[1];
        }

        /* Free the extra memory and return. */
        size =
            sizeof(struct gull_snapshot) + order * (order + 3) * sizeof(double);
        *snapshot = realloc(*snapshot, size);

        return GULL_RETURN_SUCCESS;

exit_on_syntax_error:
        GULL_ERROR_VREGISTER(
            GULL_RETURN_FORMAT_ERROR, "invalid syntax [%s:%d]", path, line);

exit_on_runtime_error:
        if (fid != NULL) fclose(fid);
        gull_snapshot_destroy(snapshot);
        return GULL_ERROR_RAISE();

#undef MAX_NAME_SIZE
#undef SEGMENT_SIZE
#undef LINE_WIDTH
}

void gull_snapshot_destroy(struct gull_snapshot ** snapshot)
{
        if ((snapshot == NULL) || (*snapshot == NULL)) return;
        free(*snapshot);
        *snapshot = NULL;
}

/*
 * Compute the geomagnetic field components.
 *
 * This is an adaptation of geomag70/shval3 routine which is based on the
 * subroutine  'igrf' by D. R. Barraclough and S. R. C. Malin, report no. 71/1,
 * institute of geological sciences, U.K.
 */
enum gull_return gull_snapshot_field(struct gull_snapshot * snapshot,
    double latitude, double longitude, double altitude, double magnet[3],
    double ** workspace_)
{
        GULL_ERROR_INITIALISE(gull_snapshot_field);

        const double earths_radius = 6371.2;
        const double a2 = 40680631.59; /* WGS84. */
        const double b2 = 40408299.98; /* WGS84. */
        memset(magnet, 0x0, 3 * sizeof(double));

        /* Check the altitude. */
        altitude *= 1E-03; /* m -> km. */
        if ((altitude < snapshot->altmin) || (altitude > snapshot->altmax)) {
                GULL_ERROR_FORMAT(GULL_RETURN_DOMAIN_ERROR,
                    "invalid altitude value: %.5lE", altitude);
        }

        /* Configure the temporary work memory. */
        double * workspace = realloc((workspace_ == NULL) ? NULL : *workspace_,
            snapshot->order * (snapshot->order + 5) * sizeof(*workspace));
        if (workspace == NULL) {
                GULL_ERROR_MESSAGE(
                    GULL_RETURN_MEMORY_ERROR, "could not allocate memory");
        }
        if (workspace_ != NULL) *workspace_ = workspace;

        /*
         * Compute the sine and cosine of the latitude, with protection against
         * poles.
         */
        double slat = sin(latitude * M_PI / 180.);
        double aa;
        if ((90.0 - latitude) < 0.001) {
                aa = 89.999; /*  300 ft. from North pole  */
        } else {
                if ((90.0 + latitude) < 0.001)
                        aa = -89.999; /*  300 ft. from South pole  */
                else
                        aa = latitude;
        }
        double clat = cos(aa * M_PI / 180.);

        longitude *= M_PI / 180.;
        double * const sl = workspace;
        double * const cl = sl + snapshot->order;
        sl[0] = sin(longitude);
        cl[0] = cos(longitude);

        /* Convert to geocentric. */
        aa = a2 * clat * clat;
        double bb = b2 * slat * slat;
        double cc = aa + bb;
        const double dd = sqrt(cc);
        const double r =
            sqrt(altitude * (altitude + 2. * dd) + (a2 * aa + b2 * bb) / cc);
        const double ratio = earths_radius / r;
        const double cd = (altitude + dd) / r;
        const double sd = (a2 - b2) * slat * clat / (dd * r);
        aa = slat;
        slat = slat * cd - clat * sd;
        clat = clat * cd + aa * sd;

        /* Compute the magnetic field components. */
        const int npq = (snapshot->order * (snapshot->order + 3)) / 2;
        double * const p = cl + snapshot->order;
        double * const q = p + npq;
        aa = sqrt(3.);
        p[0] = 2. * slat;
        p[1] = 2. * clat;
        p[2] = 4.5 * slat * slat - 1.5;
        p[3] = 3. * aa * clat * slat;
        q[0] = -clat;
        q[1] = slat;
        q[2] = -3.0 * clat * slat;
        q[3] = aa * (slat * slat - clat * clat);

        double x = 0., y = 0., z = 0.;
        double rr = 0.;
        int k, n = 0, m = 1;
        const double * coeff;
        for (k = 0, coeff = snapshot->coeff; k < npq; k++, coeff += 2) {
                if (m > n) {
                        m = 0;
                        n++;
                        rr = pow(ratio, n + 2);
                }
                if (k >= 4) {
                        if (m == n) {
                                aa = sqrt(1. - 0.5 / m);
                                const int j = k - n - 1;
                                p[k] = (1. + 1. / m) * aa * clat * p[j];
                                q[k] = aa * (clat * q[j] + slat / m * p[j]);
                                sl[m - 1] =
                                    sl[m - 2] * cl[0] + cl[m - 2] * sl[0];
                                cl[m - 1] =
                                    cl[m - 2] * cl[0] - sl[m - 2] * sl[0];
                        } else {
                                aa = sqrt(n * n - m * m);
                                bb = sqrt((n - 1.) * (n - 1.) - m * m) / aa;
                                cc = (2. * n - 1.) / aa;
                                const int ii = k - n;
                                const int j = k - 2 * n + 1;
                                p[k] = (n + 1.) *
                                    (cc * slat / n * p[ii] -
                                        bb / (n - 1.) * p[j]);
                                q[k] = cc * (slat * q[ii] - clat / n * p[ii]) -
                                    bb * q[j];
                        }
                }
                aa = rr * coeff[0];
                if (m == 0) {
                        x += aa * q[k];
                        z -= aa * p[k];
                } else {
                        bb = rr * coeff[1];
                        cc = aa * cl[m - 1] + bb * sl[m - 1];
                        x += cc * q[k];
                        z -= cc * p[k];
                        if (clat > 0) {
                                y += (aa * sl[m - 1] - bb * cl[m - 1]) * m *
                                    p[k] / ((n + 1.) * clat);
                        } else {
                                y += (aa * sl[m - 1] - bb * cl[m - 1]) * q[k] *
                                    slat;
                        }
                }
                m++;
        }

        /* Free the temporary memory, if not claimed. */
        if (workspace_ == NULL) free(workspace);

        /* Fill and return. */
        magnet[0] = y * 1E-09;                  /* East.   */
        magnet[1] = (x * cd + z * sd) * 1E-09;  /* North.  */
        magnet[2] = -(z * cd - x * sd) * 1E-09; /* Upward. */

        return GULL_RETURN_SUCCESS;
}

/* Information on a geomagnetic snapshot. */
void gull_snapshot_info(struct gull_snapshot * snapshot, int * order,
    double * altitude_min, double * altitude_max)
{
        if (order != NULL) *order = snapshot->order;
        if (altitude_min != NULL) *altitude_min = snapshot->altmin * 1E+03;
        if (altitude_max != NULL) *altitude_max = snapshot->altmax * 1E+03;
}
