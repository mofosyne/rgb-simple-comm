/**
  Title: RGB Simple Communication
  Author: Brian Khuu
  Website: briankhuu.com
  Date: 28th August 2016
  Description:
    The concept is to transmit data via LED in the simplest method possible.
    The target situation is a smartphone pointed at an LED.

    The self imposed challenge is:
      * No external clocking : The issue with having an led dedicated to the
        clock signal is that the state transition between data bits is at least
        twice. It would be better to embed the clocking in the data.
        This was acheived by having the encoding and decoding algorithm
        send every half nibble, encoded as a transition of one colour state to
        another. This makes this communication system stateful, as it relies on
        the previous color state as well current colour to decide what it had
        received.
      * Timing insensitive : The hardware recording the colour changes like
        a smartphone camera should not be relied upon to be timing accurate.
        Also by making the system timing insensitive, it makes
        implementation much much easier even in slow hardware.
      * LED is either fully on or off : By keeping the requirement simple
        it would allow the seqence player to play almost on any hardware with
        at least 3 GPIO pins to have this transmission output method.
        It also makes detection via RGB sensor much easier as well,
        since colour levels detection accuracy is not needed as much.
      * If the red green and blue leds are off, then the communication
        channel is down.

    What was acheived is a working algorithm for transmission of variable
    length bytes. In this implementation if the output colour is dark, then
    the channel is closed. The channel is online if at least one led is active.
    Every 2bits is represented as a transition between two data colour state.
    This leaves just two more states, yellow and white. These two colour were
    named Mark 1 and Mark 2, this can be customised for any particular purpose.
    (But in the example, it is useds as a character mark and optionally as a
    parity bit)

  Optional Objectives:
    * Would be good to implement a bit of parity checking
    * Package this as a libary for ready use in anything.
    * Write an android program that can visually decode such signals.
    * Or maybe a hardware with colour sensor that can decode such signals.

  Application:
    Honestly there is not too much use for something like this, considering
    that there are already bluetooth.
    - As a slower alternative to UART... really?
    - Debug output? You might not have a UART engine in your mcu, but may have
      at least 3 pins to spare.


  This is this table showing the LED colours and the meaning assigned to each state in this algo

  ```
    | COLOUR   | R | G | B | Description                                                                |
    |----------|---|---|---|----------------------------------------------------------------------------|
    | DARK     | 0 | 0 | 0 | - Channel Off                                                              |
    | BLUE     | 0 | 0 | 1 | 2bit Data Colour State 0                                                   |
    | GREEN    | 0 | 1 | 0 | 2bit Data Colour State 1                                                   |
    | CYAN     | 0 | 1 | 1 | 2bit Data Colour State 2                                                   |
    | RED      | 1 | 0 | 0 | 2bit Data Colour State 3                                                   |
    | MAGENTA  | 1 | 0 | 1 | 2bit Data Colour State 4                                                   |
    | YELLOW   | 1 | 1 | 0 | - Mark 2 (Mark stderr ?) (Mark&Parity Value 1 ?) (bitmap mode: vsync ?)      |
    | WHITE    | 1 | 1 | 1 | - Mark 1 (Mark stdout ?) (Mark&Parity Value 0 ?) (bitmap mode: hsync ?)      |
  ```

*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>

// Helper macros for setting bitflags
#define GETBIT( VARIABLE, BITPOS )                 ( VARIABLE   &  (1u<<BITPOS) )
#define SETBIT( VARIABLE, BITPOS )                   VARIABLE   |=  (1u<<BITPOS)
#define CLRBIT( VARIABLE, BITPOS )                   VARIABLE   &= ~(1u<<BITPOS)
#define TOGBIT( VARIABLE, BITPOS )                   VARIABLE   ^=  (1u<<BITPOS)

#define PARITY_SETTING ODD_PARITY




typedef enum rgb_colour {
  DARK = 0,
  BLUE = 1,
  GREEN = 2,
  CYAN = 3,
  RED = 4,
  MAGENTA = 5,
  YELLOW = 6,
  WHITE = 7
} rgb_colour_t;


/*
  PARITY
*/
typedef enum paritySel {
  NO_PARITY,
  EVEN_PARITY,
  ODD_PARITY
} paritySel_t;

