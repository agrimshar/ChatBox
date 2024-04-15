# ChatBox
The foundation for a basic chat application using hardware interrupts and GPIO communication on DE1-SoC boards.

https://github.com/agrimshar/ChatBox/assets/55661784/57b8f449-ee09-4345-8036-91fcdd255c6c

This code implements a chatbox application for communication between two DE1-SoCs using interrupts and GPIO registers. Here's a breakdown of its main components:

1. Global Definitions and Pointers: Defines register addresses for GPIO, PS2, LED, pixel buffer, and character buffer. Also initializes pointers to these memory locations.
2. Structures: Defines two structures: Message for holding user messages and MessageNode for creating a linked list of messages.
3. Global Variables: Defines various global variables including buffers, cursor position, message counters, and flags.
4. Interrupt Handlers: Implements interrupt handlers for PS2 and GPIO interrupts. PS2 ISR decodes PS2 scan codes into ASCII characters, while GPIO ISR reads data from GPIO and stores it in a buffer.
5. Drawing Functions: Implements functions for plotting pixels, drawing lines, drawing cursor, and writing characters to VGA display.
6. Initialization and Setup: Initializes the display and sets up the initial cursor position. It also prompts the user to enter their name.
7. Message Handling Functions: Includes functions for inserting messages into a linked list, printing messages on the display, and testing message insertion and display.
8. Connection Establishment: Detects connection between the two DE1-SoCs using GPIO interrupts.
9. Main Function: Initializes GPIO and PS2, enables interrupts, sets up the initial cursor position, prompts the user to enter their name, and detects connection between devices.
