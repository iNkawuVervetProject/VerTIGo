#include "pico/stdlib.h"
#include <stdio.h>

int main() {
	setup_default_uart();

	int n = 0;
	printf("\033[2J Welcome to pellet dispenser debug output.\n\n");
	while (true) {
		n++;

		printf("\033[Auptime: %ds\n", n);
		sleep_ms(1000);
	}
	return 0;
}