uint8_t calcParity_u8bit (uint8_t data, paritySel_t paritySelect ) {
  uint8_t parity = 0;
  // Parity Calc
  while (data) {
    parity ^= (data & 0x01);
    data = data >> 1;
  }
  switch (paritySelect) {
  case (EVEN_PARITY): // Even number of `1` means parity bit of 0 ; Odd number of `1` means parity bit of 1 ;
    return parity & 0x01;
  case (ODD_PARITY): //  Even number of `1` means parity bit of 1 ; Odd number of `1` means parity bit of 0 ;
    return (~parity) & 0x01;
  case (NO_PARITY):
    return 0x01;
  }
  return 0; // Should not be reached
}

uint8_t validParity_u8bit (uint8_t data, uint8_t parity_bit, paritySel_t paritySelect ) {
  uint8_t parity = calcParity_u8bit(data, paritySelect);
  return (parity & 0x01) == (parity_bit & 0x01);
}



/*
  ENCODE
*/

rgb_colour_t nextColourSeq_from_2bit (uint8_t halfnibble, rgb_colour_t previous_colour) {
  unsigned int offset = 0;

  // Guard
  halfnibble = halfnibble & 0x03;

  // Find Offset: Cannot repeat colour when sending every 2 bits of data (Since this is clocked by each colour transition)
  switch (previous_colour) {
  case (DARK):  // Channel Offline
    offset = 0;
    break;
  case (BLUE):
    offset = 1;
    break;
  case (GREEN):
    offset = 2;
    break;
  case (CYAN):
    offset = 3;
    break;
  case (RED):
    offset = 4;
    break;
  case (MAGENTA):
    offset = 5;
    break;
  case (YELLOW):
    offset = 0;
    break;
  case (WHITE):
    offset = 0;
    break;
  }

  // 5 colour state for transmitting 2bits without seperate clock.
  rgb_colour_t halfByteColour[5] = {
    BLUE,
    GREEN,
    CYAN,
    RED,
    MAGENTA
  };


  // Returns the next colour in seqence
  return halfByteColour[ (halfnibble + offset) % 5 ];
}


void toColourSeq_uint8 (const uint8_t data, rgb_colour_t colourSeq[100], int *seq_ptr) {
  int j = *seq_ptr;
  rgb_colour_t previous_colour;

  previous_colour = (j == 0) ? DARK : colourSeq[j - 1];


  // Data
  for (int i = 0 ; i < 4; i++) { // 4 data seq + 1 mark seq
    uint8_t half_nibble = (data >> 2 * (3 - i)) & 0x3; // next 2 bits

    colourSeq[j] = nextColourSeq_from_2bit(half_nibble, previous_colour);

    previous_colour = colourSeq[j];
    j++;
  }

  // Mark End of Word (And also include parity bit)
  if ( (calcParity_u8bit(data, PARITY_SETTING ) == 0 ) || PARITY_SETTING == NO_PARITY ) { //1
    colourSeq[j] = WHITE; // parity = 0
  } else {
    colourSeq[j] = YELLOW; // parity = 1
  }

  j++;

  // Return Values
  *seq_ptr = j;
  return;
}

/*
  DECODE
*/

