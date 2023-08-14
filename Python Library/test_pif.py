
import numpy as np
import PIL.Image
import time
import os
import sys
sys.path.append('Python_Library/')
from pif import *

TEST_LIST = [
	(PIF.PIFType.ImageTypeRGB888, PIF.CompressionType.NO_COMPRESSION),
	(PIF.PIFType.ImageTypeRGB888, PIF.CompressionType.RLE_COMPRESSION),
	(PIF.PIFType.ImageTypeRGB565, PIF.CompressionType.NO_COMPRESSION),
	(PIF.PIFType.ImageTypeRGB565, PIF.CompressionType.RLE_COMPRESSION),
	(PIF.PIFType.ImageTypeRGB332, PIF.CompressionType.NO_COMPRESSION),
	(PIF.PIFType.ImageTypeRGB332, PIF.CompressionType.RLE_COMPRESSION),
	(PIF.PIFType.ImageTypeRGB332, PIF.CompressionType.NO_COMPRESSION),
	(PIF.PIFType.ImageTypeRGB16C, PIF.CompressionType.NO_COMPRESSION),
	(PIF.PIFType.ImageTypeRGB16C, PIF.CompressionType.RLE_COMPRESSION),
	(PIF.PIFType.ImageTypeBLWH,	  PIF.CompressionType.NO_COMPRESSION),
	(PIF.PIFType.ImageTypeBLWH,	  PIF.CompressionType.RLE_COMPRESSION),
	(PIF.PIFType.ImageTypeIND24,  PIF.CompressionType.NO_COMPRESSION),
	(PIF.PIFType.ImageTypeIND24,  PIF.CompressionType.RLE_COMPRESSION),
	(PIF.PIFType.ImageTypeIND16,  PIF.CompressionType.NO_COMPRESSION),
	(PIF.PIFType.ImageTypeIND16,  PIF.CompressionType.RLE_COMPRESSION),
	(PIF.PIFType.ImageTypeIND8,	  PIF.CompressionType.NO_COMPRESSION),
	(PIF.PIFType.ImageTypeIND8,	  PIF.CompressionType.RLE_COMPRESSION),
]

test_colorTable = (np.array([[255, 255, 255],
							[0, 0, 0],
							[123, 123, 123],
							[98, 76, 54]], dtype=np.uint8), 4)

# imagePath = 'test_images/colorful/colorful.bmp'
imagePath = 'test_images/Lenna/Lenna.bmp'

"""
Basic test routine to check the pif library by converting
an test image into various paths to a PIF byte array and
reading it back to a BMP/Image file.
The PIF and BMP files are saved in a test folder, which have to be
then manually inspected.
"""

testpath = 'Python Library/testing'
if not os.path.exists(testpath):
    os.makedirs(testpath)

origImage = PIL.Image.open(imagePath)

for test_case in TEST_LIST:
    print(f'\n\nTesting {test_case[0].name} with {test_case[1].name}')
    print(f'Opening and encoding file...')
    startTime = time.time()
    rawPIF = PIF.encodeFile(origImage, test_case[0], test_case[1], test_colorTable, True if test_case[1] == PIF.CompressionType.NO_COMPRESSION else False)
    endTime = time.time()
    print(f' {endTime - startTime} seconds\n')
    print(f'Decoding file...')
    startTime = time.time()
    decodedPIF, _ = PIF.decode(rawPIF)
    endTime = time.time()
    print(f' {endTime - startTime} seconds\n')
    print(f'End')
    decodedPIF.save(f'{testpath}/{(test_case[0].name).split(".")[-1]}{"_rle" if test_case[1] == PIF.CompressionType.RLE_COMPRESSION  else ""}.bmp')
    rawPIF.tofile(f'{testpath}/{(test_case[0].name).split(".")[-1]}{"_rle" if test_case[1] == PIF.CompressionType.RLE_COMPRESSION  else ""}.pif')
