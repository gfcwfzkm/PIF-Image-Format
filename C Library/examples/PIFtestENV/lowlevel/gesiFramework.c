/**
 * @file gesiFramework.c
 * @brief C-Datei des Frameworks.
 */ 

#include "gesiFramework.h"

// the prescaler is set so that timer0 ticks every 64 clock cycles, and the
// the overflow handler is called every 256 ticks.
#define MICROSECONDS_PER_TIMER0_OVERFLOW (clockCyclesToMicroseconds(64 * 256))

// the whole number of milliseconds per timer0 overflow
#define MILLIS_INC (MICROSECONDS_PER_TIMER0_OVERFLOW / 1000)

// the fractional number of milliseconds per timer0 overflow. we shift right
// by three to fit these numbers into a byte. (for the clock speeds we care
// about - 8 and 16 MHz - this doesn't lose precision.)
#define FRACT_INC ((MICROSECONDS_PER_TIMER0_OVERFLOW % 1000) >> 3)
#define FRACT_MAX (1000 >> 3)

uint8_t analog_reference = DEFAULT;


#if defined(__AVR_XMEGA__)
volatile unsigned long millis_count;
ISR(TCE0_OVF_vect)
{
	++millis_count;
}

uint32_t gf_millis()
{
	uint8_t oldSREG = SREG;
	cli();
	uint32_t result = millis_count;
	SREG = oldSREG;
	
	return result;
}

uint32_t gf_micros()
{
	uint8_t oldSREG = SREG;
	cli();
	uint32_t result = millis_count * 1000 + (TCE0.CNT >> 2);
	SREG = oldSREG;
	
	return result;
}

#else

volatile unsigned long millis_count;
static uint8_t timer_fract = 0;
volatile uint32_t timer_overflow_count = 0;

#if defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
ISR(TIM0_OVF_vect)
#else
ISR(TIMER0_OVF_vect)
#endif
{
	uint32_t m = millis_count;
	uint8_t f = timer_fract;

	m += MILLIS_INC;
	f += FRACT_INC;
	if (f >= FRACT_MAX) {
		f -= FRACT_MAX;
		m += 1;
	}

	timer_fract = f;
	millis_count = m;
	timer_overflow_count++;
}

uint32_t gf_millis()
{
	uint32_t m;
	uint8_t oldSREG = SREG;

	cli();
	m = millis_count;
	SREG = oldSREG;

	return m;
}

uint32_t gf_micros()
{
	uint32_t m;
	uint8_t oldSREG = SREG;
	uint8_t t;
	
	cli();
	m = timer_overflow_count;
	#if defined(TCNT0)
	t = TCNT0;
	#elif defined(TCNT0L)
	t = TCNT0L;
	#elif defined(TCE0_CNTL)
	t = (uint8_t)TCE0.CNT;
	#else
	#error TIMER 0 not defined
	#endif

	#ifdef TIFR0
	if ((TIFR0 & _BV(TOV0)) && (t < 255))
	m++;
	#else
	if ((TIFR & _BV(TOV0)) && (t < 255))
	m++;
	#endif

	SREG = oldSREG;
	
	return ((m << 8) + t) * (64 / clockCyclesPerMicrosecond());
}
#endif

void gf_delay_us(uint32_t us)
{
	uint32_t start = gf_micros();
	
	while (gf_micros() - start <= us);
}

void gf_delay_ms(uint32_t ms)
{
	uint32_t start = gf_millis();
	
	while (gf_millis() - start <= ms);
}

//This initializes the FIFO structure with the given buffer and size
void gf_fifo_init(fifo_t * f, uint8_t * buf, uint16_t size){
	f->head = 0;
	f->tail = 0;
	f->size = size;
	f->buf = buf;
}

uint16_t gf_fifo_read(fifo_t * f)
{
	uint16_t data;
	if (f->tail != f->head)
	{
		data = f->buf[f->tail];
		f->tail++;
		if (f->tail == f->size)
		{
			f->tail = 0;
		}
	}
	else
	{
		data = 0xFF00;
	}
	return data;
}

//This reads nbytes bytes from the FIFO
//The number of bytes read is returned
uint16_t gf_fifo_reads(fifo_t * f, void * buf, uint16_t nbytes){
	uint16_t i;
	uint8_t * p;
	p = buf;
	for(i=0; i < nbytes; i++)
	{
		if( f->tail != f->head )
		{ //see if any data is available
			*p++ = f->buf[f->tail];  //grab a byte from the buffer
			f->tail++;  //increment the tail
			if( f->tail == f->size )
			{  //check for wrap-around
				f->tail = 0;
			}
		} 
		else 
		{
			return i; //number of bytes read
		}
	}
	return nbytes;
}

