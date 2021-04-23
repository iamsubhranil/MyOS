#include "timer.h"
#include "io.h"
#include "irq.h"
#include "terminal.h"

volatile u32 Timer::ticks     = 0;
u32          Timer::frequency = 18;

void Timer::setFrequency(u16 hz) {
	Terminal::write("Setting freq ");
	Terminal::write(hz);
	Terminal::write("..\n");
	u16 divisor = 1193180 / hz; /* Calculate our divisor */
	Terminal::write("Sending command byte..\n");
	IO::outb(0x43, 0x36); /* Set our command byte 0x36 */
	Terminal::write("Sending low byte..\n");
	IO::outb(0x40, divisor & 0xFF); /* Set low byte of divisor */
	Terminal::write("Sending high byte..\n");
	IO::outb(0x40, divisor >> 8); /* Set high byte of divisor */
	Terminal::write("Freq set complete..\n");
	frequency = hz;
}

/*  Handles the timer. In this case, it's very simple: We
 *  increment the 'timer_ticks' variable every time the
 *  timer fires. By default, the timer fires 18.222 times
 *  per second. Why 18.222Hz? Some engineer at IBM must've
 *  been smoking something funky */
void Timer::handler(Register *r) {
	(void)r;
	/* Increment our 'tick count' */
	ticks++;
	// Terminal::prompt(VGA::Color::Blue, "Timer", "Tick..");

	/* Every 'frequency' clocks (approximately 1 second), we will
	 *  display a message on the screen */
	// if(ticks % frequency == 0) {
	//	Terminal::prompt(VGA::Color::Blue, "Timer", "One second is done..");
	// }
}

void Timer::wait(u32 t) {
	u32 end = ticks + t;
	while(ticks < end) {
	};
}

/* Sets up the system clock by installing the timer handler
 *  into IRQ0 */
void Timer::init() {
	Terminal::info("Setting up PIT..");
	/* Installs 'timer_handler' to IRQ0 */
	Terminal::prompt(VGA::Color::Brown, "PIT", "Installing handler..");
	IRQ::installHandler(0, Timer::handler);
	Timer::setFrequency(10000);
	Terminal::done("PIT setup complete..");
}
