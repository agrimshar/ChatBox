#include "stdbool.h"
#include "stdlib.h"
#include "math.h"
#include "string.h"
#include "stdio.h"

/* GLOBAL REGISTER DEFINITIONS */
#define LED_BASE 0xFF200000
#define GPIO_BASE 0xFF200070
#define PS2_BASE 0xFF200100
#define PIXEL_BUFFER_BASE 0xFF203020
#define CHARACTER_BUFFER_BASE 0xFF203030

/* MISC DEFINITIONS */
#define BUFFER_SIZE 256
#define PS2_IRQ 7
#define GPIO_IRQ 12

/* GLOBAL IO POINTERS */
volatile int *const GPIO_PTR = (int *)GPIO_BASE;
volatile int *const PS2_PTR = (int *)PS2_BASE;
volatile int *const LED_PTR = (int *)LED_BASE;
volatile int *const PIXEL_PTR = (int *)PIXEL_BUFFER_BASE;
volatile int *const CHARACTER_PTR = (int *)CHARACTER_BUFFER_BASE;

/* GLOBAL STRUCTS */
// Defining struct for a message
struct Message
{
	char user_name[256];
	char message[256];
	int y_location;
};

// Defining struct for linked list
struct MessageNode
{
	struct Message message;
	struct Message *next;
};

/* PROGRAM GLOBAL VARIABLES */
char buffer[BUFFER_SIZE];
char my_user_name[BUFFER_SIZE];
volatile char received_buffer[BUFFER_SIZE];
volatile char connected_user_name[BUFFER_SIZE];

char last_pressed = 0;
char byte1 = 0;
char byte2 = 0;
char byte3 = 0;

int cursor_x = 0;
int cursor_y = 0;
int buffer_index = 0;
int scrollCounter = 0;
int messageCounter = 0;

volatile int conn = 0;
volatile int received_index = 0;
volatile int received_index_saver = 0;

bool cursor_toggle = 1;

struct Message messages[4 * BUFFER_SIZE];

/* INTERRUPT FUNCTION PROTOTYPES */
void the_reset(void) __attribute__((section(".reset")));
void the_exception(void) __attribute__((section(".exceptions")));

/* FUNCTION PROTOTYPES */
void plot_pixel(int, int, short int);
void clear_screen();
void swap(int *, int *);
void draw_line(int, int, int, int, short int);
void draw_cursor_colour();
void draw_cursor(int x, int y);
void draw_typing_border();
void draw_logged_in_border();
void write_char(int, int, char);
void clear_characters();
void write_word(int, int, char *);
void initial_setup();
void enter_delete_pressed();
void clean_display();
void enter_name();
void whos_logged_in();
void test_messages(struct MessageNode *head);
void detect_connection();
void insertMessage(struct MessageNode **head, struct Message m);
void printMessages(struct MessageNode *head);
void show_message(struct MessageNode *m, int counter);
void interrupt_handler(void);
void gpio_ISR(void);
void ps2_ISR(void);
void send_data_to_gpio(void);
char scanCodeDecoder(char scanCode);
char get_gpio_data(volatile int *GPIO_PTR);
struct MessageNode *createMessage(struct Message m);

/* INTERRUPT HANDLERS */
#define NIOS2_READ_STATUS(dest)    \
	do                             \
	{                              \
		dest = __builtin_rdctl(0); \
	} while (0)

#define NIOS2_WRITE_STATUS(src)  \
	do                           \
	{                            \
		__builtin_wrctl(0, src); \
	} while (0)

#define NIOS2_READ_ESTATUS(dest)   \
	do                             \
	{                              \
		dest = __builtin_rdctl(1); \
	} while (0)

#define NIOS2_READ_BSTATUS(dest)   \
	do                             \
	{                              \
		dest = __builtin_rdctl(2); \
	} while (0)

