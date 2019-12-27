CC = gcc
CFLAGS = -O

all:test_brightness test_watchdog test_gpio test_led test_common test_eeprom watchdog_daemon

test_brightness: test_brightness.c
	$(CC) $(CFLAGS) -o test_brightness test_brightness.c
	@sync
test_gpio: test_gpio.c
	$(CC) $(CFLAGS) -o test_gpio test_gpio.c
	@sync
test_led: test_led.c
	$(CC) $(CFLAGS) -o test_led test_led.c
	@sync
test_common: test_common.c
	$(CC) $(CFLAGS) -o test_common test_common.c
	@sync
test_watchdog: test_watchdog.c
	$(CC) $(CFLAGS) -o test_watchdog test_watchdog.c
	@sync
test_eeprom: test_eeprom.c
	$(CC) $(CFLAGS) -o test_eeprom test_eeprom.c
	@sync
watchdog_daemon: watchdog_daemon.c
	$(CC) $(CFLAGS) -o watchdog_daemon watchdog_daemon.c
	@sync

clean:
	@rm -f *.o test_brightness test_gpio test_led test_common test_watchdog test_eeprom watchdog_daemon
