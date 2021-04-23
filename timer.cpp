#include "timer.h"
#include "io.h"
#include "irq.h"
#include "terminal.h"

u32 Timer::ticks     = 0;
u32 Timer::frequency = 18;

void Timer::setFrequency(u16 hz) {
	u16 divisor = 1193180 / hz;     /* Calculate our divisor */
	IO::outb(0x43, 0x36);           /* Set our command byte 0x36 */
	IO::outb(0x40, divisor & 0xFF); /* Set low byte of divisor */
	IO::outb(0x40, divisor >> 8);   /* Set high byte of divisor */
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
	Terminal::write("Tick..\n");

	/* Every 'frequency' clocks (approximately 1 second), we will
	 *  display a message on the screen */
	if(ticks % frequency == 0) {
		Terminal::prompt(VGA::Color::Blue, "Timer", "One second is done..");
	}
}

void Timer::wait(u32 t) {
	// Terminal::write("Ticks: ");
	// Terminal::write(t);
	// Terminal::write("\n");
	u32 end = ticks + t;
	while(ticks < end) {
	};
}

/* Sets up the system clock by installing the timer handler
 *  into IRQ0 */
void Timer::init() {
	Terminal::info("Setting up PIT..");
	Terminal::write("Ticks: ");
	Terminal::write(ticks);
	Terminal::write("\n");
	Timer::setFrequency(20);
	/* Installs 'timer_handler' to IRQ0 */
	Terminal::prompt(VGA::Color::Brown, "PIT", "Installing handler..");
	IRQ::installHandler(0, Timer::handler);
	Terminal::done("PIT setup complete..");
}