#define NIOS2_READ_IENABLE(dest)   \
	do                             \
	{                              \
		dest = __builtin_rdctl(3); \
	} while (0)

#define NIOS2_WRITE_IENABLE(src) \
	do                           \
	{                            \
		__builtin_wrctl(3, src); \
	} while (0)

#define NIOS2_READ_IPENDING(dest)  \
	do                             \
	{                              \
		dest = __builtin_rdctl(4); \
	} while (0)

#define NIOS2_READ_CPUID(dest)     \
	do                             \
	{                              \
		dest = __builtin_rdctl(5); \
	} while (0)

/* INTERRUPT FUNCTION DECLARATIONS */
void the_reset(void)
/*******************************************************************************
 * Reset code. By giving the code a section attribute with the name ".reset" we
 * allow the linker program to locate this code at the proper reset vector
 * address. This code just calls the main program.
 ******************************************************************************/
{
	asm(".set		noat");		/* Instruct the assembler NOT to use reg at (r1) as
								 * a temp register for performing optimizations */
	asm(".set		nobreak");	/* Suppresses a warning message that says that
								 * some debuggers corrupt regs bt (r25) and ba
								 * (r30)
								 */
	asm("movia		r2, main"); // Call the C language main program
	asm("jmp		r2");
}

void the_exception(void)
/*******************************************************************************
 * Exceptions code. By giving the code a section attribute with the name
 * ".exceptions" we allow the linker program to locate this code at the proper
 * exceptions vector address.
 * This code calls the interrupt handler and later returns from the exception.
 ******************************************************************************/
{
	asm("subi	sp, sp, 128");
	asm("stw	et, 96(sp)");
	asm("rdctl	et, ctl4");
	asm("beq	et, r0, SKIP_EA_DEC"); // Interrupt is not external
	asm("subi	ea, ea, 4");		   /* Must decrement ea by one instruction
										* for external interupts, so that the
										* interrupted instruction will be run */

	asm("SKIP_EA_DEC:");
	asm("stw	r1,  4(sp)"); // Save all registers
	asm("stw	r2,  8(sp)");
	asm("stw	r3,  12(sp)");
	asm("stw	r4,  16(sp)");
	asm("stw	r5,  20(sp)");
	asm("stw	r6,  24(sp)");
	asm("stw	r7,  28(sp)");
	asm("stw	r8,  32(sp)");
	asm("stw	r9,  36(sp)");
	asm("stw	r10, 40(sp)");
	asm("stw	r11, 44(sp)");
	asm("stw	r12, 48(sp)");
	asm("stw	r13, 52(sp)");
	asm("stw	r14, 56(sp)");
	asm("stw	r15, 60(sp)");
	asm("stw	r16, 64(sp)");
	asm("stw	r17, 68(sp)");
	asm("stw	r18, 72(sp)");
	asm("stw	r19, 76(sp)");
	asm("stw	r20, 80(sp)");
	asm("stw	r21, 84(sp)");
	asm("stw	r22, 88(sp)");
	asm("stw	r23, 92(sp)");
	asm("stw	r25, 100(sp)"); // r25 = bt (skip r24 = et, because it is saved
								// above)
	asm("stw	r26, 104(sp)"); // r26 = gp
	// skip r27 because it is sp, and there is no point in saving this
	asm("stw	r28, 112(sp)"); // r28 = fp
	asm("stw	r29, 116(sp)"); // r29 = ea
	asm("stw	r30, 120(sp)"); // r30 = ba
	asm("stw	r31, 124(sp)"); // r31 = ra
	asm("addi	fp,  sp, 128");

	asm("call	interrupt_handler"); // Call the C language interrupt handler

	asm("ldw	r1,  4(sp)"); // Restore all registers
	asm("ldw	r2,  8(sp)");
	asm("ldw	r3,  12(sp)");
	asm("ldw	r4,  16(sp)");
	asm("ldw	r5,  20(sp)");
	asm("ldw	r6,  24(sp)");
	asm("ldw	r7,  28(sp)");
	asm("ldw	r8,  32(sp)");
	asm("ldw	r9,  36(sp)");
	asm("ldw	r10, 40(sp)");
	asm("ldw	r11, 44(sp)");
	asm("ldw	r12, 48(sp)");
	asm("ldw	r13, 52(sp)");
	asm("ldw	r14, 56(sp)");
	asm("ldw	r15, 60(sp)");
	asm("ldw	r16, 64(sp)");
	asm("ldw	r17, 68(sp)");
	asm("ldw	r18, 72(sp)");
	asm("ldw	r19, 76(sp)");
	asm("ldw	r20, 80(sp)");
	asm("ldw	r21, 84(sp)");
	asm("ldw	r22, 88(sp)");
	asm("ldw	r23, 92(sp)");
	asm("ldw	r24, 96(sp)");
	asm("ldw	r25, 100(sp)"); // r25 = bt
	asm("ldw	r26, 104(sp)"); // r26 = gp
	// skip r27 because it is sp, and we did not save this on the stack
	asm("ldw	r28, 112(sp)"); // r28 = fp
	asm("ldw	r29, 116(sp)"); // r29 = ea
	asm("ldw	r30, 120(sp)"); // r30 = ba
	asm("ldw	r31, 124(sp)"); // r31 = ra

	asm("addi	sp,  sp, 128");

	asm("eret");
}

