#include <stdio.h>
#include <stdlib.h>
#include "gull.h"

int main()
{
	struct gull_model * model = NULL;
	const char * path = "data/IGRF12.COF";
	int line;
	enum gull_return rc = gull_model_create(&model, path,
		3, 10, 2016, &line);
	if (rc != GULL_RETURN_SUCCESS) {
		gull_error_print(stderr, rc,
			(gull_function_t *)gull_model_create, path, line);
		fprintf(stderr, "\n");
		exit(EXIT_FAILURE);
	}
	
	double magnet[3];
	rc = gull_model_magnet(model,  45.76415653, 2.95536402, 1090., magnet);
	if (rc != GULL_RETURN_SUCCESS) {
		gull_error_print(stderr, rc,
			(gull_function_t *)gull_model_magnet, NULL, 0);
		fprintf(stderr, "\n");
		exit(EXIT_FAILURE);
	}
	printf("{\"magnet [muT]\" : [%.5lf, %.5lf, %.5lf]}\n", magnet[0]*1E+06,
		magnet[1]*1E+06, magnet[2]*1E+06);

	gull_model_destroy(&model);
	exit(EXIT_SUCCESS);
}