// this writes one byte into the FIFO buffer
// If the head runs into the tail, it returns a "one" and cancels the operation
uint8_t gf_fifo_write(fifo_t * f, uint8_t data)
{
	if ( (f->head + 1 == f->tail) || ( (f->head + 1 == f->size) && (f->tail == 0) ) )
	{
		return 1;
	}
	else
	{
		f->buf[f->head] = data;
		f->head++;
		if (f->head == f->size)
		{
			f->head = 0;
		}
		return 0;
	}
}

//This writes up to nbytes bytes to the FIFO
//If the head runs in to the tail, not all bytes are written
//The number of bytes written is returned
uint16_t gf_fifo_writes(fifo_t * f, const void * buf, uint16_t nbytes){
	uint16_t i;
	const uint8_t * p;
	p = buf;
	for(i=0; i < nbytes; i++)
	{
		//first check to see if there is space in the buffer
		if ( (f->head + 1 == f->tail) || ( (f->head + 1 == f->size) && (f->tail == 0) ) )
		{
			return i; //no more room
		}
		else 
		{
			f->buf[f->head] = *p++;
			f->head++;  //increment the head
			if( f->head == f->size )
			{  //check for wrap-around
				f->head = 0;
			}
		}
	}
	return nbytes;
}

uint8_t gf_shiftIn(uint8_t dataPin, uint8_t clockPin, enum bitOrder _bitorder, volatile uint8_t *portout, volatile uint8_t *portin) {
	uint8_t value = 0;
	uint8_t i;

	for (i = 0; i < 8; ++i)
	{
		*portout |= (1<<clockPin);
		if (_bitorder == LSBFIRST)
		{
			value |= (*portin & (1<<dataPin)) << i;
		}
		else
		{
			value |= (*portin & (1<<dataPin)) << (7 - i);
		}
		*portout &=~ (1<<clockPin);
	}
	return value;
}

void gf_shiftOut(uint8_t dataPin, uint8_t clockPin, enum bitOrder _bitorder, uint8_t val, volatile uint8_t *port)
{
	uint8_t i;
	
	for (i = 0; i < 8; i++)
	{
		if (_bitorder == LSBFIRST)
		{
			*port |= (1 << (!!(val & (1 << i))));
		}
		else
		{
			*port |= (1 << (!!(val & (1 << (7 - i)))));
		}
		
		*port |= (1<<clockPin);
		*port &=~ (1<<clockPin);
	}
}

void gf_init()
{
	// this needs to be called before setup() or some functions won't
	// work there
	
	sei();
#if defined(TCCR0A) && defined(WGM01)
	sbi(TCCR0A, WGM01);
	sbi(TCCR0A, WGM00);
#endif
		
	// set timer 0 prescale factor to 64
#if defined(__AVR_ATmega128__)
	// CPU specific: different values for the ATmega128
	sbi(TCCR0, CS02);
#elif defined(TCCR0) && defined(CS01) && defined(CS00)
	// this combination is for the standard atmega8
	sbi(TCCR0, CS01);
	sbi(TCCR0, CS00);
#elif defined(TCCR0B) && defined(CS01) && defined(CS00)
	// this combination is for the standard 168/328/1280/2560
	sbi(TCCR0B, CS01);
	sbi(TCCR0B, CS00);
#elif defined(TCCR0A) && defined(CS01) && defined(CS00)
	// this combination is for the __AVR_ATmega645__ series
	sbi(TCCR0A, CS01);
	sbi(TCCR0A, CS00);
#elif defined(__AVR_XMEGA__) && defined(TCE0_CNTL)
	// XMEGA has no classic 8bit timers anymore, so the lower 8bit of TCE0 is used
	// and the upper are preset to 0xFF
	TCE0.PERBUF = 4000;
	TCE0.CTRLA = TC_CLKSEL_DIV8_gc;
	TCE0.CTRLB = TC_WGMODE_NORMAL_gc;
#else
	#error Timer 0 prescale factor 64 not set correctly
#endif

	// enable timer 0 overflow interrupt
#if defined(TIMSK) && defined(TOIE0)
	sbi(TIMSK, TOIE0);
#elif defined(TIMSK0) && defined(TOIE0)
	sbi(TIMSK0, TOIE0);
#elif defined(__AVR_XMEGA__) && defined(TCC2_LCNT)
	PMIC.CTRL = PMIC_LOLVLEN_bm;
	TCE0.INTCTRLA = TC0_OVFINTLVL0_bm;
#else
	#error	Timer 0 overflow interrupt not set correctly
#endif
}