/* FUNCTION DEFINITIONS */
char scanCodeDecoder(char scanCode)
{
	// Getting the scan code and returning the ASCII value
	switch (scanCode)
	{
	case 0x76: // ESC
		return 0X1B;
		break;
	case 0X1C: // A
		return 0X41;
		break;
	case 0x32: // B
		return 0X42;
		break;
	case 0x21: // C
		return 0X43;
		break;
	case 0x23: // D
		return 0X44;
		break;
	case 0x24: // E
		return 0X45;
		break;
	case 0x2B: // F
		return 0X46;
		break;
	case 0x34: // G
		return 0X47;
		break;
	case 0x33: // H
		return 0X48;
		break;
	case 0x43: // I
		return 0X49;
		break;
	case 0x3B: // J
		return 0X4A;
		break;
	case 0x42: // K
		return 0X4B;
		break;
	case 0x4B: // L
		return 0X4C;
		break;
	case 0x3A: // M
		return 0X4D;
		break;
	case 0x31: // N
		return 0X4E;
		break;
	case 0x44: // O
		return 0X4F;
		break;
	case 0x4D: // P
		return 0X50;
		break;
	case 0x15: // Q
		return 0X51;
		break;
	case 0x2D: // R
		return 0X52;
		break;
	case 0x1B: // S
		return 0X53;
		break;
	case 0x2C: // T
		return 0X54;
		break;
	case 0x3C: // U
		return 0X55;
		break;
	case 0x2A: // V
		return 0X56;
		break;
	case 0x1D: // W
		return 0X57;
		break;
	case 0x22: // X
		return 0X58;
		break;
	case 0x35: // Y
		return 0X59;
		break;
	case 0x1A: // Z
		return 0X5A;
		break;
	case 0x45: // 0
		return 0X30;
		break;
	case 0x16: // 1
		return 0X31;
		break;
	case 0x1E: // 2
		return 0X32;
		break;
	case 0x26: // 3
		return 0X33;
		break;
	case 0x25: // 4
		return 0X34;
		break;
	case 0x2E: // 5
		return 0X35;
		break;
	case 0x36: // 6
		return 0X36;
		break;
	case 0x3D: // 7
		return 0X37;
		break;
	case 0x3E: // 8
		return 0X38;
		break;
	case 0x46: // 9
		return 0X39;
		break;
	case 0x66:
		return 0x08; // Backspace
		break;
	case 0x49: // Period
		return 0x2E;
		break;
	case 0x52: // Apostrophe
		return 0x27;
		break;
	case 0x41: // Comma
		return 0x2C;
		break;
	case 0x0E: // Backtick
		return 0x60;
		break;
	case 0x4E: // Minus
		return 0x2D;
		break;
	case 0x55: // Equals
		return 0x3D;
		break;
	case 0x54: // Left Bracket
		return 0x5B;
		break;
	case 0x5B: // Right Bracket
		return 0x5D;
		break;
	case 0x4C: // Semicolon
		return 0x3B;
		break;
	case 0x4A: // Slash
		return 0x2F;
		break;
	case 0x7C: // Star
		return 0x2A;
		break;
	case 0x5D: // Front Slash
		return 0x5C;
		break;
	case 0x79: // Plus
		return 0x2B;
		break;
	case 0x29: // Space
		return 0x20;
		break;
	case 0x5A: // Enter
		return 0x10;
		break;
	case 0xF0: // Break code
		break;
	default:
		break;
	}
}

