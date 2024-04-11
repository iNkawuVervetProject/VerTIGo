#include <stdio.h>
#include "pico/stdlib.h"

int main() {
	setup_default_uart();

	int n = 0;
	printf("\n");
	while (true) {
		n++;

		printf("\033[ATime is %d\n", n);
		sleep_ms(1000);
	}
	return 0;
}
