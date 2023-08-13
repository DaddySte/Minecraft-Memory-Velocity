#include "Velocity.h"

//Example on How To Use

int main() {
	timeBeginPeriod(1);
	std::thread(VelocityScanner).detach();

	while (true) {
		changeVelocity(0, 0, 100, 100, 100, false, false, false);
	}
}