char get_gpio_data(volatile int *GPIO_PTR)
{
	char data = *GPIO_PTR >> 8;
	return data & 0xFF;
}

void send_data_to_gpio(void)
{
	for (int i = 0; i < buffer_index; i++)
	{
		*GPIO_PTR = buffer[i];
		for (int j = 0; j < 270; j++)
		{
		}
	}

	buffer_index = 0; // Reset buffer index after sending
}

void gpio_ISR(void)
{
	char data;
	data = get_gpio_data(GPIO_PTR);
	received_buffer[received_index] = data;
	received_index++;

	if (received_index >= BUFFER_SIZE)
	{
		received_index_saver = received_index;
		received_index = 0; // Reset buffer index to avoid overflow
		*(volatile int *)(GPIO_BASE + 0x0C) = 0xFFFFFFFF;
	}
	else if (received_buffer[received_index - 1] == 0x10)
	{
		received_index_saver = received_index;
		received_index = 0; // Reset buffer index to avoid overflow
		*(volatile int *)(GPIO_BASE + 0x0C) = 0xFFFFFFFF;
	}
}

void ps2_ISR(void)
{
	// PS2 interrupt service routine
	int PS2_data, RVALID;
	PS2_data = *(PS2_PTR);
	RVALID = (PS2_data & 0x8000);

	if (RVALID)
	{
		byte1 = byte2;
		byte2 = byte3;
		byte3 = PS2_data & 0xFF;
		char key = byte1;
		if (key != 0)
		{ // Not a break code
			byte1 = 0, byte2 = 0, byte3 = 0;
			key = scanCodeDecoder(key);

			last_pressed = key;

			if (key == 0x08)
			{ // Backspace key pressed
				if (buffer_index > 0)
				{
					buffer_index--;
					buffer[buffer_index] = 0;
				}
			}
			else
			{
				buffer[buffer_index] = key;
				buffer_index++;
			}

			if (buffer_index >= BUFFER_SIZE)
			{
				buffer_index = 0; // Reset buffer index to avoid overflow
			}

			if (key == 0x10)
			{ // Enter key pressed
				send_data_to_gpio();
			}
		}
	}
}

void interrupt_handler(void)
{
	int ipending;
	NIOS2_READ_IPENDING(ipending);
	if (ipending & (1 << PS2_IRQ))
	{ // Check if PS2 interrupt
		ps2_ISR();
	}
	if (ipending & (1 << GPIO_IRQ))
	{ // Check if GPIO interrupt
		gpio_ISR();
		conn = 1;
	}
	// Handle other interrupts as needed
}

void plot_pixel(int x, int y, short int pixel_color)
{
	volatile short int *one_pixel_address;

	// set the address of the pixel to the buffer start plus its x and y coordinate
	one_pixel_address = *PIXEL_PTR + (y << 10) + (x << 1);

	// dereferencing the pixel address allows us to modify the pixel colour
	*one_pixel_address = pixel_color;
}