/**

  Return Values:
    0 - succesful return of value
    1 - mark 1
    2 - mark 2
*/
int nextColourSeq_to_2bit (const rgb_colour_t incoming_colour, const rgb_colour_t previous_colour, uint8_t *halfnibble_out) {
  if (incoming_colour == previous_colour) {
    return -1; // Indicate that the channel is iding
  }
  // Find Offset: Cannot repeat colour when sending every 2 bits of data (Since this is clocked by each colour transition)
  uint8_t offset = 0;
  switch (previous_colour) {
  case (DARK):  // Channel Offline
    offset = 0;
    break;
  case (BLUE):
    offset = 1;
    break;
  case (GREEN):
    offset = 2;
    break;
  case (CYAN):
    offset = 3;
    break;
  case (RED):
    offset = 4;
    break;
  case (MAGENTA):
    offset = 5;
    break;
  case (YELLOW):
    offset = 0;
    break;
  case (WHITE):
    offset = 0;
    break;
  }

  // Corresponding Nibble Base
  uint8_t nibblebase = 0;
  switch (incoming_colour) {
  case (DARK): //0
    return -2; // Channel Going Down
    break;
  case (BLUE): //1
    nibblebase = 0;
    break;
  case (GREEN): //2
    nibblebase = 1;
    break;
  case (CYAN): //3
    nibblebase = 2;
    break;
  case (RED): //4
    nibblebase = 3;
    break;
  case (MAGENTA): //5
    nibblebase = 4;
    break;
  case (YELLOW): //6
    return 2; // Mark 2
    break;
  case (WHITE): //7
    return 1; // Mark 1
    break;
  }

  uint8_t halfnibble;

  /*
  This check is required because of the effect of subtraction operation result going below zero in an unsigned variable.
  */
  //if (incoming_colour > previous_colour) {
  if (nibblebase >= offset) {
    halfnibble = (nibblebase - offset) % 4;
  } else {
    halfnibble = (5 + nibblebase - offset) % 4;
  }



  // Return Nibble
  *halfnibble_out = halfnibble & 0x03;

  return 0; // Sucessfully returned a nibble
}


int fromColourSeq_get_uint8 (const rgb_colour_t colourSeq[100], int *seq_ptr, uint8_t *output ) {
  rgb_colour_t previous_colour;
  rgb_colour_t incoming_colour;

  previous_colour = ((*seq_ptr) == 0) ? DARK : colourSeq[ (*seq_ptr) - 1];

  // Communication Line is Closed
  rgb_colour_t seqPeek = colourSeq[(*seq_ptr)];
  if ( seqPeek == DARK ){
    return -1;
  }

  // Extract next 8bit from stream
  *output = 0x00;
  for (int i = 0 ; i < 5 ; i++) {
    uint8_t halfnibble_out;

    incoming_colour = colourSeq[(*seq_ptr)];
    int return_code = nextColourSeq_to_2bit ( incoming_colour, previous_colour, &halfnibble_out);
    previous_colour = incoming_colour;

    (*seq_ptr)++;

    switch (return_code) {
    case (2): // Mark 2
      return 2; // Yellow: Sucessfully Received a Byte (Parity bit = 1)
      break;
    case (1): // Mark 1
      return 1; // White: Sucessfully Received a Byte (Parity bit = 0)
      break;
    case (0): // Shift in half a nibble
      *output = *output << 2; // Shift by half nibble
      *output |= halfnibble_out;
      break;
    case (-1): // idling line keep scanning
      break;
    case (-2): // Communication Closed Unexpectedly
      return -1;
      break;
    }

  }
  return 0; // Only supports up to 8bit
}


/*
  TEST TOOLS
*/

// Colour String
static char *rgb_colour_str[] = {
  "Dark",
  "Blue",
  "Green",
  "Cyan",
  "Red",
  "Magenta",
  "Yellow",
  "White"
};

// Colour Short Version
static char *rgb_colour_short_str[] = {
  "D",
  "B",
  "G",
  "C",
  "R",
  "M",
  "Y",
  "W"
};

// Colour Short Version Marked
static char *rgb_colour_short_marked_str[] = {
  "_D_",
  " B ",
  " G ",
  " C ",
  " R ",
  " M ",
  "!Y!",
  "|W|"
};



uint8_t displayBinary_uint8_t(uint8_t input) {
  printf(" %x '%c' = ", input, input);
  int i;
  for (i = 8 - 1; i >= 0 ; i--) {
    if ( (input >> i) & 1u ) {
      printf("1");
    } else {
      printf("0");
    }
  }
  printf("\n");
  return input;
}

