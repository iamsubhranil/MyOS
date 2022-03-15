#include <arch/x86/idt.h>
#include <arch/x86/irq.h>
#include <drivers/io.h>
#include <drivers/terminal.h>
#include <drivers/timer.h>
#include <sched/scheduler.h>

volatile u32 Timer::ticks     = 0;
u32          Timer::frequency = 18;

void Timer::setFrequency(u32 hz) {
	Terminal::prompt(Terminal::Color::Orange, "PIT", "Setting frequency to ",
	                 hz, "..");
	u16 divisor = 1193180 / hz;     /* Calculate our divisor */
	IO::outb(0x43, 0x36);           /* Set our command byte 0x36 */
	IO::outb(0x40, divisor & 0xFF); /* Set low byte of divisor */
	IO::outb(0x40, divisor >> 8);   /* Set high byte of divisor */
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
	// Terminal::prompt(Terminal::Color::Blue, "Timer", "Tick..");

	/* Every 'frequency' clocks (approximately 1 second), we will
	 *  display a message on the screen */
	// if(ticks % frequency == 0) {
	//	Terminal::prompt(Terminal::Color::Blue, "Timer", "One second is
	// done..");
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
	Terminal::prompt(Terminal::Color::Orange, "PIT", "Installing handler..");
	IRQ::installHandler(0, Timer::handler);
	u32 freq = 100;
	Timer::setFrequency(freq);
	Terminal::done("PIT setup complete..");
}

extern "C" {
extern void _irq0();
// arch/x86/tsc.S
extern void _tsc_measure_irq();
extern u64  tsc_measure(u32 freq);
};

u64 Timer::calibrateTSC() {
	// set to a known freq
	u16 freqBak = frequency;
	u32 freq    = 1000;
	Timer::setFrequency(freq);
	// install calibration handler
	IDT::setGate(32, (uptr)_tsc_measure_irq, 0x08, 0x8E);
	// call measurement function
	// measure for less ticks, and then extrapolate
	u64 elapsed = tsc_measure(freq / 100) * 100;
	// reinstall the original handler
	IDT::setGate(32, (uptr)_irq0, 0x08, 0x8E);
	// reset the freq
	Timer::setFrequency(freqBak);
	// so, tsc / ms is elapsed / freq
	u64 elapsedAvg = (elapsed / freq) * (freq / 1000);
	return elapsedAvg;
}