void clear_screen()
{
	for (int x = 0; x < 320; x++)
	{
		for (int y = 0; y < 240; y++)
		{
			plot_pixel(x, y, 0x0000);
		}
	}
}

void swap(int *x, int *y)
{
	int temp = *x;
	*x = *y;
	*y = temp;
}

// Using Bresenham's Algorithm to draw lines
void draw_line(int x0, int y0, int x1, int y1, short int line_color)
{
	bool is_steep = abs(y1 - y0) > abs(x1 - x0);

	if (is_steep)
	{
		swap(&x0, &y0);
		swap(&x1, &y1);
	}

	if (x0 > x1)
	{
		swap(&x0, &x1);
		swap(&y0, &y1);
	}

	int deltaX = x1 - x0;
	int deltaY = abs(y1 - y0);
	int error = -1 * (deltaX / 2);
	int y = y0;
	int y_step = 0;

	if (y0 < y1)
	{
		y_step = 1;
	}
	else
	{
		y_step = -1;
	}

	for (int x = x0; x <= x1; x++)
	{
		if (is_steep)
		{
			plot_pixel(y, x, line_color);
		}
		else
		{
			plot_pixel(x, y, line_color);
		}

		error = error + deltaY;

		if (error > 0)
		{
			y = y + y_step;
			error = error - deltaX;
		}
	}
}

void draw_cursor_colour()
{
	int cursor_colour;

	if (cursor_toggle == 0)
	{
		cursor_colour = 0x000;
	}
	else
	{
		cursor_colour = 0xFFFF;
	}

	draw_line(cursor_x, cursor_y, cursor_x, cursor_y + 10, cursor_colour);
	draw_line(cursor_x + 1, cursor_y, cursor_x + 1, cursor_y + 10, cursor_colour);
	draw_line(cursor_x + 2, cursor_y, cursor_x + 2, cursor_y + 10, cursor_colour);
	draw_line(cursor_x + 3, cursor_y, cursor_x + 3, cursor_y + 10, cursor_colour);
}

void draw_cursor(int x, int y)
{
	int cursor_colour;

	if (cursor_toggle == 0)
	{
		cursor_colour = 0x000;
	}
	else
	{
		cursor_colour = 0xFFFF;
	}

	draw_line(x, y, x, y + 10, cursor_colour);
	draw_line(x + 1, y, x + 1, y + 10, cursor_colour);
	draw_line(x + 2, y, x + 2, y + 10, cursor_colour);
	draw_line(x + 3, y, x + 3, y + 10, cursor_colour);
}

void draw_typing_border()
{
	draw_line(0, 216, 319, 216, 0xFFFF);
	draw_line(0, 217, 319, 217, 0xFFFF);
}

void draw_logged_in_border()
{
	draw_line(0, 20, 319, 20, 0xFFFF);
	draw_line(0, 21, 319, 21, 0xFFFF);
}

void write_char(int x, int y, char c)
{
	if (c == 0x10 || c == 0x08)
	{
		return;
	}
	volatile char *character_buffer = (char *)(*CHARACTER_PTR + (y << 7) + x);
	*character_buffer = c;
}

void clear_characters()
{
	// Character buffer x length is 80
	// 					y length is 60
	for (int x = 0; x < 80; x++)
	{
		for (int y = 0; y < 60; y++)
		{
			write_char(x, y, 0);
		}
	}
}

void write_word(int x, int y, char *word)
{
	while (*word)
	{
		write_char(x, y, *word);
		x++;
		word++;
	}
}

void initial_setup()
{
	clean_display();
	draw_cursor(cursor_x, cursor_y);
	draw_typing_border();
	draw_logged_in_border();
	write_word(2, 57, "Enter Message:");
	whos_logged_in();
}

void enter_delete_pressed()
{
	clear_characters();
	write_word(2, 57, "Enter Message:");
	whos_logged_in();
}

