/**
 * This example illustrates the basic functionalities of the GULL library,
 * i.e. loading a snapshot of the geomagnetic field and computing its components
 * at some Earth location. It also provides a simple example of GULL's error
 * handling using a user defined ***gull_handle_cb** callback.
 */

#include "gull.h"
/* C89 standard library */
#include <stdio.h>
#include <stdlib.h>

/* Handle for a snapshot of the geomagnetic field. */
static struct gull_snapshot * snapshot = NULL;

/* Error handler: dump any error message and exit to the OS. */
void handle_error(
    enum gull_return rc, gull_function_t * caller, const char * message)
{
        /* Dump an error message. */
        fprintf(stderr, "%s\n", message);

        /* Finalise and exit to the OS. */
        gull_snapshot_destroy(&snapshot);
        exit(EXIT_FAILURE);
}

int main(int argc, char * argv[])
{
        /** First let us set an error handler for GULL library functions. Most
         * of GULL's library functions return a `gull_return` code that cand be
         * checked explicitly or catched by a user defined error handler.
         */
        /* Register the error handler for GULL library functions. */
        gull_error_handler_set(&handle_error);

        /** Let us now load a snapshot of the geomagnetic field from the disk
         * and show how to access its meta-data.
         */
        /* Create a snapshot of the magnetic field. */
        const char * path = (argc == 1) ? "share/data/IGRF13.COF" : argv[1];
        const int day = 23, month = 3, year = 2020;
        gull_snapshot_create(&snapshot, path, day, month, year);

        int order;
        double z_min, z_max;
        gull_snapshot_info(snapshot, &order, &z_min, &z_max);

        printf("# Snapshot\n");
        printf("- date       : %d/%d/%d\n", day, month, year);
        printf("- data set   : %s\n", path);
        printf("- altitude   : [%.0lf, %.0lf] (m)\n", z_min, z_max);

        /** Then, let us show how to get the corresponding geomagnetic
         * components at some specific location on Earth. The components are
         * given in unit Tesla in **E**ast, **North** and **U**pward (E,N,U)
         * coordinates.
         */
        /*
         * Get the components at Auberge des Gros Manaux, Puy de Dome, France.
         */
        const double latitude = 45.76415653;
        const double longitude = 2.95536402;
        const double altitude = 1090.;
        double field[3];
        gull_snapshot_field(
            snapshot, latitude, longitude, altitude, field, NULL);

        printf("# Geomagnetic field\n");
        printf("- location   : [%.5lf, %.5lf] (deg)\n", latitude, longitude);
        printf("- components : [%.0lf, %.0lf, %.0lf] (nT)\n", field[0] * 1E+09,
            field[1] * 1E+09, field[2] * 1E+09);

        /* Finalise and exit to the OS. */
        gull_snapshot_destroy(&snapshot);
        exit(EXIT_SUCCESS);
}