int main( void )
{
  printf("Colour Seq Test\n===============\n");

  printf("# Nibble ENCODING & DECODING Test\n");
  uint8_t halfnibble_input;
  uint8_t halfnibble_output;
  rgb_colour_t colour_curr;
  rgb_colour_t colour_prev;
  for (int j = 0 ; j < 8 ; j++) {
    colour_prev = j;
    printf("\n> colour prev = %d;\n", colour_prev);
    for (int i = 0 ; i < 0x04 ; i++ ) {
      halfnibble_input = i;
      colour_curr = nextColourSeq_from_2bit(halfnibble_input, colour_prev);
      nextColourSeq_to_2bit(colour_curr, colour_prev, &halfnibble_output);
      printf("%1d %1d | halfnibble in = %x, out = %x ; colour curr = %d, prev = %d;\n", halfnibble_input == halfnibble_output, colour_curr != colour_prev, halfnibble_input, halfnibble_output, colour_curr, colour_prev);
    }
  }



  // This simulates a stream of colours (Assumption: initial prev colour was DARK for channel closed)
  rgb_colour_t colourSeq[100];
  memset(colourSeq, 0x00, sizeof(rgb_colour_t) * 100);

  int j = 0;

  printf("\n\n");

  printf("# ENCODING Input\n");
  toColourSeq_uint8(displayBinary_uint8_t('H'), colourSeq, &j);
  toColourSeq_uint8(displayBinary_uint8_t('E'), colourSeq, &j);
  toColourSeq_uint8(displayBinary_uint8_t('L'), colourSeq, &j);
  toColourSeq_uint8(displayBinary_uint8_t('L'), colourSeq, &j);
  toColourSeq_uint8(displayBinary_uint8_t('O'), colourSeq, &j);
  toColourSeq_uint8(displayBinary_uint8_t(' '), colourSeq, &j);
  toColourSeq_uint8(displayBinary_uint8_t('W'), colourSeq, &j);
  toColourSeq_uint8(displayBinary_uint8_t('O'), colourSeq, &j);
  toColourSeq_uint8(displayBinary_uint8_t('R'), colourSeq, &j);
  toColourSeq_uint8(displayBinary_uint8_t('L'), colourSeq, &j);
  toColourSeq_uint8(displayBinary_uint8_t('D'), colourSeq, &j);
  toColourSeq_uint8(displayBinary_uint8_t('.'), colourSeq, &j);
  toColourSeq_uint8(displayBinary_uint8_t('.'), colourSeq, &j);
  toColourSeq_uint8(displayBinary_uint8_t('.'), colourSeq, &j);
  toColourSeq_uint8(displayBinary_uint8_t(' '), colourSeq, &j);
  toColourSeq_uint8(displayBinary_uint8_t(' '), colourSeq, &j);
  toColourSeq_uint8(displayBinary_uint8_t(' '), colourSeq, &j);
  toColourSeq_uint8(displayBinary_uint8_t(' '), colourSeq, &j);
  printf("\n\n# Encoded Colour Seqence Output\n");
  for (int i; i < 100 ; i++) {
    printf("%d:%s ", i, rgb_colour_str[colourSeq[i]] );
  }
  printf("\n\n# Encoded Colour Seqence Output (Compact)\n");
  for (int i; i < 100 ; i++) {
    printf("%s", rgb_colour_short_str[colourSeq[i]] );
  }
  printf("\n\n# Encoded Colour Seqence Output (marked)\n");
  for (int i; i < 100 ; i++) {
    printf("%s", rgb_colour_short_marked_str[colourSeq[i]] );
  }


  printf("\n\n");


  printf("# DECODING Test\n");
  int k = 0;
  uint8_t output_byte;

  //printf("%2d k=%2d 0x%.2X '%c' \n", fromColourSeq_get_uint8(colourSeq, &k, &output_byte ), k, output_byte, output_byte );

  for (int i; i < 40 ; i++) {
    int returncode = fromColourSeq_get_uint8(colourSeq, &k, &output_byte );
    if (returncode >=0) {
      printf("%c", output_byte);
      // Received Parity
      uint8_t parity_bit;
      switch (returncode) {
      case (1): // White parity=0
        parity_bit = 0x00;
        break;
      case (2): // Yellow parity=1
        parity_bit = 0x01;
        break;
      }
      // Mark End of Word (And also include parity bit)
      if ( !validParity_u8bit(output_byte, parity_bit, PARITY_SETTING) ) {
        //printf("! %d %d %d", output_byte, parity_bit, k);
        printf("! ");
      }
      //printf("\n %d %d %d \n", output_byte, parity, parity_bit);
    } else {
      printf(" [END OF TRANSMISSION] ");
      fflush(stdout);
      break;
    }
    //  printf("X:%d",i);
  }

  printf("\n\n# Completed\n");
  return 0;
}