// Returns "true" only once when button is pressed
uint8_t gf_isButtonPressed(btn_check_t *b)
{
	if (*b->btn_port & (1<<b->btn_pin))
	{
		if (!(*b->btn_pressedVar & (1<<b->btn_pin)))
		{
			// Optional Beep or LED-Blink
			if (b->beep_port != 0)
			{				
				if (b->beep_activeState)
				{
					*b->beep_port |= (1<<b->beep_pin);
				}
				else
				{
					*b->beep_port &=~ (1<<b->beep_pin);
				}
				gf_delaymsLong(b->beep_timeMS);
				if (b->beep_activeState)
				{
					*b->beep_port &=~ (1<<b->beep_pin);
				}
				else
				{
					*b->beep_port |= (1<<b->beep_pin);
				}
			}
			else
			{
				gf_delaymsLong(b->beep_timeMS);
			}
			*b->btn_pressedVar |= (1<<b->btn_pin);
			return (1);
		}
	}
	else
	{
		*b->btn_pressedVar &=~ (1<<b->btn_pin);
	}
	return (0);
}

// Returns "true" only once when button is released
uint8_t gf_isButtonReleased(btn_check_t *b)
{
	if (*b->btn_port & (1<<b->btn_pin))
	{
		*b->btn_releasedVar |= (1<<b->btn_pin);
	}
	else
	{
		if (*b->btn_releasedVar & (1<<b->btn_pin))
		{
			// Optional Beep or LED-Blink
			if (b->beep_port != 0)
			{
				if (b->beep_activeState)
				{
					*b->beep_port |= (1<<b->beep_pin);
				}
				else
				{
					*b->beep_port &=~ (1<<b->beep_pin);
				}
				gf_delaymsLong(b->beep_timeMS);
				if (b->beep_activeState)
				{
					*b->beep_port &=~ (1<<b->beep_pin);
				}
				else
				{
					*b->beep_port |= (1<<b->beep_pin);
				}
			}
			else
			{
				gf_delaymsLong(b->beep_timeMS);
			}
			*b->btn_releasedVar &=~ (1<<b->btn_pin);
			return (1);
		}
	}
	return (0);
}

uint8_t gf_TimeCheck(gf_delay_t *m, uint32_t _time)
{
	if (_time >= (m->callEveryTime + m->oldCallTime))
	{
		m->oldCallTime = _time;
		return 1;
	}
	else
	{
		return 0;
	}
}

void (*userFunc[GF_TASK_SIZE])(void);
uint32_t oldCallTime[GF_TASK_SIZE];
uint32_t callEveryTime[GF_TASK_SIZE];
uint8_t taskInfo[GF_TASK_SIZE];
uint8_t taskListCount = 0;

uint16_t gf_addTask(void (*userFunction)(void), uint32_t timeRun)
{
	if (taskListCount == GF_TASK_SIZE)	return ((GF_TASK_MAXREACHED << 8) | taskListCount);
	
	userFunc[taskListCount] = userFunction;
	callEveryTime[taskListCount] = timeRun;
	taskInfo[taskListCount] = GF_TASK_RUNNING;
	oldCallTime[taskListCount] = 0;
	
	taskListCount++;
		
	return ((GF_TASK_DONE << 8) | taskListCount);
}

void gf_runScheduler(uint32_t _time)
{
	for (uint8_t i = 0; i < taskListCount; i++)
	{
		if (_time > (oldCallTime[i] + callEveryTime[i]))
		{
			if (taskInfo[i] == GF_TASK_RUNNING)
			{
				oldCallTime[i] = _time;
				(*userFunc[i])();
			}
		}
	}
}

uint8_t gf_pauseTask(void (*userFunction)(void))
{
	uint8_t returnVar = GF_TASK_NOTFOUND;
	for (uint8_t i = 0; i < taskListCount; i++)
	{
		if (userFunc[i] == userFunction)
		{
			taskInfo[i] = GF_TASK_PAUSED;
			returnVar = GF_TASK_PAUSED;
			break;
		}
	}
	return returnVar;
}

uint8_t gf_runTask(void (*userFunction)(void))
{
	uint8_t returnVar = GF_TASK_NOTFOUND;
	for (uint8_t i = 0; i < taskListCount; i++)
	{
		if (userFunc[i] == userFunction)
		{
			taskInfo[i] = GF_TASK_RUNNING;
			returnVar = GF_TASK_RUNNING;
			break;
		}
	}
	return returnVar;
}

uint8_t gf_getTaskStatus(void (*userFunction)(void))
{
	uint8_t returnVar = GF_TASK_NOTFOUND;
	for (uint8_t i = 0; i < taskListCount; i++)
	{
		if (userFunc[i] == userFunction)
		{
			returnVar = taskInfo[i];
			break;
		}
	}
	return returnVar;
}

void gf_delaymsLong(uint16_t ms)
{
	for(uint16_t i=0;i<ms;i++)
	{
		_delay_ms(1);
	}
}

// Reverses a string 'str' of length 'len'
void _reverse(char* str, int len)
{
	uint8_t i = 0, j = len - 1, temp;
	while (i < j) {
		temp = str[i];
		str[i] = str[j];
		str[j] = temp;
		i++;
		j--;
	}
}