void clean_display()
{
	clear_screen();
	clear_characters();
}

void enter_name()
{
	clean_display();
	write_word(25, 30, "Enter Your Name:");

	// Loop until enter is pressed
	while (last_pressed != 0X10 || buffer[0] == 0x10)
	{
		int cursor_offset = buffer_index - 1;
		cursor_toggle = 0;
		draw_cursor(cursor_x + 4 * cursor_offset, cursor_y);

		if (last_pressed == 0x08)
		{
			clean_display();
			write_word(25, 30, "Enter Your Name:");
			draw_cursor(cursor_x + 4 * (cursor_offset + 2), cursor_y);
			last_pressed = -1;
		}

		write_word(43, 30, buffer);

		cursor_toggle = 1;
		draw_cursor(cursor_x + 4 * (cursor_offset + 1), cursor_y);
	}

	last_pressed = -1;
	strcpy(my_user_name, buffer);

	// Delay for visual effect
	for (int i = 0; i < 1000000; i++)
	{
	}
}

void whos_logged_in()
{
	char logged_in_text[256] = "Logged in as ";
	char talking_to_text[256] = "Talking to ";

	// Concatinating user names to display messages for VGA
	strcat(logged_in_text, my_user_name);
	strcat(talking_to_text, (char *)connected_user_name);

	write_word(2, 2, talking_to_text);
	write_word(45, 2, logged_in_text);
}

