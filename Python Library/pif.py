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

	def decode(PIFdata: np.ndarray) -> (PIL.Image.Image, PIFInfo):
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
	
	def __LEGACYrleCompress(pixelArray: np.ndarray):
		outlist = []
		rlePos = []
		tempArray = [None]*129
		outCnt = 0
		pixCnt = 0

		tempArray[0] = pixelArray[pixCnt]
		pixCnt += 1

		while not (pixCnt > len(pixelArray)):

			if (tempArray[0] == None):	break
			if (pixCnt >= len(pixelArray)):
				tempArray[1] = None
			else:
				tempArray[1] = pixelArray[pixCnt]
			pixCnt += 1

			# Check if a compressible sequence starts or not
			if (tempArray[0] != tempArray[1]):
				# Uncompressible sequence!
				outCnt = 1

				# Run this loop as long as there is still data to read, the counter being below 128 AND 
				# the previous read word isn't the same as the one before
				while True:
					outCnt += 1
					if (pixCnt >= len(pixelArray)):
						tempArray[outCnt] = None
					else:
						tempArray[outCnt] = pixelArray[pixCnt]
					pixCnt += 1
					# Makeshift "Do-While" loop
					if (pixCnt >= len(pixelArray)):
						tempArray[outCnt] = None
						break
					if not (outCnt < 127):
						break
					if not (tempArray[outCnt] != tempArray[outCnt - 1]):
						break

				keep = tempArray[outCnt]
				if (tempArray[outCnt] == tempArray[outCnt - 1]):
					outCnt -= 1

				#Add to list
				rlePos.append(len(outlist))
				outlist.append(outCnt * -1)
				outlist.extend(tempArray[:outCnt])

				tempArray[0] = tempArray[outCnt]
				if (keep == None or not outCnt < 127):
					continue
				
			# Compressible sequence
			outCnt = 2

			while True:
				if (pixCnt >= len(pixelArray)):
					tempArray[1] = None
				else:
					tempArray[1] = pixelArray[pixCnt]
				pixCnt += 1
				outCnt += 1
				if not (outCnt < 127):
					break
				if not (tempArray[0] == tempArray[1]):
					break
				
			rlePos.append(len(outlist))
			outlist.append(outCnt - 1)
			outlist.append(tempArray[0])

			tempArray[0] = tempArray[1]
	
		rlePos.append(None)
		return rlePos,outlist
	
	def __LEGACYconvertToPIF(image: PIL.Image.Image, conversion: PIFType, colorLength: int, colorTable: np.ndarray, dithering: bool, compression: CompressionType):
		class ImageH(Enum):
			IMAGETYPE = 0
			BITSPERPIXEL = 1
			IMAGEWIDTH = 2
			IMAGEHEIGHT = 3
			IMAGESIZE = 4
			COLTABLESIZE = 5
			COMPRTYPE = 6

		checkImageType = conversion
		imageHeader = [0,0,0,0,0,0,0]
		imageColors = []
		# Image Data contains the pixels in word-size (min 1 byte, max 3 bytes)
		# Proper conversion done when saving the data, BITSPERPIXEL to process correctly
		imageData = []

		imageWord = 0
		imageCnt = 0
		color = 0

		TemporaryImage = image.copy()

		#Write the image header with the information we already have
		imageHeader[ImageH.COMPRTYPE.value] = compression.value

		if ((checkImageType != PIF.PIFType.ImageTypeRGB888) and (checkImageType !=  PIF.PIFType.ImageTypeRGB565)):
			TemporaryImage = PIF.__convertImage(image, checkImageType, (colorTable, colorLength), dithering, True)
		else:
			TemporaryImage = PIF.__convertImage(image, PIF.PIFType.ImageTypeRGB888, (colorTable, colorLength), dithering, True)

		if ((checkImageType ==  PIF.PIFType.ImageTypeIND8) or (checkImageType ==  PIF.PIFType.ImageTypeIND16) or (checkImageType ==  PIF.PIFType.ImageTypeIND24)):
			checkImageType =  PIF.PIFType.ImageTypeIND8

		imageHeader[ImageH.IMAGEWIDTH.value] = TemporaryImage.width
		imageHeader[ImageH.IMAGEHEIGHT.value] = TemporaryImage.height

		match checkImageType:
			case PIF.PIFType.ImageTypeIND8:
				# Write the Image Type and Bits per Pixel into the imageHeader
				if (conversion == PIF.PIFType.ImageTypeIND8):
					imageHeader[ImageH.IMAGETYPE.value] = 0x4942	# Indexed8 mode selected
				elif (conversion == PIF.PIFType.ImageTypeIND16):
					imageHeader[ImageH.IMAGETYPE.value] = 0x4947	# Indexed16 mode selected
				else:
					imageHeader[ImageH.IMAGETYPE.value] = 0x4952	# Indexed24 mode selected
				imageHeader[ImageH.BITSPERPIXEL.value] = int(colorLength - 1).bit_length()

				# Create the color table
				for colval in range(colorLength):
					if (conversion == PIF.PIFType.ImageTypeIND8):
						imageColors.append(colorTable[colval * 3] | (colorTable[colval * 3 + 1] >> 3) | (colorTable[colval * 3 + 2] >> 6))
					elif (conversion == PIF.PIFType.ImageTypeIND16):
						imageColors.append((colorTable[colval * 3] << 8) | (colorTable[colval * 3 + 1] << 3) | (colorTable[colval * 3 + 2] >> 3))
					else:
						imageColors.append((colorTable[colval * 3] << 16) | (colorTable[colval * 3 + 1] << 8) | colorTable[colval * 3 + 2])

				# Write the Image Type and Bits per Pixel into the imageHeader
				if (conversion == PIF.PIFType.ImageTypeIND8):
					imageHeader[ImageH.IMAGETYPE.value] = 0x4942	# Indexed8 mode selected
					imageHeader[ImageH.COLTABLESIZE.value] = len(imageColors)
				elif (conversion == PIF.PIFType.ImageTypeIND16):
					imageHeader[ImageH.IMAGETYPE.value] = 0x4947	# Indexed16 mode selected
					imageHeader[ImageH.COLTABLESIZE.value] = len(imageColors) * 2
				else:
					imageHeader[ImageH.IMAGETYPE.value] = 0x4952	# Indexed24 mode selected
					imageHeader[ImageH.COLTABLESIZE.value] = len(imageColors) * 3
				imageHeader[ImageH.BITSPERPIXEL.value] = int(colorLength - 1).bit_length()
				# Check every pixel and pack it into a byte if possible
				for y in range(TemporaryImage.height):
					for x in range(TemporaryImage.width):
						index = TemporaryImage.getpixel((x,y))
						if (imageHeader[ImageH.BITSPERPIXEL.value] <=4):
							if (imageHeader[ImageH.BITSPERPIXEL.value] >= 3):
								imageWord |= index << imageCnt
								imageCnt += 4
							elif (imageHeader[ImageH.BITSPERPIXEL.value] == 2):
								imageWord |= index << imageCnt
								imageCnt += 2
							elif (imageHeader[ImageH.BITSPERPIXEL.value] == 1):
								imageWord = imageWord | (index << imageCnt)
								imageCnt += 1
							if (imageCnt >= 8):
								imageData.append(imageWord)
								imageWord = 0
								imageCnt = 0
						else:
							imageData.append(index)
				# Add remaining Pixels, if there are any
				if (imageCnt > 0):
					imageData.append(imageWord)					
			case PIF.PIFType.ImageTypeBLWH:
				# Write the Image Type and Bits per Pixel into the imageHeader
				imageHeader[ImageH.IMAGETYPE.value] = 0x7DAA	# B/W mode selected
				imageHeader[ImageH.BITSPERPIXEL.value] = 1		# 1 Bits per Pixel
				# Check every pixel and pack to a group of 8 within a byte
				for y in range(TemporaryImage.height):
					for x in range(TemporaryImage.width):
						color = TemporaryImage.getpixel((x,y))
						if (color): # 0 = black, 255 = white
							imageWord |= 1 << imageCnt
						imageCnt += 1
						if (imageCnt >= 8):
							imageData.append(imageWord)
							imageWord = 0
							imageCnt = 0
				# Add remaining Pixels, if there are any
				if (imageCnt > 0):
					imageData.append(imageWord)
			case PIF.PIFType.ImageTypeRGB16C:
				# Write the Image Type and Bits per Pixel into the imageHeader
				imageHeader[ImageH.IMAGETYPE.value] = 0xB895	# RGB16C mode selected
				imageHeader[ImageH.BITSPERPIXEL.value] = 4		# 4 Bits per Pixel
				# Check every pixel and pack to a group of 2 within a byte
				for y in range(TemporaryImage.height):
					for x in range(TemporaryImage.width):
						color = TemporaryImage.getpixel((x,y))
						# Check which of the 16 colors is used to return a value between 0 and 15
						imageWord = imageWord | (color << ((imageCnt % 2) * 4))
						imageCnt = imageCnt + 1
						if (imageCnt >= 2):
							imageData.append(imageWord)
							imageWord = 0
							imageCnt = 0
				# Add remaining Pixels, if there are any
				if (imageCnt > 0):
					imageData.append(imageWord)
			case PIF.PIFType.ImageTypeRGB332:
				# Write the Image Type and Bits per Pixel into the imageHeader
				TemporaryImage = TemporaryImage.convert('RGB')
				imageHeader[ImageH.IMAGETYPE.value] = 0x1E53	# RGB332 mode selected
				imageHeader[ImageH.BITSPERPIXEL.value] = 8		# 8 Bits per Pixel
				for y in range(TemporaryImage.height):
					for x in range(TemporaryImage.width):
						color = list(TemporaryImage.getpixel((x,y)))
						color[0] = color[0] & 0xE0	# R
						color[1] = color[1] & 0xE0	# G
						color[2] = color[2] & 0xC0	# B
						imageData.append(color[0] | (color[1] >> 3) | (color[2]) >> 6)
			case PIF.PIFType.ImageTypeRGB565:
				imageHeader[ImageH.IMAGETYPE.value] = 0xE5C5	# RGB565 mode selected
				imageHeader[ImageH.BITSPERPIXEL.value] = 16		# 16 Bits per Pixel
				# Relatively easy here, just trim the 24-bit RGB image to 16-bit RGB
				# and add it to the list
				for y in range(TemporaryImage.height):
					for x in range(TemporaryImage.width):
						color = list(TemporaryImage.getpixel((x,y)))
						color[0] = color[0] & 0xF8	# R
						color[1] = color[1] & 0xFC	# G
						color[2] = color[2] & 0xF8	# B
						imageData.append((color[0] << 8) | (color[1] << 3) | ((color[2] >> 3)))
			case PIF.PIFType.ImageTypeRGB888:
				imageHeader[ImageH.IMAGETYPE.value] = 0x433C	# RGB888 mode selected
				imageHeader[ImageH.BITSPERPIXEL.value] = 24		# 24 Bits per Pixel
				# Fit the color into 24bit and add it to the list
				for y in range(TemporaryImage.height):
					for x in range(TemporaryImage.width):
						color = list(TemporaryImage.getpixel((x,y)))
						imageData.append((color[0] << 16) | (color[1] << 8) | (color[2]))

		# compress the data if requested
		if (compression == PIF.CompressionType.RLE_COMPRESSION):
			rlePos, imageData = PIF.__LEGACYrleCompress(imageData)
		else:
			rlePos = [None]

		# Write down the image size into the header
		imageHeader[ImageH.IMAGESIZE.value] = len(imageData)

		# Return the image header, color table and image data
		return imageHeader,imageColors,imageData,rlePos
	
	def __LEGACYsavePIFbinary(imageHeader, colorTable, imageData, rlePos):
		tTotalPIF = []
		tPIFHeader = [None] * 12
		tImgHeader = [None] * 16
		if (imageHeader[0] == 0x4942):
			tColTable = [None] * imageHeader[5]
		elif (imageHeader[0] == 0x4947):
			tColTable = [None] * imageHeader[5]
		else:
			tColTable = [None] * imageHeader[5]

		tImgData = []

		tPIFHeader[0] = 0x50
		tPIFHeader[1] = 0x49
		tPIFHeader[2] = 0x46
		tPIFHeader[3] = 0

		tImgHeader[0] = imageHeader[0] & 0xFF
		tImgHeader[1] = (imageHeader[0] & 0xFF00) >> 8
		tImgHeader[2] = imageHeader[1] & 0xFF
		tImgHeader[3] = (imageHeader[1] & 0xFF00) >> 8
		tImgHeader[4] = imageHeader[2] & 0xFF
		tImgHeader[5] = (imageHeader[2] & 0xFF00) >> 8
		tImgHeader[6] = imageHeader[3] & 0xFF
		tImgHeader[7] = (imageHeader[3] & 0xFF00) >> 8
		tImgHeader[12] = imageHeader[5] & 0xFF
		tImgHeader[13] = (imageHeader[5] & 0xFF00) >> 8
		tImgHeader[14] = imageHeader[6] & 0xFF
		tImgHeader[15] = (imageHeader[6] & 0xFF00) >> 8

		if (imageHeader[5] > 0):
			for index in range(len(colorTable)):
				if (imageHeader[0] == 0x4942):
					tColTable[index] = colorTable[index]
				elif (imageHeader[0] == 0x4947):
					tColTable[index * 2] = (colorTable[index] & 0xFF)
					tColTable[index * 2 + 1] = (colorTable[index] & 0xFF00) >> 8
				else:

					tColTable[index * 3] = (colorTable[index] & 0xFF) 
					tColTable[index * 3 + 1] = (colorTable[index] & 0xFF00) >> 8
					tColTable[index * 3 + 2] = (colorTable[index] & 0xFF0000) >> 16

		rleIndex = 0
		for index in range(len(imageData)):
			# RLE Data is always only 1 byte large, while image data can be up to three bytes large
			if ((imageHeader[6] != 0) and (rlePos[rleIndex] == index)):
				rleIndex += 1
				tImgData.append(imageData[index] & 0xFF)
			else:
				if (imageHeader[1] == 16):
					tImgData.append(imageData[index] & 0xFF)
					tImgData.append((imageData[index] & 0xFF00) >> 8)
				elif (imageHeader[1] == 24):
					tImgData.append(imageData[index] & 0xFF)
					tImgData.append((imageData[index] & 0xFF00) >> 8)
					tImgData.append((imageData[index] & 0xFF0000) >> 16)
				else:
					tImgData.append(imageData[index] & 0xFF)

		imageHeader[4] = len(tImgData)

		tImgHeader[8] = imageHeader[4] & 0xFF
		tImgHeader[9] = (imageHeader[4] & 0xFF00) >> 8
		tImgHeader[10] = (imageHeader[4] & 0xFF0000) >> 16
		tImgHeader[11] = (imageHeader[4] & 0xFF000000) >> 24

		tTotalPIF.extend(tPIFHeader)
		tTotalPIF.extend(tImgHeader)
		if (imageHeader[5] > 0):
			tTotalPIF.extend(tColTable)
		iStart = len(tTotalPIF)
		tTotalPIF.extend(tImgData)
		iSize = len(tTotalPIF)

		tTotalPIF[4] = iSize & 0xFF
		tTotalPIF[5] = (iSize & 0xFF00) >> 8
		tTotalPIF[6] = (iSize & 0xFF0000) >> 16
		tTotalPIF[7] = (iSize & 0xFF000000) >> 24

		tTotalPIF[8] = iStart & 0xFF
		tTotalPIF[9] = (iStart & 0xFF00) >> 8
		tTotalPIF[10] = (iStart & 0xFF0000) >> 16
		tTotalPIF[11] = (iStart & 0xFF000000) >> 24

		return (np.array(tTotalPIF),iSize)

	def __convertImage(image: PIL.Image.Image, imageType: PIFType, ColorTableInfo: None | tuple[np.ndarray, int], dithering: bool, internal: bool) -> tuple[PIL.Image.Image, None | tuple[np.ndarray, int]]:
		IndexedColorTable, ColorTableLength = ColorTableInfo		
		if ((imageType == PIF.PIFType.ImageTypeIND8) or (imageType == PIF.PIFType.ImageTypeIND16) or (imageType == PIF.PIFType.ImageTypeIND24)):
			if ((type(ColorTableInfo) != tuple) and (type(IndexedColorTable) != np.ndarray)):
				raise 'IndexedColorTable required for indexed image modes!'
			else:
				if ((np.shape(IndexedColorTable)[0] != ColorTableLength) or (np.shape(IndexedColorTable)[1] != 3)):
					raise 'IndexedColorTable needs to be of the format [length, [R,G,B]]'
		
		# Process image depending on PIFType
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
					multMask = np.tile(np.array([1.028225806, 1.011904762, 1.028225806], dtype=np.float32), (image.height, image.width, 1))
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
					maskRGB332 = np.tile(np.array([1.138392857, 1.138392857, 1.328125], dtype=np.float32), (256,1))
					maskData = np.rint(np.multiply(maskData, maskRGB332, dtype=np.float32)).astype(dtype=np.uint8)
				
				# Fuck is numpy great! Flatten the [256,3] Array to one long, continous 1D array
				maskData = maskData.flatten()
				# Apply the color palette into the dummy mask image
				maskImage.putpalette(maskData.tolist())
				# Apply the dummy mask image palette into the actual image, apply optional dithering
				imageToReturn = image.quantize(256, palette=maskImage, dither=dithering)
			case PIF.PIFType.ImageTypeRGB16C:
				# Converting to RGB16C is similar to RGB332, just a lot simpler
				maskImage = PIL.Image.new('P', (16,16))
				# Use the 16 (R,G,B) color palette as lookup
				maskData = PIF.COLORTABLE_16C.copy()
				# Fill up the rest with emptyness
				maskData.extend(maskData[:3] * 240)
				maskImage.putpalette(maskData)
				imageToReturn = image.quantize(16, palette=maskImage, dither=dithering)
			case PIF.PIFType.ImageTypeBLWH:
				# Is there anything to comment here?
				imageToReturn = image.convert("1", dither=dithering)
			case PIF.PIFType.ImageTypeIND24:
				# Handling the indexed images is very similar to RGB332, using
				# an image palette to generate the indexed array
				maskImage = PIL.Image.new('P', (16,16))
				# Flatten things to a continous, single array / dimension
				temporary = IndexedColorTable.flatten()
				# Fill out any remaining space, just to be save
				temporary = np.append(temporary, np.tile(temporary[:1], (256 - ColorTableLength)))
				# Apply color palette to dummy image
				maskImage.putpalette(temporary)
				# Apply dummy image palette to actual image
				imageToReturn = image.quantize(IndexedColorTable, palette=maskImage, dither=dithering)
			case PIF.PIFType.ImageTypeIND16:
				# Will be similar to IND24 and RGB565
				maskImage = PIL.Image.new('P', (16,16))
				colorMask = np.tile(np.array([0xF8, 0xFC, 0xF8], dtype=np.uint8), (image.height, image.width, 1))
				# Logic-AND the data with the mask
				IndexedColorTable = np.bitwise_and(IndexedColorTable, colorMask)

				temporary = IndexedColorTable.copy()
				# If the image is used for the preview, we have to fix the colors a little...
				if (internal == False):
					# The idea is to represent the colors accurately. RGB565 white would be fully white on a RGB565 display,
					# but lightly gray on a RGB888 display. To compensate that, a factor is multiplied to all values...
					multMask = np.tile(np.array([1.028225806, 1.011904762, 1.028225806], dtype=np.float32), (ColorTableLength, 1))
					temporary = np.rint(np.multiply(temporary, multMask, dtype=np.float32)).astype(dtype=np.uint8)
				
				# Fill out any remaining space, just to be save
				temporary = np.append(temporary, np.tile(temporary[:1], (256 - ColorTableLength)))
				# Apply color palette to dummy image
				maskImage.putpalette(temporary)
				# Apply dummy image palette to actual image
				imageToReturn = image.quantize(ColorTableLength, palette=maskImage, dither=dithering)
			case PIF.PIFType.ImageTypeIND8:
				maskImage = PIL.Image.new('P', (16,16))
				maskRGB332 = np.tile(np.array([0xE0, 0xE0, 0xC0], dtype=np.uint8), (ColorTableLength, 1))
				# Logic-AND the data with the mask
				IndexedColorTable = np.bitwise_and(IndexedColorTable, maskRGB332)	
				temporary = IndexedColorTable.copy()

				# If the image is used for the preview, we have to fix the colors a little...
				if (internal == False):
					# The idea is to represent the colors accurately. RGB565 white would be fully white on a RGB565 display,
					# but lightly gray on a RGB888 display. To compensate that, a factor is multiplied to all values...
					multMask = np.tile(np.array([1.138392857, 1.138392857, 1.328125], dtype=np.float32), (ColorTableLength, 1))
					temporary = np.rint(np.multiply(temporary, multMask, dtype=np.float32)).astype(dtype=np.uint8)
				
				# Fill out any remaining space, just to be save
				temporary = np.append(temporary, np.tile(temporary[:1], (256 - ColorTableLength)))
				# Apply color palette to dummy image
				maskImage.putpalette(temporary)
				# Apply dummy image palette to actual image
				imageToReturn = image.quantize(ColorTableLength, palette=maskImage, dither=dithering)
			case _:
				raise f'Unknown format type: {imageType}'
		
		return (imageToReturn, (IndexedColorTable, ColorTableLength))

	def encodeFile(image: PIL.Image.Image, imageType: PIFType, compression: CompressionType, IndexedColorTable: None | tuple[np.ndarray, int], dithering: bool = False) -> np.ndarray:
		if not type(IndexedColorTable) == None:
			ColorIndexLength = IndexedColorTable[1]
			ColorIndexTable = IndexedColorTable[0]
		else:
			ColorIndexLength = None
			ColorIndexTable = None
		
		imageHeader, colorTable, imageData, rlePos = PIF.__LEGACYconvertToPIF(image, imageType, ColorIndexLength, ColorIndexTable, dithering, compression)
		dataPIF,_ = PIF.__LEGACYsavePIFbinary(imageHeader, colorTable, imageData, rlePos)		
		return dataPIF
	
	def encodePreview(image: PIL.Image.Image, imageType: PIFType, IndexedColorTable: None | tuple[np.ndarray, int], dithering: bool = False) -> PIL.Image.Image:
		""" Gets an preview of the image

		Generates an preview of the PIF parameters and optional IndexedColorTable to
		get a rough estimate of the image, that will be generated

		Parameters:
			image : PIL.Image.Image
				Image to convert / get the preview of
			imageType : PIF.PIFType
				PIF Type to convert the image into
			IndexedColorTable : None or (np.ndarray, int)
				If the image type is not indexed, None is expected
				Otherwise a tuple containing a [R,G,B] numpy array and the amount of colors
			dithering : bool
				Enable dithering, doesn't work for ImageTypeRGB888 or ImageTypeRGB565
		
		Returns:
			PIL.Image.Image
				A generated preview image that would represent the PIF image at the given parameters
		"""
		imageToReturn,_ = PIF.__convertImage(image, imageType, IndexedColorTable, dithering, internal=False)
		return imageToReturn

"""
Decode Test
a = np.fromfile('/root/Programming/PIF-Image-Format/test_images/Lenna/Lenna_RGB888.pif', dtype=np.uint8)
image, info = PIF.decode(a)
img = Image.fromarray(image, 'RGB')
img.save('te123st.bmp')
"""

a = PIL.Image.open('test_images/Lenna/Lenna.bmp')
b = PIF.encodePreview(a, PIF.PIFType.ImageTypeIND8, (np.array([[123,132,12],[255,255,255],[0,0,0]], dtype=np.uint8), 3))
b.save('test.bmp')