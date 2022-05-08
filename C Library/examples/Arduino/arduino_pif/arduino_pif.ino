/* Arduino PIF Image Demo
 *  
 *  This little arduino sketch should give anyone a rough idea how the PIF image format can be used
 *  Two images will be shown on the display, first the arduino-logo in full screen with a small HackADay logo on top of it!
 *  
 *  This demo for the original arduino uno is quite a tight fit, didn't expect the arduino display library eat half the AVR's memory!
 */

#include <MCUFRIEND_kbv.h>  // 3.5" ILI9486 Display (320 x 480px, 16bpp) - Howly Mowly how is this using half of the AVR's memory?!

extern "C"
{
  #include "pifdec.h"         // PIF Image Decoder
}

#include "arduino.h"  // Arduino Image, 2 Indexed Colors, 16bpp
#include "hackaday.h" // Hackaday Logo, Monochrome

MCUFRIEND_kbv tft;

/* Drawing, IO and PIF Handler */
pifPAINT_t pifPaintingStruct;
pifIO_t pifFileIOStruct;
pifHANDLE_t pifHandler;
uint8_t colorLookupTable[16]; // Array to buffer indexed colors

/* Drawing Functions */
uint8_t first;  // Needed for the TFT-pushColors
int8_t displayPrepare(void *p_Disp, pifINFO_t *p_Info)
{
  // The display only supports 16-bit colors, so return a error on any other data format as we don't apply conversion */
  if ((p_Info->imageType == PIF_TYPE_IND16) || (p_Info->imageType == PIF_TYPE_RGB565) || (p_Info->imageType == PIF_TYPE_MONO))
  {
    // Setup the drawing window
    tft.setAddrWindow(p_Info->startX, p_Info->startY, p_Info->startX + p_Info->imageWidth - 1, p_Info->startY + p_Info->imageHeight - 1);
    first = 1;
    return 0;
  }
  else
  {
    return 1;
  }
}

void displayDraw(void *p_Disp, pifINFO_t *p_Info, uint32_t pixel)
{
  // Simply blast the image data out to the display!
  tft.pushColors((uint16_t*)&pixel, 1, first);
  first = 0;
}

/* File I/O functions */
uint16_t fSeekPos;  // Seeking only really is used when a indexed image is read, and no buffer is provided
void *openImage(const char* pc_filePath, int8_t *fileError)
{
  PGM_P imgArray;
  *fileError = 0;
  fSeekPos = 0;

  // Check which of the two images has been requested, otherwise return an error  
  switch (pc_filePath[0])
  {
    case 'a':
      // Arduino Logo
      imgArray = arduino;
      break;
    case 'h':
      // Hackaday Logo      
      imgArray = hackaday;
      break;
    default:
      *fileError = 1;
  }

  // Return the pointer to the image data array in the flash memory
  return imgArray;
}

int8_t closeImage(void *p_file)
{
  // Nothing to do, since we're not dealing with actual file I/O
  return 0;
}

int8_t readImage(void *p_file, uint8_t *p_buffer, uint8_t size)
{
  // Simply read the requested amount of bytes from the flash memory and increase the pointer
  for (uint8_t i = 0; i < size; i++)
  {
    p_buffer[i] = pgm_read_byte((PGM_P)p_file + fSeekPos++);
  }
  return 0;
}

int8_t seekImage(void *p_file, uint32_t offset)
{
  // Change flash-memory pointer
  fSeekPos = offset;
  return 0;
}

void setup() {
  uint16_t ID;
  pifRESULT res = PIF_RESULT_OK;
   
  Serial.begin(115200);

  ID = tft.readID();
  Serial.print(F("Initialising Display ID = 0x"));
  Serial.println(ID, HEX);
  tft.begin(ID);
  
  Serial.print(F("Setting up the PIF Library..."));
  pif_createPainter(&pifPaintingStruct, &displayPrepare, &displayDraw, NULL, NULL, colorLookupTable, sizeof(colorLookupTable));
  pif_createIO(&pifFileIOStruct, &openImage, &closeImage, &readImage, &seekImage);
  pif_createPIFHandle(&pifHandler, &pifFileIOStruct, &pifPaintingStruct);
  Serial.println(F("Done!"));
  
  Serial.println(F("Loading arduino"));
  res = pif_OpenAndDisplay(&pifHandler, "a", 0, 0);
  if (res != PIF_RESULT_OK)
  {
    // Oops, something went wrong... print the error code!
    Serial.println(res, HEX);
  }
  
  Serial.println(F("Loading hackaday."));
  res = pif_OpenAndDisplay(&pifHandler, "h", 96, 176);
  if (res != PIF_RESULT_OK)
  {
    // Oops, something went wrong... print the error code!
    Serial.println(res, HEX);
  }
}

void loop() {
  // put your main code here, to run repeatedly:

}
