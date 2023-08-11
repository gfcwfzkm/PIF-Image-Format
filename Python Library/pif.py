from enum import Enum   # Python 3.10 or higher requried
import numpy as np		# pip install numpy
import PIL.Image		# pip install pillow

class PIF():
	"""
	PIF Image Decoder and Encoder library
	"""

	COLORTABLE_16C = [
		0, 0, 0,		# black
		0, 0, 170,		# blue
		0, 170, 0,		# green
		0, 170, 170,	# cyan
		170, 0, 0,		# red
		170, 0, 170,	# magenta
		170, 170, 0,	# dark yellow
		170, 170, 170,	# light gray
		85, 85, 85,		# dark gray
		85, 85, 255,	# light blue
		85, 255, 85,	# light green
		85, 255, 255,	# light cyan
		255, 85, 85,	# light red
		255, 85, 255,	# light magenta
		255, 255, 85,	# yellow
		255, 255, 255	# white
	]

	COLORTABLE_OFFSET = 28

	class CompressionType(Enum):
		NO_COMPRESSION 	= 0
		RLE_COMPRESSION = 0x7DDE

	class PIFType(Enum):
		ImageTypeRGB888 = 0x433C
		ImageTypeRGB565 = 0xE5C5
		ImageTypeRGB332 = 0x1E53
		ImageTypeRGB16C = 0xB895
		ImageTypeBLWH   = 0x7DAA
		ImageTypeIND24  = 0x4952
		ImageTypeIND16  = 0x4947
		ImageTypeIND8   = 0x4942

	class InvalidHeader(Exception):
		"""Exception raised if file header is not valid

    	Attributes:
        	None
    	"""
		def __init__(self, message = "Invalid or unknown PIF Header") -> None:
			self.message = message
			super().__init__(self.message)

	class PIFInfo():
		def __init__(self) -> None:
			self.fileSize = None	# integer
			self.imageOffset = None	# integer
			self.imageType = None	# PIFType
			self.bitsPerPixel = None # integer
			self.imageWidth = None	# integer
			self.imageHeigt = None	# integer
			self.imageSize = None	# integer
			self.colorTableSize = None	# integer
			self.colorTable = None	# numpy.ndarray, 1D
			self.compression = PIF.CompressionType.NO_COMPRESSION
			self.rawImageData = None # numpy.ndarray, 1D

	def __init__(self) -> None:
		pass

	def __decomplressRLE(rleData: np.ndarray, bitsPerPixel: int, imageSize: int) -> np.ndarray:
		rleInstruction = 0
		imageData = np.zeros(3, dtype=np.int8)
		imagePointer = 0
		dataCounter = 0

		if (bitsPerPixel == 24):
			uncompressedData = np.zeros(imageSize * 3, dtype=np.uint8)
		elif (bitsPerPixel == 16):
			uncompressedData = np.zeros(imageSize * 2, dtype=np.uint8)
		elif (bitsPerPixel == 4):
			uncompressedData = np.zeros(imageSize // 2, dtype=np.uint8)
		else:
			uncompressedData = np.zeros(imageSize, dtype=np.uint8)
		
		while (dataCounter < len(rleData)):
			# Load byte from RLE-compressed image data array
			# and handle it depending on the content of
			# "rleInstruction"...
			imageData[0] = rleData[dataCounter]

			#Check RLE Instruction
			if (rleInstruction > 0):
				# RLE Instruction is positive;
				# Repeat the image data "rleInstruction" amount of times

				# For two or three byte image data: preload extra data
				if (bitsPerPixel >= 16):
					dataCounter += 1
					imageData[1] = rleData[dataCounter]
				if (bitsPerPixel == 24):
					dataCounter += 1
					imageData[2] = rleData[dataCounter]
				
				while (rleInstruction > 0):
					uncompressedData[imagePointer] = imageData[0]
					imagePointer += 1
					rleInstruction -= 1
					if (bitsPerPixel >= 16):
						uncompressedData[imagePointer] = imageData[1]
						imagePointer += 1
					if (bitsPerPixel == 24):
						uncompressedData[imagePointer] = imageData[2]
						imagePointer += 1
			
			elif (rleInstruction < 0):
				# RLE Instruction is negative; The next (rleInstruction * -1) bytes are uncompressed
				uncompressedData[imagePointer] = imageData[0]
				imagePointer += 1
				rleInstruction += 1
				if (bitsPerPixel >= 16):
					dataCounter += 1
					uncompressedData[imagePointer] = rleData[dataCounter]
					imagePointer += 1
				if (bitsPerPixel == 24):
					dataCounter += 1
					uncompressedData[imagePointer] = rleData[dataCounter]
					imagePointer += 1
			
			else:
				# RLE Instruction is 0: The next byte is a RLE Instruction
				rleInstruction = imageData[0]
			
			dataCounter += 1
		
		# Return the decompressed image data
		return uncompressedData
	
	def __convertToRGB888(rawData: np.ndarray, imageSize: int, imageInfo: PIFInfo) -> np.ndarray:
		imageData = np.zeros(imageSize * 3, dtype=np.uint8)
		imageDataPointer = 0

		if (imageInfo.imageType == PIF.PIFType.ImageTypeRGB565) or (imageInfo.imageType == PIF.PIFType.ImageTypeIND16):
			for index in range(0, len(rawData), 2):
				# Convert RGB565 to RGB888 accurately as possible
				imageData[imageDataPointer] = round(((rawData[index] & 0x1F) << 3) * 1.028225806451613)
				imageData[imageDataPointer + 1] = round(((rawData[index] & 0xE0) >> 3 | (rawData[index + 1] & 0x07) << 5) * 1.011904762)
				imageData[imageDataPointer + 2] = round((rawData[index + 1] & 0xF8) * 1.028225806451613)
				imageDataPointer += 3
		elif ((imageInfo.imageType == PIF.PIFType.ImageTypeRGB332) or (imageInfo.imageType  == PIF.PIFType.ImageTypeIND8)):
			for index in range(len(rawData)):
				# Convert RGB332 to RGB888
				imageData[imageDataPointer] = round(((rawData[index] & 0x3) << 6) * 1.328125)
				imageData[imageDataPointer + 1] = round(((rawData[index] & 0x1C) << 3) * 1.138392857)
				imageData[imageDataPointer + 2] = round((rawData[index] & 0xE0) * 1.138392857)
				imageDataPointer += 3
		elif (imageInfo.imageType == PIF.PIFType.ImageTypeRGB16C):
			for index in range(imageSize):
				# Convert from RGB16 to RGB888
				colorValue = (rawData[index // 2] & (0x0F << ((index % 2) * 4))) >> ((index % 2) * 4)
				imageData[imageDataPointer] = PIF.COLORTABLE_16C[colorValue * 3 + 2]
				imageData[imageDataPointer + 1] = PIF.COLORTABLE_16C[colorValue * 3 + 1]
				imageData[imageDataPointer + 2] = PIF.COLORTABLE_16C[colorValue * 3]
				imageDataPointer += 3
		elif (imageInfo.imageType == PIF.PIFType.ImageTypeBLWH):
			for index in range(imageSize):
				# Convert from Monochrome / BlackWhite to RGB888
				PixelVal = rawData[index // 8] & (1 << (index % 8))
				if (PixelVal != 0):
					imageData[imageDataPointer] = 255
					imageData[imageDataPointer + 1] = 255
					imageData[imageDataPointer + 2] = 255
				imageDataPointer += 3

		return imageData

	def __indexedToRGB888(rawData: np.ndarray, imageInfo: PIFInfo) -> np.ndarray:
		bitsperpixel = imageInfo.bitsPerPixel
		imageSize = imageInfo.imageHeigt * imageInfo.imageWidth
		imageData = np.zeros(imageSize * 3, dtype=np.uint8)
		imageDataPointer = 0
		indexedCol = 0
		# Treat 3bpp as 4bpp
		if (bitsperpixel == 3):	bitsperpixel = 4

		if (bitsperpixel > 4):
			for index in range(len(rawData)):
				imageData[imageDataPointer] = imageInfo.colorTable[rawData[indexedCol * 3]]
				imageData[imageDataPointer + 1] = imageInfo.colorTable[rawData[indexedCol * 3 + 1]]
				imageData[imageDataPointer + 2] = imageInfo.colorTable[rawData[indexedCol * 3 + 2]]
				imageDataPointer += 3
		elif (bitsperpixel == 4):
			for index in range(imageSize):
				indexedCol = rawData[index // 2] & ((1 << bitsperpixel) - 1)
				rawData[index // (8 // bitsperpixel)] >>= bitsperpixel
				imageData[imageDataPointer] = imageInfo.colorTable[indexedCol * 3]
				imageData[imageDataPointer + 1] = imageInfo.colorTable[indexedCol * 3 + 1]
				imageData[imageDataPointer + 2] = imageInfo.colorTable[indexedCol * 3 + 2]
				imageDataPointer += 3
		elif (bitsperpixel == 2):
			for index in range(imageSize):
				indexedCol = rawData[index // 4] & ((1 << bitsperpixel) - 1)
				rawData[index // (8 // bitsperpixel)] >>= bitsperpixel
				imageData[imageDataPointer] = imageInfo.colorTable[indexedCol * 3]
				imageData[imageDataPointer + 1] = imageInfo.colorTable[indexedCol * 3 + 1]
				imageData[imageDataPointer + 2] = imageInfo.colorTable[indexedCol * 3 + 2]
				imageDataPointer += 3
		elif (bitsperpixel == 1):
			for index in range(imageSize):
				indexedCol = rawData[index // 8] & ((1 << bitsperpixel) - 1)
				rawData[index // (8 // bitsperpixel)] >>= bitsperpixel
				imageData[imageDataPointer] = imageInfo.colorTable[indexedCol * 3]
				imageData[imageDataPointer + 1] = imageInfo.colorTable[indexedCol * 3 + 1]
				imageData[imageDataPointer + 2] = imageInfo.colorTable[indexedCol * 3 + 2]
				imageDataPointer += 3

		return imageData

	def decode(PIFdata: np.ndarray(shape=(int)),dtype=np.uint8) -> (PIL.Image.Image, PIFInfo):
		"""Decodes a raw PIF bytearray

		Decodes the PIF image header and image data itself and returns it

		Parameters:
			PIFData : numpy.ndarray, mandatory
				Binary PIF data
		
		Returns:
			(PIL.Image.Image, PIFInfo) Tuple:
				Returns a tuple containing the read image as pillow image, as well
				as the header information, color table and raw data within PIFInfo.
		"""
		# Header alone is 28 Bytes, thus reject data smaller than 28 Bytes
		if (PIFdata.size < PIF.COLORTABLE_OFFSET):
			raise "Invalid PIF header, file too small"

		# Valid PIF header starts with the string "PIF\0"
		if (PIFdata[0] != 0x50 or PIFdata[1] != 0x49 or PIFdata[2] != 0x46 or PIFdata[3] != 0x00):
			raise "Invalid PIF header, magic bytes not matching"
		
		imageInfo = PIF.PIFInfo()

		# So far so good, trying to parse in the data
		imageInfo.fileSize = PIFdata.size
		imageInfo.imageOffset = PIFdata[8] + (PIFdata[9] << 8) + (PIFdata[10] << 16) + (PIFdata[11] << 24)
		imageInfo.imageType = PIFdata[12] + (PIFdata[13] << 8)
		imageInfo.bitsPerPixel = PIFdata[14] + (PIFdata[15] << 8)
		imageInfo.imageWidth = PIFdata[16] + (PIFdata[17] << 8)
		imageInfo.imageHeigt = PIFdata[18] + (PIFdata[19] << 8)
		imageInfo.imageSize = PIFdata[20] + (PIFdata[21] << 8) + (PIFdata[22] << 16) + (PIFdata[23] << 24)
		imageInfo.colorTableSize = PIFdata[24] + (PIFdata[25] << 8)
		imageInfo.compression = PIFdata[26] + (PIFdata[27] << 8)

		# Sanity-Check some things, python's enums should raise error if not found
		imageInfo.imageType = PIF.PIFType(imageInfo.imageType)
		imageInfo.compression = PIF.CompressionType(imageInfo.compression)
			
		# Raw image data to process
		imageInfo.rawImageData = np.copy(PIFdata[imageInfo.imageOffset : imageInfo.imageOffset + imageInfo.imageSize])

		# Check if the image is compressed
		if (imageInfo.compression == PIF.CompressionType.RLE_COMPRESSION):
			# Decompress the image data first
			imageInfo.rawImageData = PIF.__decomplressRLE(imageInfo.rawImageData, imageInfo.bitsPerPixel, imageInfo.imageWidth * imageInfo.imageHeigt)
		
		# Need a pure RGB888 image for further processing...
		if (imageInfo.imageType != PIF.PIFType.ImageTypeRGB888) and ((imageInfo.imageType.value & 0xFF00) != (PIF.PIFType.ImageTypeIND16.value & 0xFF00)):
			# Image not RGB888 AND not indexed? Convert it to RGB888
			imageInfo.rawImageData = PIF.__convertToRGB888(imageInfo.rawImageData, imageInfo.imageHeigt * imageInfo.imageWidth, imageInfo)
		elif ((imageInfo.imageType.value & 0xFF00) == (PIF.PIFType.ImageTypeIND8.value & 0xFF00)):
			# Image is Indexed!
			# Read in the color table values 
			imageInfo.colorTable = np.copy(PIFdata[PIF.COLORTABLE_OFFSET : PIF.COLORTABLE_OFFSET + imageInfo.colorTableSize])
			
			# Convert colortable to RGB888, if it's not
			if (imageInfo.imageType != PIF.PIFType.ImageTypeIND24):
				if (imageInfo.imageType == PIF.PIFType.ImageTypeIND16):
					ColTableColors = imageInfo.colorTableSize // 2
				else:
					ColTableColors = imageInfo.colorTableSize
				imageInfo.colorTable = PIF.__convertToRGB888(imageInfo.colorTable, ColTableColors, imageInfo)

			# Finally convert the indexed image data to an pure RGB888 array
			imageInfo.rawImageData = PIF.__indexedToRGB888(imageInfo.rawImageData, imageInfo)
				
		# Converting to HxWx3 (mostly for easy pillow integration)
		rgbImage = np.reshape(imageInfo.rawImageData, (imageInfo.imageHeigt, imageInfo.imageWidth, -3))

		# Since the data is little endian, rgbImage is actually in an BGR format!
		# So it has to be fixed...
		rgbImage = rgbImage[:, :, ::-1]

		# Return a Pillow image as well as the header / file information
		return (PIL.Image.fromarray(rgbImage, 'RGB'), imageInfo)
	
	def __convertImage(image: PIL.Image.Image, imageType: PIFType, IndexedColorTable: None, dithering: bool, internal: bool) -> PIL.Image.Image:
		match imageType:
			case PIF.PIFType.ImageTypeRGB888:
				imageToReturn = image.copy()
			case PIF.PIFType.ImageTypeRGB565:
				# Get the image as a numpy array
				imageData = np.array(image, dtype=np.uint8)
				# Create an array of the same size/dimensions with the logical 
				# mask for each color
				imageMask = np.tile(np.array([0xF8, 0xFC, 0xF8], dtype=np.uint8), (image.height, image.width, 1))
				# Logic-AND the data with the mask
				imageData = np.bitwise_and(imageData, imageMask)
				
				# If the image is used for the preview, we have to fix the colors a little...
				if (internal == False):
					# The idea is to represent the colors accurately. RGB565 white would be fully white on a RGB565 display,
					# but lightly gray on a RGB888 display. To compensate that, a factor is multiplied to all values...
					multMask = np.tile(np.array([1.028225806, 1.011904762, 1.028225806], dtype=np.float64), (image.height, image.width, 1))
					imageData = np.rint(np.multiply(imageData, multMask, dtype=np.float32)).astype(dtype=np.uint8)

				# Convert it back to a PIL image
				imageToReturn = PIL.Image.fromarray(imageData, 'RGB')
			case PIF.PIFType.ImageTypeRGB332:
				# Placeholder image to contain the new color palette
				maskImage = PIL.Image.new('P', (16,16))
				# Create an [R,G,B] array with 256 entries
				maskData = np.stack((np.arange(256, dtype=np.uint8), np.arange(256, dtype=np.uint8), np.arange(256, dtype=np.uint8)), axis=1, dtype=np.uint8)
				# Create the masking array to cut off the RGB888 to RGB332-ish colors
				# Red = Color & 0xE0; Green = Color & 0x1C; Blue = Color & 0x03
				maskRGB332 = np.tile(np.array([0xE0, 0x1C, 0x03], dtype=np.uint8), (256, 1))
				maskData = np.bitwise_and(maskData, maskRGB332)
				# Green and blue are located too low, gotta shift them up with an... exactly, another array!
				maskRGB332 = np.tile(np.array([0, 3, 6], dtype=np.uint8), (256, 1))
				maskData = np.left_shift(maskData, maskRGB332, dtype=np.uint8)

				# Similar as with RGB565, apply an mask for image previews to display the colors
				# precisely on a RGB888 display / image.
				if (internal == False):
					maskRGB332 = np.tile(np.array([1.138392857, 1.138392857, 1.328125], dtype=np.float64), (256,1))
					maskData = np.rint(np.multiply(maskData, maskRGB332, dtype=np.float32)).astype(dtype=np.uint8)
				
				# Fuck is numpy great! Flatten the [256,3] Array to one long, continous 1D array
				maskData = maskData.flatten()
				# Apply the color palette into the dummy mask image
				maskImage.putpalette(maskData.tolist())
				# Apply the dummy mask image palette into the actual image, apply optional dithering
				imageToReturn = image.quantize(256, palette=maskImage, dither=dithering)
			case _:
				raise f'Unknown format type: {imageType}'
		
		return imageToReturn

	#def encodeFile(image: PIL.Image.Image, imageType: ConversionType, compression: CompressionType, IndexedColorTable: (np.ndarray, int) | None, dithering = False) -> (np.ndarray, PIFInfo):
	#	pass
	
	def encodePreview(image: PIL.Image.Image, imageType: PIFType, IndexedColorTable: None | tuple[np.ndarray(shape=(any,3),dtype=np.uint8), int], dithering: bool = False) -> PIL.Image.Image:
		imageToReturn = PIF.__convertImage(image, imageType, IndexedColorTable, dithering, internal=False)
		return imageToReturn

"""
Decode Test
a = np.fromfile('/root/Programming/PIF-Image-Format/test_images/Lenna/Lenna_RGB888.pif', dtype=np.uint8)
image, info = PIF.decode(a)
img = Image.fromarray(image, 'RGB')
img.save('te123st.bmp')
"""

a = PIL.Image.open('/root/Programming/PIF-Image-Format/test_images/Lenna/Lenna.bmp')
b = PIF.encodePreview(a, PIF.PIFType.ImageTypeRGB332, None)
b.save('test.bmp')