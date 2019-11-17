#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "mach_defines.h"
#include "sdk.h"
#include "gfx_load.h"
#include "cache.h"
#include "badgetime.h"

//Pointer to the framebuffer memory.
uint8_t *fbmem;

#define FB_WIDTH 512
#define FB_HEIGHT 320

#define R1 (0b11000010101)
#define R2 (0b00110000000)
#define R3 (0b00001101010)

#define G1 (0b00110000000)
#define G2 (0b11001111111)
#define G3 (0b00000000000)

#define B1 (0b00001101010)
#define B2 (0b00000000000)
#define B3 (0b11110010101)


void main(int argc, char **argv) {
	//We're running in app context. We have full control over the badge and can do with the hardware what we want. As
	//soon as main() returns, however, we will go back to the IPL.
	printf("Hello World app: main running\n");

	//Blank out fb while we're loading stuff by disabling all layers. This just shows the background color.
	GFX_REG(GFX_BGNDCOL_REG)=0x202020; //a soft gray
	GFX_REG(GFX_LAYEREN_REG)=0; //disable all gfx layers

	//First, allocate some memory for the background framebuffer. We're gonna dump a fancy image into it. The image is
	//going to be 8-bit, so we allocate 1 byte per pixel.
	fbmem=calloc(FB_WIDTH,FB_HEIGHT);
	printf("Hello World: framebuffer at %p\n", fbmem);

	//Tell the GFX hardware to use this, and its pitch. We also tell the GFX hardware to use palette entries starting
	//from 128 for the frame buffer; the tiles left by the IPL will use palette entries 0-16 already.
	GFX_REG(GFX_FBPITCH_REG)=(128<<GFX_FBPITCH_PAL_OFF)|(FB_WIDTH<<GFX_FBPITCH_PITCH_OFF);
	//Set up the framebuffer address
	GFX_REG(GFX_FBADDR_REG)=((uint32_t)fbmem);

	//Flush the memory region to psram so the GFX hw can stream it from there.
	cache_flush(fbmem, fbmem+FB_WIDTH*FB_HEIGHT);

	//The IPL leaves us with a tileset that has tile 0 to 127 map to ASCII characters, so we do not need to
	//load anything specific for this.

	//In order to get some text out, we can use the /dev/console device
	//that will use these tiles to put text in a tilemap. It uses escape codes to do so, see
	//ipl/gloss/console_out.c for more info.
	//Note that this overwrites bits of the tilemap, as it's drawn on the same layer.
	FILE *f;
	f=fopen("/dev/console", "w");
	setvbuf(f, NULL, _IONBF, 0); //make console line unbuffered
	//Note that without the setvbuf command, no characters would be printed until 1024 characters are
	//buffered. You normally don't want this.
	//The next line would clear the console, removing the tilemap we loaded earlier. It's commented
	//out as we don't want to lose that.
	//fprintf(f, "\033C"); //clear the console. Note '\033' is the escape character.
	fprintf(f, "\0332X"); //set Xpos to 5
	fprintf(f, "\0335Y"); //set Ypos to 15
	fprintf(f, "Hello World!"); // Print a nice greeting.

	//The user can still see nothing of this graphics goodness, so let's re-enable the framebuffer and
	//tile layer A (the default layer for the console). Also indicate the framebuffer we have is
	//8-bit.
	GFX_REG(GFX_LAYEREN_REG)=GFX_LAYEREN_FB_8BIT|GFX_LAYEREN_FB|GFX_LAYEREN_TILEA;

	printf("Hello World ready. Press a button to exit.\n");
	//Wait until all buttons are released
	wait_for_button_release();

	uint8_t red = 0x00;
	uint8_t green = 0x55;
	uint8_t blue = 0xAA;

	int8_t rc = 1;
	int8_t gc = 1;
	int8_t bc = 1;

	while((MISC_REG(MISC_BTN_REG) & BUTTON_A)==0) {
		MISC_REG(MISC_LED_COL_REG) = 0b001;
		MISC_REG(MISC_LED_REG) = 0x07FF;
		for (int i = 0; i < 256; i++) {
			if (i >= red) MISC_REG(MISC_LED_REG) &= ~R1;
			if (i >= green) MISC_REG(MISC_LED_REG) &= ~G1;
			if (i >= blue) MISC_REG(MISC_LED_REG) &= ~B1;
		}
		delay(1);

		MISC_REG(MISC_LED_COL_REG) = 0b010;
		MISC_REG(MISC_LED_REG) = 0x07FF;
		for (int i = 0; i < 256; i++) {
			if (i >= red) MISC_REG(MISC_LED_REG) &= ~R2;
			if (i >= green) MISC_REG(MISC_LED_REG) &= ~G2;
		}

		delay(1);

		MISC_REG(MISC_LED_COL_REG) = 0b100;
		MISC_REG(MISC_LED_REG) = 0x07FF;
		for (int i = 0; i < 256; i++) {
			if (i >= red) MISC_REG(MISC_LED_REG) &= ~R3;
			if (i >= blue) MISC_REG(MISC_LED_REG) &= ~B3;
		}
		delay(1);
		red += rc;
		if (red == 0) rc *= -1;
		green += gc;
		if (green == 0) gc *= -1;
		blue += bc;
		if (blue == 0) bc *= -1;
	}
	printf("Hello World done. Bye!\n");
}