void test_messages(struct MessageNode *head)
{
	strcpy(messages[0].user_name, "Agrim");
	strcpy(messages[0].message, "hi");
	insertMessage(&head, messages[0]);
	scrollCounter++;
	strcpy(messages[1].user_name, "balls");
	strcpy(messages[1].message, "balls");
	insertMessage(&head, messages[1]);
	scrollCounter++;
	strcpy(messages[2].user_name, "lol");
	strcpy(messages[2].message, "lol");
	insertMessage(&head, messages[2]);
	scrollCounter++;
	strcpy(messages[3].user_name, "balls");
	strcpy(messages[3].message, "lol");
	insertMessage(&head, messages[3]);
	scrollCounter++;
	strcpy(messages[4].user_name, "balls");
	strcpy(messages[4].message, "balls");
	insertMessage(&head, messages[4]);
	scrollCounter++;
	strcpy(messages[5].user_name, "balls");
	strcpy(messages[5].message, "balls");
	insertMessage(&head, messages[5]);
	scrollCounter++;
	strcpy(messages[6].user_name, "balls");
	strcpy(messages[6].message, "balls");
	insertMessage(&head, messages[6]);
	scrollCounter++;
	strcpy(messages[7].user_name, "balls");
	strcpy(messages[7].message, "balls");
	insertMessage(&head, messages[7]);
	scrollCounter++;
	strcpy(messages[8].user_name, "balls");
	strcpy(messages[8].message, "balls");
	insertMessage(&head, messages[8]);
	scrollCounter++;
	strcpy(messages[9].user_name, "balls");
	strcpy(messages[9].message, "balls");
	insertMessage(&head, messages[9]);
	scrollCounter++;
	strcpy(messages[10].user_name, "balls");
	strcpy(messages[10].message, "balls");
	insertMessage(&head, messages[10]);
	scrollCounter++;
	strcpy(messages[11].user_name, "balls");
	strcpy(messages[11].message, "balls");
	insertMessage(&head, messages[11]);
	scrollCounter++;
	strcpy(messages[12].user_name, "balls");
	strcpy(messages[12].message, "balls");
	insertMessage(&head, messages[12]);
	scrollCounter++;
	strcpy(messages[13].user_name, "balls");
	strcpy(messages[13].message, "balls");
	insertMessage(&head, messages[13]);
	scrollCounter++;
	strcpy(messages[14].user_name, "balls");
	strcpy(messages[14].message, "balls");
	insertMessage(&head, messages[14]);
	scrollCounter++;
	strcpy(messages[15].user_name, "balls");
	strcpy(messages[15].message, "balls");
	insertMessage(&head, messages[15]);
	scrollCounter++;
	strcpy(messages[16].user_name, "balls");
	strcpy(messages[16].message, "balls");
	insertMessage(&head, messages[16]);
	scrollCounter++;
	strcpy(messages[17].user_name, "balls");
	strcpy(messages[17].message, "balls");
	insertMessage(&head, messages[17]);
	scrollCounter++;
	strcpy(messages[18].user_name, "balls");
	strcpy(messages[18].message, "balls");
	insertMessage(&head, messages[18]);
	scrollCounter++;
	strcpy(messages[19].user_name, "balls");
	strcpy(messages[19].message, "balls");
	insertMessage(&head, messages[19]);
	scrollCounter++;
	strcpy(messages[20].user_name, "balls");
	strcpy(messages[20].message, "balls");
	insertMessage(&head, messages[20]);
	scrollCounter++;
	strcpy(messages[21].user_name, "balls");
	strcpy(messages[21].message, "balls");
	insertMessage(&head, messages[21]);
	scrollCounter++;
	strcpy(messages[22].user_name, "balls");
	strcpy(messages[22].message, "balls");
	insertMessage(&head, messages[22]);
	scrollCounter++;
	strcpy(messages[23].user_name, "balls");
	strcpy(messages[23].message, "balls");
	insertMessage(&head, messages[23]);
	scrollCounter++;
	strcpy(messages[24].user_name, "balls");
	strcpy(messages[24].message, "balls");
	insertMessage(&head, messages[24]);
	scrollCounter++;
	strcpy(messages[25].user_name, "balls");
	strcpy(messages[25].message, "balls");
	insertMessage(&head, messages[25]);
	scrollCounter++;
	strcpy(messages[26].user_name, "balls");
	strcpy(messages[26].message, "balls");
	insertMessage(&head, messages[26]);
	scrollCounter++;
	strcpy(messages[27].user_name, "balls");
	strcpy(messages[27].message, "balls");
	insertMessage(&head, messages[27]);
	scrollCounter++;
	strcpy(messages[28].user_name, "balls");
	strcpy(messages[28].message, "balls");
	insertMessage(&head, messages[28]);
	scrollCounter++;
	strcpy(messages[29].user_name, "balls");
	strcpy(messages[29].message, "balls");
	insertMessage(&head, messages[29]);
	scrollCounter++;
	strcpy(messages[30].user_name, "balls");
	strcpy(messages[30].message, "balls");
	insertMessage(&head, messages[30]);
	scrollCounter++;
	printMessages(head);

	// write_word(2, 49, "Connection One >> Message One");
	// write_word(2, 51, "Connection Two >> Message Two");
}

void detect_connection()
{
	clean_display();

	// All of this should be in a while loop waiting until GPIO is detected
	write_word(25, 30, "Waiting for a connection...");

	while (conn == 0)
	{
	}

	for (int i = 0; i < received_index_saver; i++)
	{
		connected_user_name[i] = received_buffer[i];
	}

	// Delay for visual effect
	for (int i = 0; i < 10000000; i++)
	{
	}

	char connected_text[BUFFER_SIZE] = "You are talking to ";
	strcat(connected_text, (char *)connected_user_name);

	write_word(25, 30, connected_text);

	for (int i = 0; i < 20000000; i++)
	{
	}
}

struct MessageNode *createMessage(struct Message m)
{
	struct MessageNode *newMessage = (struct MessageNode *)malloc(sizeof(struct MessageNode));
	if (newMessage == NULL)
	{
		printf("Memory allocation failed.\n");
		exit(1);
	}
	newMessage->message = m;
	newMessage->next = NULL;
	return newMessage;
}

void insertMessage(struct MessageNode **head, struct Message m)
{
	struct MessageNode *newNode = createMessage(m);
	newNode->next = *head;
	*head = newNode;
}

void printMessages(struct MessageNode *head)
{
	// initial_setup();
	// whos_logged_in();
	struct MessageNode *current = head;
	int i = 0;
	while (current != NULL && i < scrollCounter)
	{
		show_message(current, i);
		i++;
		current = current->next;
	}
}

void show_message(struct MessageNode *m, int counter)
{
	int spacing = 51 - (counter * 2);
	if (spacing > 51 || spacing < 7)
	{
		return;
	}
	char message[600] = "";
	strcat(message, m->message.user_name);
	strcat(message, " >> ");
	strcat(message, m->message.message);
	write_word(2, spacing, message);
}

/* PROGRAM STARTS HERE */
int main(void)
{

	// Clean the display
	clean_display();

	*(volatile int *)(GPIO_BASE + 0x04) = 0xFF; // Configure GPIO direction as needed
	unsigned int ienable = (1 << PS2_IRQ) | (1 << GPIO_IRQ);
	*(volatile int *)(PS2_BASE + 0x04) |= 0x1; // Configure PS2 as needed
	NIOS2_WRITE_IENABLE(ienable);
	NIOS2_WRITE_STATUS(1); // Enable Nios II interrupts
	*(volatile int *)(GPIO_BASE + 0x08) |= 0xFF00;

	// setting current cursor position
	cursor_toggle = 1;
	cursor_x = 172;
	cursor_y = 116;

	// Enter your name
	enter_name();

	// Finding connection between
	// If GPIO is detected, show connected
	// need function here to detect that
	// set the connected person name
	cursor_toggle = 0;
	draw_cursor_colour();
	detect_connection();

	// setting current cursor position
	cursor_toggle = 1;
	cursor_x = 64;
	cursor_y = 224;

	// Clearing screen and drawing borders/cursor
	initial_setup();

	// Testing messages
	struct MessageNode *head = NULL;
	// printf("%d\n", head);
	// test_messages(head);
	// printf("%d\n", head);

	last_pressed = -1;
	memset(buffer, 0, BUFFER_SIZE);
	memset((char *)received_buffer, 0, BUFFER_SIZE);
	while (1)
	{
		int cursor_offset = buffer_index;

		cursor_toggle = 0;
		draw_cursor(cursor_x + 4 * cursor_offset, cursor_y);

		// If a message is received or entered, display it
		if (received_buffer[received_index_saver - 1] == 0x10)
		{
			strcpy(messages[messageCounter].user_name, (char *)connected_user_name);
			strcpy(messages[messageCounter].message, (char *)received_buffer);
			insertMessage(&head, messages[messageCounter]);
			scrollCounter++;
			messageCounter++;
			received_index_saver = -1; // not sure what this does
			memset((char *)received_buffer, 0, BUFFER_SIZE);
			initial_setup();
		}
		else if (last_pressed == 0x10 && buffer[0] != 0x10)
		{
			strcpy(messages[messageCounter].user_name, my_user_name);
			strcpy(messages[messageCounter].message, buffer);
			insertMessage(&head, messages[messageCounter]);
			scrollCounter++;
			messageCounter++;
			initial_setup();
			last_pressed = -1;
			memset(buffer, 0, BUFFER_SIZE);
		}
		else if (last_pressed == 0x08)
		{
			initial_setup();
			draw_cursor(cursor_x + 4 * (cursor_offset + 2), cursor_y);
			last_pressed = -1;
		}

		write_word(17, 57, buffer);

		cursor_toggle = 1;
		draw_cursor(cursor_x + 4 * (cursor_offset + 1), cursor_y);
		printMessages(head);
	}

	// if up arrow is pressed, scrollCounter++, else scrollCounter--
	// main loop will be a while loop that waits for keyboard input
	// if no keyboard input the cursor will blink
	// when there is a keyboard input the cursor will toggle to white
}
